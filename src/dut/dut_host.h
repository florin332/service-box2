#ifndef DUT_HOST_H
#define DUT_HOST_H

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// dut_host - USB host pe PIO (GP0=D+, GP1=D-) catre portul USB nativ al
// DUT-ului (panou Marble Pico), in paralel cu USB-ul device nativ al
// ServiceBox-ului (Serial debug + UF2 propriu - nemodificat).
//
// Arhitectura (thread-safety TinyUSB):
//   core1: TOT ce atinge stack-ul host - USBHost.task(), frame-ul PIO,
//          scrierile/citirile CDC si transferurile MSC. TinyUSB NU este
//          thread-safe cross-core: Adafruit_USBH_CDC::write() apeleaza
//          tuh_task() intern, deci niciun apel USB nu are voie din core0.
//   core0: comenzi catre DUT prin mailbox (dut_cdc_write_line pune in coada;
//          loop1() o goleste). Starea (link, rx, completari MSC) este
//          partajata prin campuri volatile + bariere.
//
// Pe build-ul Marble (fara host PIO) API-ul exista ca stub-uri inofensive.
// ============================================================================

// Starea legaturii cu DUT-ul.
enum class DutLink : uint8_t {
    NONE = 0,   // nimic conectat
    CDC,        // DUT in aplicatie (protocolul serial de service disponibil)
    MSC         // DUT in BOOTSEL (volum de flash "RPI-RP2" disponibil)
};

#if defined(SERVICEBOX_WAVESHARE)

// True pe hardware cu host PIO (Waveshare). UI-ul foloseste asta pentru
// activarea butonului FLASH.
bool dut_host_available();

// Apelate din setup1()/loop1() (main.cpp le gate-ueste pe SERVICEBOX_WAVESHARE).
void dut_host_core1_setup();
void dut_host_core1_loop();

// ---- stare (citire sigura de pe core0) ----
DutLink dut_link_state();

// ---- transport CDC ----
bool dut_cdc_connected();
// Pune o linie in coada de TX (terminatorul CRLF este adaugat la executie,
// pe core1). core0 doar posteaza; returneaza false daca nu e CDC sau coada
// e plina. Apelantul asteapta completarea prin dut_cdc_read_line().
bool dut_cdc_write_line(const char* line);
// Citeste o linie completa din FIFO-ul umplut de core1 (fara terminator),
// max maxLen-1 caractere. core0 doar consuma - nu atinge stack-ul USB.
// Returneaza: >0 lungimea, 0 = timeout, -1 = DUT deconectat.
int dut_cdc_read_line(char* buf, size_t maxLen, uint32_t timeoutMs);

// Goleste bufferul RX partajat (de pe core0; resturi de linii inainte de
// fiecare comanda noua).
void dut_cdc_flush_rx();

// ---- transport MSC (apelat de pe core0) ----
uint8_t dut_msc_dev_addr();   // 0 daca nu exista volum MSC montat
// Scrie `blockCount` sectoare de 512 B la LBA dat (blocant; completarea
// transferului este servita de USBHost.task() de pe core1).
bool dut_msc_write_blocks(uint32_t lba, const uint8_t* buf, uint16_t blockCount);

// Fortare re-enumerare a portului host (core0 cere, core1 executa).
// Folosita dupa SRV BOOTSEL cand DUT-ul s-a resetat, dar detach-ul electric
// nu a fost detectat (fereastra SE0 prea scurta la reset rapid): publica un
// DISCONNECT software catre TinyUSB, apoi elibereaza portul pentru re-attach.
void dut_host_force_reenumerate();

#else // !SERVICEBOX_WAVESHARE - stub-uri

inline bool dut_host_available() { return false; }
inline void dut_host_core1_setup() {}
inline void dut_host_core1_loop() {}
inline DutLink dut_link_state() { return DutLink::NONE; }
inline bool dut_cdc_connected() { return false; }
inline bool dut_cdc_write_line(const char*) { return false; }
inline int dut_cdc_read_line(char*, size_t, uint32_t) { return -1; }
inline void dut_cdc_flush_rx() {}
inline uint8_t dut_msc_dev_addr() { return 0; }
inline bool dut_msc_write_blocks(uint32_t, const uint8_t*, uint16_t) { return false; }
inline void dut_host_force_reenumerate() {}

#endif // SERVICEBOX_WAVESHARE

#endif // DUT_HOST_H
