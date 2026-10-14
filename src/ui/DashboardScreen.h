#ifndef DASHBOARD_SCREEN_H
#define DASHBOARD_SCREEN_H

#include <Arduino.h>
#include "graphic/DashboardView.h"

// ============================================================================
// Pagina UI "Dashboard" - meniul principal de service (4 butoane 2x2).
//
//   FLASH   -> pagina Flash DUT (flash firmware pe panoul conectat)
//   TEST    -> placeholder (pipeline-ul de teste panou - pas viitor)
//   INFO    -> placeholder (informatii DUT/versiuni - pas viitor)
//   SVC OUT -> iesire din meniul de service, inapoi la pagina START
//
// Sub titlu afiseaza telemetria INA219 (V/I), reimprospatata periodic -
// continutul vechiului dashboard placeholder, pastrat aici.
//
// Activarea butoanelor este la eliberare, ca in restul UI-ului.
// ============================================================================

class DashboardScreen {
public:
    explicit DashboardScreen(Adafruit_GFX* tftInstance);

    // Reseteaza starea si deseneaza pagina completa (la intrarea in pagina).
    void init();

    enum class NavRequest : uint8_t { NONE, FLASH, TEST, INFO, SVC_OUT };

    // Interactiune tactila; returneaza cererea de actiune (sau NONE).
    NavRequest update(int touchX, int touchY, bool isTouched);

    // Redesenare completa (force) + telemetrie rate-limited la 500 ms.
    void render(bool forceRedraw = false);

private:
    DashboardView      _view;
    DashboardViewModel _model;

    bool          _isRendered;
    int8_t        _btnHeld;                  // index buton apasat, -1 = niciunul
    unsigned long _lastTelemetryMs;

    static constexpr unsigned long TELEMETRY_REFRESH_MS = 500UL;

    void readTelemetry();
    static bool hitTest(const UiRect& r, int x, int y);
};

#endif // DASHBOARD_SCREEN_H
