#ifndef TOUCH_CALIBRATION_H
#define TOUCH_CALIBRATION_H

#include <stdint.h>

// ============================================================================
// TouchCalibration - fluxul de calibrare a axelor touch, modul separat.
//
// Are prioritate fata de UI-ul aplicatiei: cat timp e activ, consuma
// iteratiile de loop si nu lasa navigatorul sa ruleze.
//
// Waveshare: 2 etape (swipe orizontal -> swapXY/invX, swipe vertical -> invY)
// Marble:    3 etape (swipe orizontal, swipe vertical, 4 puncte colturi)
//
// La finalul fiecarei etape salvate: EEPROM.commit() + reboot.
// ============================================================================

class TouchCalibration {
public:
    // Incarca flag-urile din EEPROM si le aplica pe bsp_touch.
    // Apelata in setup(), inainte de orice desenare.
    void begin();

    // true daca fluxul de calibrare este activ (stage != complet).
    bool isActive() const;

    // Deseneaza ecranul corespunzator stadiului curent (la intrare in setup).
    void renderCurrent();

    // O iteratie de loop in mod calibrare. Returneaza true daca a consumat
    // iteratia (main trebuie sa sara peste UI-ul aplicatiei).
    bool update();

    // Verifica butonul hardware de recalibrare (2s hold).
    // Apelata doar cand NU e activ fluxul de calibrare (stage == complet).
    void checkRecalibButton();

private:
    // Desenare ecrane (in implementare, per target)
    void renderStage();
};

#endif // TOUCH_CALIBRATION_H
