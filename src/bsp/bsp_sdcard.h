#ifndef BSP_SDCARD_H
#define BSP_SDCARD_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// bsp_sdcard - factory unic pentru card SD, per target.
//
// Singurul punct din proiect care cunoaste interfata concreta SD:
//   SERVICEBOX_WAVESHARE -> SDIO 4-bit (PIO), GP19..GP24, FatFS (carlk3)
//   SERVICEBOX_MARBLE    -> SPI0 (bit-bang), GP16..GP19 + CS GP17, FatFS (carlk3)
//
// Restul codului (UI, actiuni) foloseste exclusiv acest API comun -
// nu exista nicio dependenta directa de ff.h / sd_card.h in afara de
// bsp_sdcard.cpp.
//
// Nota: initializarea NU se face in setup(); se face lazy, la primul
// bsp_sdcard_mount(), pentru a nu bloca boot-ul daca lipseste cardul.
// ============================================================================

// Initializeaza driver-ul SD (configuratie hardware per target).
// Sigur de apelat de mai multe ori (intern se executa o singura data).
// Returneaza true daca driver-ul a acceptat configuratia.
bool bsp_sdcard_init();

// Monteaza volumul FAT implicit ("0:").
// Returneaza true daca cardul este prezent, initializat si montat.
bool bsp_sdcard_mount();

// Demonteaza volumul si elibereaza resursele FatFS.
void bsp_sdcard_unmount();

// true daca volumul este montat curent.
bool bsp_sdcard_is_mounted();

// Capacitatea totala a cardului, in MB (sectoare * 512 / 1024 / 1024).
// Returneaza 0 daca nu este montat sau la eroare.
uint32_t bsp_sdcard_size_mb();

// Spatiu liber, in MB. Returneaza 0 daca nu este montat sau la eroare.
uint32_t bsp_sdcard_free_mb();

// Test functional: scrie o linie de log in "svcbox_sd_test.txt" si
// o citeste inapoi pentru verificare. Afiseaza rezultatul pe Serial.
// Returneaza true daca scrierea + citirea au reusit.
bool bsp_sdcard_self_test();

// Listeaza fisierele (nu directoarele) din radacina cardului.
// Numele sunt copiate in `names` (LFN daca e activat in FatFS, altfel 8.3).
// Returneaza numarul de fisiere copiate (max `maxNames`).
// Returneaza 0 daca cardul nu este montat sau la eroare.
uint16_t bsp_sdcard_list_root(char names[][24], uint16_t maxNames);

#endif // BSP_SDCARD_H
