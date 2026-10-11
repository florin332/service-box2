#ifndef STARTUP_VIEW_H
#define STARTUP_VIEW_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

#include "UiLayout.h"
#include "UiTheme.h"

// ============================================================================
// StartupView - renderer PASIV pentru pagina START.
//
// Deseneaza exclusiv ce primeste in StartupViewModel. Nu cunoaste bateria,
// nu decide praguri/culori de nivel, nu stie pe ce display/board ruleaza.
// Geometria este derivata din dimensiunile display-ului la runtime.
//
// Culoarea indicatorului de baterie si textul butonului vin rezolvate
// din stratul superior (deocamdata StartupScreen; la pasul Navigation,
// din politica de navigatie).
// ============================================================================

struct StartupViewModel {
    float       batteryVoltageV;
    uint16_t    batteryColor;    // culoare indicator, rezolvata upstream
    const char* buttonText;      // "START" / "BATT LOW"
    uint32_t    sdFreeMB;
    uint32_t    sdTotalMB;
    bool        sdAvailable;
};

class StartupView {
public:
    explicit StartupView(Adafruit_GFX* gfx);

    // Desenare completa a paginii (fundal + titlu + buton + statusuri).
    void renderFull(const StartupViewModel& m);

    // Redesenari partiale (fara flicker: apelate doar la schimbari reale).
    void drawButton(const char* text);
    void drawBatteryInfo(const StartupViewModel& m);
    void updateStorage(uint32_t freeMB, uint32_t totalMB, bool cardAvailable);

    // Zona de hit-test a butonului (calculata din layout, nu hardcodata).
    const UiRect& buttonRect() const { return _btnRect; }

private:
    Adafruit_GFX* _gfx;

    bool     _layoutReady;
    UiLayout _layout;
    UiRect   _btnRect;
    int16_t  _btnRadius;
    int16_t  _dotRadius;

    void ensureLayout();
};

#endif // STARTUP_VIEW_H
