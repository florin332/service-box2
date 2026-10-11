#ifndef BSP_TOUCH_H
#define BSP_TOUCH_H

#include <stdint.h>

// ============================================================================
// bsp_touch - factory unic pentru touch, per target.
//
//   SERVICEBOX_WAVESHARE -> CST328 (I2C1, polling registri)
//   SERVICEBOX_MARBLE    -> XPT2046 (SPI1, magistrala comuna cu LCD)
//
// Coordonatele returnate de bsp_touch_read() sunt NORMALIZATE in pixeli
// de display: corectia de axe (swapXY/invX/invY) si maparea de calibrare
// sunt aplicate intern. UI-ul nu cunoaste controllerul de touch si nu
// aplica nicio corectie proprie.
// ============================================================================

// Initializare controller touch (I2C/CST328 sau SPI/XPT2046, per target).
void bsp_touch_init();

// O singura citire hardware per apel; valorile brute sunt cache-uite
// intern pentru bsp_touch_last_raw() (etapele de calibrare refolosesc
// aceeasi citire - evita dubla citire SPI/I2C per iteratie de loop).
// Returneaza true daca ecranul este atins; x/y = pixeli display normalizati.
bool bsp_touch_read(uint16_t& x, uint16_t& y);

// Ultimele valori brute citite de bsp_touch_read() - NU face acces hardware.
void bsp_touch_last_raw(int32_t& rx, int32_t& ry, int32_t& z, bool& pressed);

// Citire hardware bruta noua, fara nicio corectie.
// Waveshare: polling direct pe registri CST328 (folosit de measureSwipe).
// Marble: citire noua XPT2046 cu izolarea display-ului.
bool bsp_touch_poll_raw(int32_t& rx, int32_t& ry);

// Configurare corectie axe - apelata de modulul de calibrare
// (la incarcare din EEPROM si dupa fiecare etapa salvata).
void bsp_touch_set_correction(bool swapXY, bool invX, bool invY);

// Configurare mapare liniara raw -> pixeli (folosita doar pe Marble;
// pe Waveshare apelul este ignorat - CST328 livreaza direct pixeli).
void bsp_touch_set_mapping(int32_t xmin, int32_t xmax, int32_t ymin, int32_t ymax);

// Blocheaza pana la eliberarea completa a degetului de pe ecran.
void bsp_touch_wait_release();

#endif // BSP_TOUCH_H
