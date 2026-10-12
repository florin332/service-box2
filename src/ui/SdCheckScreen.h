#ifndef SD_CHECK_SCREEN_H
#define SD_CHECK_SCREEN_H

#include <Arduino.h>
#include "graphic/SdCheckView.h"

// ============================================================================
// Pagina UI "SD Check".
//
// Afiseaza starea cardului SD (montat/absent, capacitati) si lista fisierelor
// din radacina. Datele sunt citite O SINGURA DATA la intrarea in pagina
// (init); cardul nu este re-citit periodic - ecranul este static pana la
// BACK. Nu exista mount/unmount/format din UI: cardul este pregatit pe PC.
//
// Singurul control: butonul BACK (activare la eliberare, ca butonul START).
// ============================================================================

class SdCheckScreen {
public:
    explicit SdCheckScreen(Adafruit_GFX* tftInstance);

    // Citeste cardul si redeseneaza pagina completa (la intrarea in pagina).
    void init();

    // Interactiune tactila. Returneaza true o data, la eliberarea de pe BACK.
    bool update(int touchX, int touchY, bool isTouched);

private:
    SdCheckView _view;      // renderer pasiv
    bool        _backHeld;  // degetul e apasat in zona BACK

    void fillModel(SdCheckViewModel& m);
};

#endif // SD_CHECK_SCREEN_H
