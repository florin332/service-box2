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
//   - standby CHARGING (10s fara touch, din Battery Check sau START:
//     ecran negru + punct verde clipitor; atingerea revine la pagina
//     activa fara a executa comenzi)
//   - avansarea automata Battery Check -> START (doar daca politica permite)
//   - CRITICAL forteaza Battery Check din orice pagina (+ eveniment
//     FORCED_LOW_POWER catre Actions)
//   - gating START: emite START_CONFIRMED numai daca politica permite
//   - navigarea din DASHBOARD: FLASH -> Flash DUT, SVC OUT -> START
//   - din FLASH_DUT: BACK -> DASHBOARD (sesiunea de service ramane
//     activa); terminarea flash-ului -> START (panelul a reboot-at)
//
// Fara timp minim de stationare pe pagini: debounce-ul touch din
// screen-uri (armare pe front + eliberare confirmata pe timp) face
// inutila protectia dwell la tranzitii.
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
    // finalul setup(): reseteaza timpul de activitate, astfel timeout-ul
    // de 5s porneste din momentul in care pagina devine efectiv vizibila
    // (setup-ul poate dura >5s: BAT KEY 3s + SD self-test + mount), nu din
    // momentul constructii.
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
    // true cand standby-ul activ este cel de CHARGING (ecran negru +
    // punct verde); false pentru standby-ul LOW/CRITICAL (indicator +
    // tensiune) din Battery Check.
    bool          _standbyCharging;

    static constexpr unsigned long SCREEN_ON_TIMEOUT_MS  = 5000UL;
    // Inactivitate in CHARGING (Battery Check / START) pana la standby.
    static constexpr unsigned long CHARGING_STANDBY_MS   = 10000UL;
    static constexpr unsigned long STANDBY_BLINK_OFF_MS  = 2000UL;
    static constexpr unsigned long STANDBY_BLINK_ON_MS   = 1000UL;

    void enterPage(UiPageId page);
    void pushEvent(UiEvent ev);

    void updateBatteryCheckPage(int16_t x, int16_t y, bool touched);
    void updateStartPage(int16_t x, int16_t y, bool touched);
    void updateDashboardPage(int16_t x, int16_t y, bool touched);
    void updateSdCheckPage(int16_t x, int16_t y, bool touched);
    void updateFlashDutPage(int16_t x, int16_t y, bool touched);

    void enterStandby();
    void enterChargingStandby();
    void wakeFromStandby();
    bool updateStandbyBlinkPhase(); // true cand s-a schimbat faza de clipit
};

#endif // PAGE_NAVIGATOR_H
