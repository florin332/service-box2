// ============================================================================
// dut_host.cpp - implementare USB host PIO (doar SERVICEBOX_WAVESHARE).
//
// Resurse folosite (fara conflicte cu restul placii):
//   - PIO1 integral (3 SM: TX/RX/EOP) - PIO0 ramane pentru SDIO (card SD)
//   - DMA channel 9 - ferit de claim-urile timpurii ale SDIO-ului
//   - GP0 (D+) / GP1 (D-), fara VBUS in cablu (DUT alimentat separat)
//
// THREAD-SAFETY (cauza blocajului la SRV BOOTSEL): TinyUSB host NU este
// sigur cross-core. Adafruit_USBH_CDC::write() apeleaza tuh_task() intern
// daca FIFO-ul de TX e plin; apelata din core0 in timp ce core1 ruleaza
// USBHost.task(), structurile de endpoint ale PIO host erau modificate
// din doua nuclee simultan -> host-ul ingheta si nu mai vedea detach-ul.
//
// Regula: ORICE apel TinyUSB host (CDC write/read/available, tuh_msc_*,
// USBHost.task, pio_usb_host_frame) ruleaza EXCLUSIV pe core1. core0
// comunica prin mailbox-uri simple (comanda TX + buffer RX + slot MSC),
// completata cu bariere __sync_synchronize().
// ============================================================================

#include "dut_host.h"
#include "dut_protocol.h"

#if defined(SERVICEBOX_WAVESHARE)

#include <Arduino.h>
#include <string.h>
#include <hardware/clocks.h>

#include <Adafruit_TinyUSB.h>
#include "pio_usb.h"

// Acces la root port-ul PIO-USB pentru re-enumerarea fortata. Simbolul este
// definit in pio_usb.c (C); bitul DISCONNECT este din pio_usb_ll.h
// (PIO_USB_INTS_DISCONNECT_POS = 1).
extern "C" {
extern root_port_t pio_usb_root_port[];
}
static constexpr uint32_t PIO_INTS_DISCONNECT = (1u << 1);

// GP0 = D+, GP1 = D- (D- trebuie sa fie pinul consecutiv urmator).
static constexpr uint8_t PIN_USB_HOST_DP = 0;

static Adafruit_USBH_Host USBHost;
static Adafruit_USBH_CDC  SerialHost;

// ---------------------------------------------------------------------------
// Stare partajata (scriitor = core1 / callback-uri; cititor = core0)
// ---------------------------------------------------------------------------

static volatile DutLink s_link       = DutLink::NONE;
static volatile uint8_t s_mscDevAddr = 0;

// ---------------------------------------------------------------------------
// Mailbox TX (core0 -> core1): o singura comanda in zbor (protocolul DUT
// este oricum serializat: o comanda -> un raspuns).
// ---------------------------------------------------------------------------

static char     s_txLine[64];
static volatile bool s_txPending = false;

// ---------------------------------------------------------------------------
// Buffer RX partajat (core1 umple, core0 consuma) - linii asamblate deja.
// ---------------------------------------------------------------------------

static char     s_rxLine[80];
static volatile size_t s_rxLen    = 0;   // >0 = linie completa disponibila
static char     s_rxAssembling[80];
static size_t   s_rxAsmLen        = 0;

// ---------------------------------------------------------------------------
// Slot MSC (core0 cere, core1 executa) - un singur transfer in zbor.
// ---------------------------------------------------------------------------

static volatile bool     s_mscReq        = false;
static uint32_t          s_mscLba        = 0;
static uint16_t          s_mscBlocks     = 0;
static const uint8_t*    s_mscBuf        = NULL;
static volatile bool     s_mscDone       = false;
static volatile bool     s_mscOk         = false;
static volatile bool     s_xferComplete  = false;
static volatile bool     s_xferOk        = false;

// ---- Re-enumerare fortata (core0 cere, core1 executa, doua faze) ----------
// Faza A: publica DISCONNECT catre TinyUSB (consumat de handler-ul din frame).
// Faza B: dupa ~50 ms elibereaza portul (connected=false) ca detectia de
// FS_IDLE din pio_usb_host_frame() sa produca un CONNECT curat spre
// bootloader-ul DUT. Fara faza A, TinyUSB ar ramane cu CDC-ul vechi montat;
// fara pauza intre faze, CONNECT si DISCONNECT ar intra in acelasi frame si
// evenimentul de CONNECT s-ar pierde (handler-ul suprascrie event-ul).
static volatile bool s_reenumReq   = false;
static uint8_t       s_reenumPhase = 0;
static uint32_t      s_reenumAt    = 0;

void dut_host_force_reenumerate()   // sigur de pe core0
{
    s_reenumReq = true;
}

static void pumpReenum()
{
    if (s_reenumReq && s_reenumPhase == 0) {
        s_reenumReq = false;
        root_port_t* root = &pio_usb_root_port[0];
        if (root->initialized && root->connected) {
            root->ints |= PIO_INTS_DISCONNECT;
            s_reenumPhase = 1;
            s_reenumAt    = millis() + 50;
            Serial.println("USB HOST: detach fortat (re-enumerare)");
        }
        return;
    }

    if (s_reenumPhase == 1 && (int32_t)(millis() - s_reenumAt) > 0) {
        s_reenumPhase = 0;
        root_port_t* root = &pio_usb_root_port[0];
        root->connected = false;
        root->suspended = false;
        Serial.println("USB HOST: port eliberat, astept re-atasarea");
    }
}

static bool onMscXferComplete(uint8_t dev_addr,
                              tuh_msc_complete_data_t const* cb_data)
{
    (void)dev_addr;
    bool ok = (NULL != cb_data) && (NULL != cb_data->csw) &&
              (cb_data->csw->status == 0) &&
              (cb_data->csw->data_residue == 0);
    s_xferOk = ok;
    s_xferComplete = true;
    return true;
}

bool dut_host_available() { return true; }

// ---------------------------------------------------------------------------
// core1: initializare host
// ---------------------------------------------------------------------------

void dut_host_core1_setup()
{
    uint32_t cpu_hz = clock_get_hz(clk_sys);
    if (cpu_hz % 12000000UL != 0) {
        Serial.printf("USB HOST: EROARE clk_sys=%lu Hz - trebuie multiplu de 12 MHz!\n",
                      (unsigned long)cpu_hz);
    }

    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp     = PIN_USB_HOST_DP;
    pio_cfg.pio_tx_num = 1;   // PIO1: SDIO (carlk3) aloca din PIO0
    pio_cfg.pio_rx_num = 1;
    pio_cfg.tx_ch      = 9;   // DMA ch9, ferit de claim-urile SDIO

    // Fara alarm pool implicit: frame-ul USB (SOF + connection_check +
    // transferuri) este pasat manual din loop1(), la ~1 kHz.
    pio_cfg.skip_alarm_pool = true;

    USBHost.configure_pio_usb(1, &pio_cfg);
    USBHost.begin(1);
    SerialHost.begin(115200);

    Serial.printf("USB HOST: activ pe GP%u/GP%u (PIO1), clk_sys=%lu MHz\n",
                  PIN_USB_HOST_DP, PIN_USB_HOST_DP + 1,
                  (unsigned long)(cpu_hz / 1000000UL));
}

// ---------------------------------------------------------------------------
// core1: pump principal - frame USB + task + munca postata de core0
// ---------------------------------------------------------------------------

// Asambleaza octetii primiti pe CDC in linii, in bufferul partajat.
static void pumpCdcRx()
{
    if (s_link != DutLink::CDC || !SerialHost.connected())
        return;

    // Daca core0 nu a consumat inca linia anterioara, nu suprascriem -
    // DUT-ul raspunde cu o singura linie per comanda, nu curgem risc.
    while (SerialHost.available()) {
        char c = (char)SerialHost.read();

        if (c == '\n' || c == '\r') {
            if (s_rxAsmLen > 0) {
                // Vizibilitate diagnostica: TOT ce trimite DUT-ul apare in
                // log, chiar daca linia anterioara nu a fost inca consumata
                // (in WAIT_MSC nu exista consumator - markerii DBG de la
                // DUT ar fi fost aruncati silentios altfel).
                s_rxAssembling[s_rxAsmLen] = '\0';
                Serial.printf("DUT RX: %s\n", s_rxAssembling);
                if (s_rxLen == 0) {
                    memcpy(s_rxLine, s_rxAssembling, s_rxAsmLen + 1);
                    __sync_synchronize();
                    s_rxLen = s_rxAsmLen;   // publicare: ultima scriere
                }
                s_rxAsmLen = 0;
            }
        } else if (s_rxAsmLen < sizeof(s_rxAssembling) - 1) {
            s_rxAssembling[s_rxAsmLen++] = c;
        }
    }
}

// Trimite comanda postata de core0 (o data, la pending).
static void pumpCdcTx()
{
    if (!s_txPending)
        return;

    if (s_link == DutLink::CDC && SerialHost.connected()) {
        SerialHost.write((const uint8_t*)s_txLine, strlen(s_txLine));
        SerialHost.write("\r\n");
        SerialHost.flush();
    }
    __sync_synchronize();
    s_txPending = false;
}

// Executa cererea MSC postata de core0 (blocant pe core1 pana la completare).
static void pumpMsc()
{
    if (!s_mscReq)
        return;

    bool ok = false;
    if (s_mscDevAddr != 0 && NULL != s_mscBuf && s_mscBlocks > 0) {
        s_xferComplete = false;
        s_xferOk       = false;
        __sync_synchronize();

        if (tuh_msc_write10(s_mscDevAddr, 0 /* LUN */, (const void*)s_mscBuf,
                            s_mscLba, s_mscBlocks, onMscXferComplete, 0)) {
            uint32_t deadline = millis() + 5000;
            while (!s_xferComplete) {
                if ((int32_t)(millis() - deadline) > 0 || s_mscDevAddr == 0)
                    break;
                // completarea vine din contextul acestui task (core1),
                // deci nu asteptam pasiv: continuam cu frame + task.
                static uint32_t f = 0;
                if ((uint32_t)(micros() - f) >= 1000) { f = micros(); pio_usb_host_frame(); }
                USBHost.task();
            }
            ok = s_xferComplete && s_xferOk;
        }
    }

    s_mscOk  = ok;
    __sync_synchronize();
    s_mscDone = true;
    s_mscReq  = false;
}

void dut_host_core1_loop()
{
    // Frame USB la ~1 kHz (SOF + connection_check + transferuri + IRQ host).
    // Fara aceasta bataie detach/attach nu sunt detectate.
    static uint32_t lastFrameUs = 0;
    uint32_t now = micros();
    if ((uint32_t)(now - lastFrameUs) >= 1000) {
        lastFrameUs = now;
        pio_usb_host_frame();
    }

    USBHost.task();

    pumpReenum();
    pumpCdcRx();
    pumpCdcTx();
    pumpMsc();
}

// ---------------------------------------------------------------------------
// Stare partajata (citire de pe core0)
// ---------------------------------------------------------------------------

DutLink dut_link_state()
{
    __sync_synchronize();
    return s_link;
}

uint8_t dut_msc_dev_addr()
{
    __sync_synchronize();
    return s_mscDevAddr;
}

// ---------------------------------------------------------------------------
// Transport CDC (core0 doar posteaza / consuma; executia e pe core1)
// ---------------------------------------------------------------------------

bool dut_cdc_connected()
{
    __sync_synchronize();
    return s_link == DutLink::CDC;
}

bool dut_cdc_write_line(const char* line)
{
    if (!dut_cdc_connected() || NULL == line)
        return false;
    if (s_txPending)
        return false;   // protocolul e serializat: o comanda in zbor

    strncpy(s_txLine, line, sizeof(s_txLine) - 1);
    s_txLine[sizeof(s_txLine) - 1] = '\0';
    __sync_synchronize();
    s_txPending = true;   // publicare: ultima scriere

    return true;
}

int dut_cdc_read_line(char* buf, size_t maxLen, uint32_t timeoutMs)
{
    if (NULL == buf || maxLen < 2)
        return -1;

    uint32_t deadline = millis() + timeoutMs;

    while ((int32_t)(deadline - millis()) > 0) {
        if (s_link == DutLink::NONE) {
            return -1;   // DUT deconectat
        }

        if (s_rxLen > 0) {
            __sync_synchronize();
            size_t n = s_rxLen;
            if (n > maxLen - 1)
                n = maxLen - 1;
            memcpy(buf, s_rxLine, n);
            buf[n] = '\0';
            s_rxLen = 0;   // consumat
            return (int)n;
        }

        yield();
    }

    buf[0] = '\0';
    return 0;
}

void dut_cdc_flush_rx()
{
    s_rxLen    = 0;
    s_rxAsmLen = 0;
    __sync_synchronize();
}

// ---------------------------------------------------------------------------
// Transport MSC (core0 posteaza cererea; core1 o executa si raporteaza)
// ---------------------------------------------------------------------------

bool dut_msc_write_blocks(uint32_t lba, const uint8_t* buf, uint16_t blockCount)
{
    if (0 == dut_msc_dev_addr() || NULL == buf || 0 == blockCount)
        return false;
    if (s_mscReq)
        return false;   // un singur transfer in zbor

    s_mscLba    = lba;
    s_mscBlocks = blockCount;
    s_mscBuf    = buf;
    s_mscDone   = false;
    __sync_synchronize();
    s_mscReq = true;

    uint32_t deadline = millis() + 6000;
    while (!s_mscDone) {
        if ((int32_t)(millis() - deadline) > 0) {
            Serial.println("USB HOST: timeout la cererea MSC");
            s_mscReq = false;
            return false;
        }
        yield();
    }

    __sync_synchronize();
    return s_mscOk;
}

// ---------------------------------------------------------------------------
// Callback-uri TinyUSB host (executate pe core1, din contextul task()-ului)
// ---------------------------------------------------------------------------

extern "C" {

void tuh_mount_cb(uint8_t daddr)
{
    Serial.printf("USB HOST: device enumerat (adresa %u)\n", (unsigned)daddr);
}

void tuh_umount_cb(uint8_t daddr)
{
    Serial.printf("USB HOST: device demontat (adresa %u)\n", (unsigned)daddr);
}

void tuh_cdc_mount_cb(uint8_t idx)
{
    SerialHost.mount(idx);
    __sync_synchronize();
    s_link = DutLink::CDC;
    Serial.println("USB HOST: DUT conectat (CDC)");
}

void tuh_cdc_umount_cb(uint8_t idx)
{
    SerialHost.umount(idx);
    dut_protocol_on_disconnect();
    __sync_synchronize();
    s_link = DutLink::NONE;
    Serial.println("USB HOST: DUT deconectat (CDC)");
}

void tuh_msc_mount_cb(uint8_t dev_addr)
{
    s_mscDevAddr = dev_addr;
    __sync_synchronize();
    s_link = DutLink::MSC;

    static scsi_read_capacity10_resp_t capacity_resp;
    tuh_msc_read_capacity(dev_addr, 0 /* LUN */, &capacity_resp,
                          onMscXferComplete, 0);
    Serial.println("USB HOST: volum MSC montat (bootloader DUT)");
}

void tuh_msc_umount_cb(uint8_t dev_addr)
{
    (void)dev_addr;
    s_mscDevAddr = 0;
    __sync_synchronize();
    s_link = DutLink::NONE;
    Serial.println("USB HOST: volum MSC demontat");
}

} // extern "C"

#endif // SERVICEBOX_WAVESHARE
