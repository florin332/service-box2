// ============================================================================
// dut_host.cpp - implementare USB host PIO (doar SERVICEBOX_WAVESHARE).
//
// Resurse folosite (fara conflicte cu restul placii):
//   - PIO1 integral (3 SM: TX/RX/EOP) - PIO0 ramane pentru SDIO (card SD)
//   - DMA channel 9 - departe de claim-urile timpurii ale SDIO-ului
//   - GP0 (D+) / GP1 (D-), fara VBUS in cablu (DUT alimentat separat)
//
// Ceas: clk_sys TREBUIE multiplu de 12 MHz (144 MHz in platformio.ini);
// la setup se verifica si se raporteaza pe Serial daca nu e respectat.
// ============================================================================

#include "dut_host.h"
#include "dut_protocol.h"

#if defined(SERVICEBOX_WAVESHARE)

#include <Arduino.h>
#include <hardware/clocks.h>

#include <Adafruit_TinyUSB.h>
#include "pio_usb.h"

// GP0 = D+, GP1 = D- (D- trebuie sa fie pinul consecutiv urmator).
static constexpr uint8_t PIN_USB_HOST_DP = 0;

static Adafruit_USBH_Host USBHost;
static Adafruit_USBH_CDC  SerialHost;

// Stare partajata: scrisa din callback-uri (core1), citita de core0.
static volatile DutLink s_link       = DutLink::NONE;
static volatile uint8_t s_mscDevAddr = 0;

// Completarea transferului MSC asincron (tuh_msc_write10/read_capacity):
// callback-ul ruleaza pe core1, in contextul USBHost.task().
static volatile bool s_xferComplete = false;
static volatile bool s_xferOk       = false;

static bool onMscXferComplete(uint8_t dev_addr,
                              tuh_msc_complete_data_t const* cb_data)
{
    (void)dev_addr;
    // CSW: status 0 = success, data_residue 0 = toti octetii ceruti au
    // fost procesati (scriem mereu buffer complet, deci orice reziduu
    // inseamna transfer incomplet).
    bool ok = (NULL != cb_data) && (NULL != cb_data->csw) &&
              (cb_data->csw->status == 0) &&
              (cb_data->csw->data_residue == 0);
    s_xferOk = ok;
    s_xferComplete = true;
    return true;  // transferul este considerat finalizat de driver
}

bool dut_host_available() { return true; }

// ---------------------------------------------------------------------------
// core1: initializare host
// ---------------------------------------------------------------------------

void dut_host_core1_setup()
{
    uint32_t cpu_hz = clock_get_hz(clk_sys);
    if (cpu_hz % 12000000UL != 0) {
        // PIO-USB nu poate genera timing-ul USB FS la acest ceas.
        Serial.printf("USB HOST: EROARE clk_sys=%lu Hz - trebuie multiplu de 12 MHz!\n",
                      (unsigned long)cpu_hz);
    }

    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp     = PIN_USB_HOST_DP;
    pio_cfg.pio_tx_num = 1;   // PIO1: SDIO (carlk3) aloca din PIO0
    pio_cfg.pio_rx_num = 1;
    pio_cfg.tx_ch      = 9;   // DMA ch9, ferit de claim-urile SDIO

    USBHost.configure_pio_usb(1, &pio_cfg);

    // begin() pe core1: prelucrarea bit-bang (ISR/DMA) ramane pe core1,
    // core0 este liber pentru UI.
    USBHost.begin(1);
    SerialHost.begin(115200);

    Serial.printf("USB HOST: activ pe GP%u/GP%u (PIO1), clk_sys=%lu MHz\n",
                  PIN_USB_HOST_DP, PIN_USB_HOST_DP + 1,
                  (unsigned long)(cpu_hz / 1000000UL));
}

void dut_host_core1_loop()
{
    USBHost.task();
}

// ---------------------------------------------------------------------------
// Stare partajata
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
// Transport CDC (core0)
// ---------------------------------------------------------------------------

bool dut_cdc_connected()
{
    __sync_synchronize();
    return (s_link == DutLink::CDC) && SerialHost && SerialHost.connected();
}

bool dut_cdc_write_line(const char* line)
{
    if (!dut_cdc_connected() || NULL == line)
        return false;

    SerialHost.write((const uint8_t*)line, strlen(line));
    SerialHost.write("\r\n");
    SerialHost.flush();
    return true;
}

int dut_cdc_read_line(char* buf, size_t maxLen, uint32_t timeoutMs)
{
    if (!dut_cdc_connected() || NULL == buf || maxLen < 2)
        return -1;

    size_t n = 0;
    uint32_t deadline = millis() + timeoutMs;

    while ((int32_t)(deadline - millis()) > 0) {
        if (!dut_cdc_connected()) {
            return -1;
        }
        while (SerialHost.available()) {
            char c = (char)SerialHost.read();
            if (c == '\n' || c == '\r') {
                if (n > 0) {
                    buf[n] = '\0';
                    return (int)n;
                }
                // linie goala: ignorata
            } else if (n < maxLen - 1) {
                buf[n++] = c;
            }
        }
        delay(1);  // FIFO-ul este umplut de USBHost.task() de pe core1
    }

    // Timeout: linia partiala (daca exista) este aruncata - DUT-ul raspunde
    // cu linii complete si prompt, un rest fara terminator nu e utilizabil.
    buf[0] = '\0';
    return 0;
}

void dut_cdc_flush_rx()
{
    if (!dut_cdc_connected())
        return;
    while (SerialHost.available()) {
        (void)SerialHost.read();
    }
}

// ---------------------------------------------------------------------------
// Transport MSC (core0) - scriere raw de sectoare UF2
//
// Bootrom-ul RP2040 expune volumul "RPI-RP2" ca un FAT virtual: fiecare
// sector scris este scanat dupa magic-ul UF2 si programat in flash pe baza
// adresei din blocul UF2 - LBA-ul la care scriem NU conteaza. Scriem
// secvential de la un LBA in zona de date a discului.
// ---------------------------------------------------------------------------

bool dut_msc_write_blocks(uint32_t lba, const uint8_t* buf, uint16_t blockCount)
{
    uint8_t dev = dut_msc_dev_addr();
    if (0 == dev || NULL == buf || 0 == blockCount)
        return false;

    // Transfer asincron: completarea soseste prin callback (servita de
    // USBHost.task() de pe core1). Aici doar asteptam rezultatul.
    s_xferComplete = false;
    s_xferOk       = false;
    __sync_synchronize();

    if (!tuh_msc_write10(dev, 0 /* LUN */, (const void*)buf, lba, blockCount,
                         onMscXferComplete, 0 /* user arg */)) {
        return false;  // transferul nu a putut fi pornit
    }

    uint32_t deadline = millis() + 5000;  // 8 sectoare FS << 5s
    while (!s_xferComplete) {
        if ((int32_t)(millis() - deadline) > 0) {
            Serial.println("USB HOST: timeout la scrierea MSC");
            return false;
        }
        if (dut_msc_dev_addr() == 0) {
            return false;  // dispozitivul a disparut in timpul transferului
        }
        delay(1);
    }

    __sync_synchronize();
    return s_xferOk;
}

// ---------------------------------------------------------------------------
// Callback-uri TinyUSB host (executate pe core1, din contextul task()-ului)
// ---------------------------------------------------------------------------

extern "C" {

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

    // Capacitatea se citeste asincron; rezultatul nu e folosit de flasher
    // (scrierea UF2 nu depinde de geometria discului), dar citirea ei
    // "trezeste" complet pipeline-ul SCSI dupa montare.
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
