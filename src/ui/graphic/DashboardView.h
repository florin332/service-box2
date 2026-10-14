#ifndef DASHBOARD_VIEW_H
#define DASHBOARD_VIEW_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

#include "UiLayout.h"
#include "UiTheme.h"

// ============================================================================
// DashboardView - renderer PASIV pentru pagina DASHBOARD (meniul de service).
//
// Pagina are 4 butoane intr-o grila 2x2: FLASH, TEST, INFO, SVC OUT,
// plus o linie de telemetrie INA219 (V/I) sub titlu. Deseneaza exclusiv ce
// primeste in DashboardViewModel; nu citeste senzori si nu decide stari.
// ============================================================================

// Indecsi butoane (folositi si de screen pentru hit-test).
enum DashboardButton : uint8_t {
    DASH_BTN_FLASH = 0,
    DASH_BTN_TEST  = 1,
    DASH_BTN_INFO  = 2,
    DASH_BTN_SVC_OUT = 3,
    DASH_BTN_COUNT = 4
};

struct DashboardViewModel {
    bool  ina219Available;
    float busVoltageV;
    float currentMa;
};

class DashboardView {
public:
    explicit DashboardView(Adafruit_GFX* gfx);

    // Desenare completa a paginii (fundal + titlu + telemetrie + butoane).
    void renderFull(const DashboardViewModel& m);

    // Redesenare partiala a liniei de telemetrie (anti-flicker).
    void drawTelemetry(const DashboardViewModel& m);

    // Zona de hit-test a unui buton (calculata din layout).
    const UiRect& buttonRect(uint8_t idx) const { return _btnRects[idx]; }

private:
    Adafruit_GFX* _gfx;

    bool     _layoutReady;
    UiLayout _layout;
    UiRect   _btnRects[DASH_BTN_COUNT];
    int16_t  _btnRadius;

    void ensureLayout();
    void drawButton(uint8_t idx, const char* label);
};

#endif // DASHBOARD_VIEW_H
