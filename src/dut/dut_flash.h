#ifndef DUT_FLASH_H
#define DUT_FLASH_H

#include <stdint.h>

// ============================================================================
// dut_flash - flasher UF2 pentru DUT (panou Marble Pico) prin host-ul USB.
//
// Flux (masina de stari, pasata din loop() pe core0):
//   IDENTIFY        -> retine identitatea curenta (informativa; handshake-ul
//                      real al sesiunii se face la intrarea in START)
//   ENTER_BOOTSEL   -> mb_02 (comanda din firmware-ul DUT); daca DUT-ul
//                      nu o recunoaste (firmware vechi), se asteapta oricum
//                      MSC - utilizatorul poate apasa BOOTSEL fizic
//   WAIT_MSC        -> asteapta volumul "RPI-RP2" (timeout -> FAILED)
//   WRITING         -> streaming: citeste chunk-uri .uf2 de pe SD si scrie
//                      sectoare pe MSC (bootrom-ul scaneaza magic-ul UF2,
//                      LBA-ul nu conteaza)
//   DONE            -> toate blocurile scrise; bootloader-ul reboot-eaza
//                      DUT-ul singur si box-ul revine la START (urmatorul
//                      handshake - cel al Start-ului - descopera noul
//                      firmware; NU exista verify/reconnect aici)
//   FAILED          -> terminal pana la dut_flash_init()
//
// Sursa .uf2: cardul SD (bsp_sdcard), validata la start (magic + family ID
// RP2040 0xE48BFF56, dimensiune multiplu de 512).
// ============================================================================

enum class DutFlashPhase : uint8_t {
    IDLE = 0,
    IDENTIFY,
    ENTER_BOOTSEL,
    WAIT_MSC,
    WRITING,
    DONE,
    FAILED
};

struct DutFlashStatus {
    DutFlashPhase phase;
    char     fileName[24];         // fisierul selectat
    uint32_t totalBlocks;          // sectoare de 512 B de scris
    uint32_t writtenBlocks;        // progres
    char     identityBefore[24];   // identitatea de la intrarea in flux
    char     error[64];            // motivul la FAILED (gol altfel)
};

// Reset la IDLE (apelata la intrarea in pagina Flash DUT).
void dut_flash_init();

// Valideaza fisierul .uf2 de pe SD si porneste fluxul.
// Returneaza false imediat daca validarea esueaza (motivul in status().error)
// sau daca DUT-ul nu este pe CDC.
bool dut_flash_start(const char* uf2Name);

// Varianta "blank DUT": panou nou/neprogramat care s-a enumerat DIRECT ca
// volum MSC (RPI-RP2), fara CDC si fara identitate. Sare peste IDENTIFY si
// ENTER_BOOTSEL si intra direct in WAIT_MSC (MSC-ul este deja, de regula,
// prezent). Returneaza false daca nu exista inca volum MSC.
bool dut_flash_start_direct(const char* uf2Name);

// Un pas al masinii de stari. Apelata din loop() (core0), la fiecare iteratie.
void dut_flash_update();

// True cat timp fluxul este in desfasurare (BACK e blocat in UI).
bool dut_flash_busy();

const DutFlashStatus& dut_flash_status();

#endif // DUT_FLASH_H
