#ifndef PAGE_NAVIGATOR_H
#define PAGE_NAVIGATOR_H

#include <Arduino.h>
#include "battery/IBatteryProvider.h"
#include "UiPageId.h"
#include "UiEvents.h"

class StartupScreen;
class BatteryCheckScreen;
class SdCheckScreen;
class DashboardScreen;
class FlashDutScreen;

// ============================================================================
//
// Responsabilitati:
//   - pagina curenta + tranzitiile (conform regulilor din NavigationPolicy)
//   - timeout/standby Battery Check (5s fara touch -> standby; touch ->
//     reaprinde si reseteaza timerul; clipitul indicatorului in standby)
//   - avansarea automata Battery Check -> START (doar daca politica permite)
//   - CRITICAL forteaza Battery Check din orice pagina (+ eveniment
//     FORCED_LOW_POWER catre Actions)
//   - gating START: emite START_CONFIRMED numai daca politica permite
//   - navigarea din DASHBOARD: FLASH -> Flash DUT, SVC OUT -> START
//   - din FLASH_DUT: BACK -> DASHBOARD (sesiunea de service ramane
//     activa); terminarea flash-ului -> START (panelul a reboot-at)
//   - timp minim de stationare pe pagina (PAGE_MIN_DWELL_MS): atingerile
//     sunt ignorate imediat dupa o tranzitie, pentru a nu activa din
//     greseala butoane de pe pagina noua care se suprapun cu cele de pe
//     pagina anterioara
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
                  SdCheckScreen& sdCheck, DashboardScreen& dashboard,
                  FlashDutScreen& flashDut, IBatteryProvider& battery);

    // Intrarea formala in pagina initiala (BATTERY_CHECK). Se apeleaza la
    // finalul setup(): reseteaza timpii de activitate/stationare, astfel
    // timeout-ul de 5s si dwell-ul pornesc din momentul in care pagina
    // devine efectiv vizibila (setup-ul poate dura >5s: BAT KEY 3s +
    // SD self-test + mount), nu din momentul constructii.
    void begin();

    // O iteratie de loop: proceseaza inputul si aplica regulile.
    void update(int16_t touchX, int16_t touchY, bool touched);

    // Deseneaza pagina curenta (partial/full, dupa flag-uri).
    void render();

    // Consuma urmatorul eveniment emis (NONE daca coada e goala).
    UiEvent pollEvent();

    UiPageId currentPage() const { return _page; }

private:
    StartupScreen&       _startup;
    BatteryCheckScreen&  _batteryCheck;
    SdCheckScreen&       _sdCheck;
    DashboardScreen&     _dashboard;
    FlashDutScreen&      _flashDut;
    IBatteryProvider&    _battery;

    UiPageId      _page;
    bool          _pageNeedsRender;
    unsigned long _pageEnteredMs;  // millis() la intrarea pe pagina curenta

    // Coada de evenimente (capacitate 4 - suficient pentru o iteratie)
    UiEvent _events[4];
    uint8_t _evHead;
    uint8_t _evTail;

    // ---- Politica de standby Battery Check (mutata din BatteryCheckScreen) --
    bool          _screenOn;
    unsigned long _lastActivityMs;
    unsigned long _standbyBlinkTimerMs;
    bool          _standbyBlinkOn;
    bool          _standbyIndicatorVisible;

    static constexpr unsigned long SCREEN_ON_TIMEOUT_MS  = 5000UL;
    static constexpr unsigned long STANDBY_BLINK_OFF_MS  = 2000UL;
    static constexpr unsigned long STANDBY_BLINK_ON_MS   = 1000UL;
    // Timp minim de stationare pe o pagina: touch-ul este ignorat in
    // primele 1.5s dupa tranzitie (protectie la suprapunerea butoanelor).
    static constexpr unsigned long PAGE_MIN_DWELL_MS     = 1500UL;

    void enterPage(UiPageId page);
    void pushEvent(UiEvent ev);

    void updateBatteryCheckPage(int16_t x, int16_t y, bool touched);
    void updateStartPage(int16_t x, int16_t y, bool touched);
    void updateDashboardPage(int16_t x, int16_t y, bool touched);
    void updateSdCheckPage(int16_t x, int16_t y, bool touched);
    void updateFlashDutPage(int16_t x, int16_t y, bool touched);

    void enterStandby();
    void wakeFromStandby();
    bool updateStandbyBlinkPhase(); // true cand s-a schimbat faza de clipit
};

#endif // PAGE_NAVIGATOR_H
