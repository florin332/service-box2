#ifndef PAGE_NAVIGATOR_H
#define PAGE_NAVIGATOR_H

#include <Arduino.h>
#include "battery/IBatteryProvider.h"
#include "UiPageId.h"
#include "UiEvents.h"

class StartupScreen;
class BatteryCheckScreen;

// ============================================================================
// PageNavigator - masina de stari a paginilor UI.
//
// Responsabilitati:
//   - pagina curenta + tranzitiile (conform regulilor din NavigationPolicy)
//   - timeout/standby Battery Check (5s fara touch -> standby; touch ->
//     reaprinde si reseteaza timerul; clipitul indicatorului in standby)
//   - avansarea automata Battery Check -> START (doar daca politica permite)
//   - CRITICAL forteaza Battery Check din orice pagina (+ eveniment
//     FORCED_LOW_POWER catre Actions)
//   - gating START: emite START_CONFIRMED numai daca politica permite
//   - detectia eliberarii de pe butonul BACK al dashboard-ului
//
// NU deseneaza nimic si NU executa actiuni: emite evenimente (UiEvent)
// catre main/Actions si ofera flag-uri de stare pentru desenare.
//
// Depende de Adafruit_GFX doar indirect, prin screen-uri (detinute in
// main.cpp) - navigatorul le comanda, nu le cunoaste continutul grafic.
// ============================================================================

class PageNavigator {
public:
    PageNavigator(StartupScreen& startup, BatteryCheckScreen& batteryCheck,
                  IBatteryProvider& battery);

    // O iteratie de loop: proceseaza inputul si aplica regulile.
    void update(int16_t touchX, int16_t touchY, bool touched);

    // Deseneaza pagina curenta (partial/full, dupa flag-uri).
    void render();

    // Consuma urmatorul eveniment emis (NONE daca coada e goala).
    UiEvent pollEvent();

    UiPageId currentPage() const { return _page; }

    // Refresh periodic valori dashboard (proxim; mutat la Actions la pasul 4).
    void dashboardUpdate();

private:
    StartupScreen&       _startup;
    BatteryCheckScreen&  _batteryCheck;
    IBatteryProvider&    _battery;

    UiPageId _page;
    bool     _pageNeedsRender;

    // Coada de evenimente (capacitate 4 - suficient pentru o iteratie)
    UiEvent _events[4];
    uint8_t _evHead;
    uint8_t _evTail;

    // Stare dashboard
    bool          _dashboardBaseDrawn;
    unsigned long _dashboardLastRefreshMs;

    // Tracking buton BACK (activare la eliberare, ca butonul START)
    bool _backWasHeld;

    // ---- Politica de standby Battery Check (mutata din BatteryCheckScreen) --
    bool          _screenOn;
    unsigned long _lastActivityMs;
    unsigned long _standbyBlinkTimerMs;
    bool          _standbyBlinkOn;
    bool          _standbyIndicatorVisible;

    static constexpr unsigned long SCREEN_ON_TIMEOUT_MS  = 5000UL;
    static constexpr unsigned long STANDBY_BLINK_OFF_MS  = 2000UL;
    static constexpr unsigned long STANDBY_BLINK_ON_MS   = 1000UL;
    static constexpr unsigned long DASHBOARD_REFRESH_MS  = 500UL;

    void enterPage(UiPageId page);
    void pushEvent(UiEvent ev);

    void updateBatteryCheckPage(int16_t x, int16_t y, bool touched);
    void updateStartPage(int16_t x, int16_t y, bool touched);
    void updateDashboardPage(int16_t x, int16_t y, bool touched);

    void enterStandby();
    void wakeFromStandby();
    bool updateStandbyBlinkPhase(); // true cand s-a schimbat faza de clipit

    bool dashboardBackHit(int16_t x, int16_t y, bool touched);
};

#endif // PAGE_NAVIGATOR_H
