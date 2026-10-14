#ifndef DUT_HOST_H
#define DUT_HOST_H

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// dut_host - USB host pe PIO (GP0=D+, GP1=D-) catre portul USB nativ al
// DUT-ului (panou Marble Pico), in paralel cu USB-ul device nativ al
// ServiceBox-ului (Serial debug + UF2 propriu - nemodificat).
//
// Arhitectura (pattern-ul DualRole din Adafruit TinyUSB):
//   core1: doar USBHost.task() + callback-urile de mount/unmount
//          (setup1/loop1 din main.cpp apeleaza dut_host_core1_setup/loop)
//   core0: tot restul - protocolul serial si scrierea MSC sunt apelate din
//          contextul loop()-ului (exact ca in exemplele Adafruit).
//
// Starea de legatura este partajata prin campuri volatile + bariere DMB;
// singurul writer este core1 (callback-uri), core0 doar citeste.
//
// Pe build-ul Marble (fara host PIO) API-ul exista ca stub-uri inofensive,
// ca restul codului (UI) sa ramana neconditionat.
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

// ---- transport CDC (apelat de pe core0) ----
bool dut_cdc_connected();
// Scrie o linie catre DUT (adauga terminatorul CRLF). false daca nu e CDC.
bool dut_cdc_write_line(const char* line);
// Citeste o linie completa (fara terminator), max maxLen-1 caractere.
// Returneaza: >0 lungimea liniei, 0 = timeout fara linie completa,
// -1 = DUT deconectat in timpul citirii.
int dut_cdc_read_line(char* buf, size_t maxLen, uint32_t timeoutMs);

// Goleste FIFO-ul de receptie (resturi de linii, ex. iesirile RAW dintr-un
// mod analyze ramas activ pe DUT). Apelata inainte de fiecare comanda.
void dut_cdc_flush_rx();

// ---- transport MSC (apelat de pe core0) ----
uint8_t dut_msc_dev_addr();   // 0 daca nu exista volum MSC montat
// Scrie `blockCount` sectoare de 512 B la LBA dat (blocant; completarea
// transferului este servita de USBHost.task() de pe core1).
bool dut_msc_write_blocks(uint32_t lba, const uint8_t* buf, uint16_t blockCount);

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

#endif // SERVICEBOX_WAVESHARE

#endif // DUT_HOST_H
