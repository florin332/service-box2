#ifndef BATTERY_CHECK_VIEW_H
#define BATTERY_CHECK_VIEW_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

#include "UiLayout.h"
#include "UiTheme.h"

// ============================================================================
// BatteryCheckView - renderer PASIV pentru pagina Battery Check.
//
// Deseneaza exclusiv ce primeste in BatteryCheckViewModel. Nu cunoaste
// bateria, pragurile, timeout-urile sau faza de clipit - toate vin
// rezolvate din stratul superior (deocamdata BatteryCheckScreen; la pasul
// Navigation, din politica de navigatie).
// ============================================================================

struct BatteryCheckViewModel {
    float       voltageV;
    uint8_t     percent;
    uint16_t    indicatorColor;     // culoare indicator, rezolvata upstream
    const char* chargeText;         // nullptr = ascuns; "CHARGING"/"CHARGED"
    uint16_t    chargeColor;
    bool        standbyIndicatorOn; // faza de clipit, rezolvata upstream
};

class BatteryCheckView {
public:
    explicit BatteryCheckView(Adafruit_GFX* gfx);

    // Continut complet: titlu + tensiune + stare incarcare + indicator.
    void renderMain(const BatteryCheckViewModel& m);

    // Doar indicatorul (punctul colorat) - redesenare la schimbare de nivel.
    void renderIndicator(const BatteryCheckViewModel& m);

    // Continut standby: indicator (aprins/stins conform modelului) +
    // tensiunea afisata permanent deasupra.
    void renderStandby(const BatteryCheckViewModel& m);

    // Sterge tot ecranul (intrare in standby / iesire completa).
    void clearAll();

private:
    Adafruit_GFX* _gfx;

    bool     _layoutReady;
    UiLayout _layout;
    int16_t  _indicatorRadius;
    int16_t  _indicatorX;
    int16_t  _indicatorY;

    void ensureLayout();
};

#endif // BATTERY_CHECK_VIEW_H
