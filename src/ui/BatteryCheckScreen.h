#ifndef BATTERY_CHECK_SCREEN_H
#define BATTERY_CHECK_SCREEN_H

#include <Arduino.h>
#include "battery/IBatteryProvider.h"
#include "graphic/BatteryCheckView.h"
#include "graphic/UiTheme.h"

// ============================================================================
// Pagină UI "Battery Check".
//
// Afișează nivelul bateriei și starea de încărcare. Pagina este exclusiv
// informativă: nu are butoane, controale sau gesturi touchscreen.
//
// Comportament:
//   - Rămâne activă 5 secunde după activare/reactivare.
//   - După timeout fără atingere, partea grafică principală se șterge și
//     rămâne un punct indicator colorat care clipește (2s stins, 1s aprins),
//     plus procentul bateriei afișat permanent deasupra indicatorului.
//   - O atingere reaprinde pagina și resetează timerul.
//   - Dacă Battery Manager raportează NORMAL și timeout-ul a expirat,
//     semnalează către main.cpp că trebuie avansat către pagina următoare (START).
//   - Dacă starea este LOW, CRITICAL sau CHARGING, rămâne în Battery Check
//     și NU avansează automat.
//
// Politica este aplicată pe baza stării discrete (NORMAL/LOW/CRITICAL/CHARGING)
// furnizate de IBatteryProvider, nu pe baza pragurilor locale.
//
// Depinde doar de IBatteryProvider, deci poate fi testată cu stub.
// ============================================================================

class BatteryCheckScreen {
public:
    BatteryCheckScreen(Adafruit_GFX* tft, IBatteryProvider& battery);

    void init();
    void render(bool forceRedraw = false);

    // Acces la view (folosit de PageNavigator pentru standby/clipit).
    BatteryCheckView& view() { return _view; }

    // Construieste modelul de vizualizare din starea curenta a bateriei
    // (public: il foloseste PageNavigator la redesenarile din standby).
    void fillModel(BatteryCheckViewModel& m) const;

private:
    BatteryCheckView _view; // renderer pasiv (deseneaza ce primeste)
    IBatteryProvider& _battery;

    bool _isRendered;

    unsigned long _lastBatteryUpdateMs;

    uint8_t  _displayedLevel;
    bool     _displayedCharging;

    static constexpr unsigned long BATTERY_UPDATE_MS = 1500UL;
};

#endif // BATTERY_CHECK_SCREEN_H
