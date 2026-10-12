#include "PageNavigator.h"

#include "../StartupScreen.h"
#include "../BatteryCheckScreen.h"
#include "../SdCheckScreen.h"
#include "NavigationPolicy.h"
#include "../graphic/UiLayout.h"
#include "../graphic/UiTheme.h"
#include "bsp/bsp_display.h"
#include "bsp/bsp_touch.h"
#include "bsp/bsp_ina219.h"

// ============================================================================
// PageNavigator - implementare.
//
// Desenarea dashboard-ului este PROXIM aici (placeholderul din main.cpp,
// acum cu geometrie derivata din dimensiunile display-ului). Cand va fi
// promovat la pagina completa, va primi DashboardView in ui/graphic.
// ============================================================================

PageNavigator::PageNavigator(StartupScreen& startup,
                             BatteryCheckScreen& batteryCheck,
                             SdCheckScreen& sdCheck,
                             IBatteryProvider& battery)
    : _startup(startup)
    , _batteryCheck(batteryCheck)
    , _sdCheck(sdCheck)
    , _battery(battery)
    , _page(UiPageId::BATTERY_CHECK)
    , _pageNeedsRender(true)
    , _evHead(0)
    , _evTail(0)
    , _dashboardBaseDrawn(false)
    , _dashboardLastRefreshMs(0)
    , _backWasHeld(false)
    , _screenOn(true)
    , _lastActivityMs(0)
    , _standbyBlinkTimerMs(0)
    , _standbyBlinkOn(false)
    , _standbyIndicatorVisible(false)
{
}

// ---------------------------------------------------------------------------
// Coada de evenimente
// ---------------------------------------------------------------------------

void PageNavigator::pushEvent(UiEvent ev)
{
    uint8_t next = (_evHead + 1) % (sizeof(_events) / sizeof(_events[0]));
    if (next == _evTail) return; // coada plina - se pierde (defensiv)
    _events[_evHead] = ev;
    _evHead = next;
}

UiEvent PageNavigator::pollEvent()
{
    if (_evTail == _evHead) return UiEvent::NONE;
    UiEvent ev = _events[_evTail];
    _evTail = (_evTail + 1) % (sizeof(_events) / sizeof(_events[0]));
    return ev;
}

// ---------------------------------------------------------------------------
// Tranzitii de pagina
// ---------------------------------------------------------------------------

void PageNavigator::enterPage(UiPageId page)
{
    _page = page;
    _pageNeedsRender = true;

    switch (page) {
        case UiPageId::BATTERY_CHECK:
            _batteryCheck.init();
            _screenOn = true;
            _lastActivityMs = millis();  // reset la intrare (evita timeout la boot)
            _standbyIndicatorVisible = false;
            _standbyBlinkOn = false;
            break;

        case UiPageId::START:
            _startup.init();
            break;

        case UiPageId::DASHBOARD:
            _dashboardBaseDrawn = false;
            _dashboardLastRefreshMs = millis();
            _backWasHeld = false;
            break;

        case UiPageId::SD_CHECK:
            // Snapshot-ul SD este citit si desenat aici, la intrarea
            // in pagina (init contine si desenarea completa).
            _sdCheck.init();
            _pageNeedsRender = false;
            break;
    }
}

// ---------------------------------------------------------------------------
// Standby Battery Check (timeout 5s, clipit 2s stins / 1s aprins)
// ---------------------------------------------------------------------------

void PageNavigator::enterStandby()
{
    _screenOn = false;
    _batteryCheck.view().clearAll();

    _standbyBlinkTimerMs = millis();
    _standbyBlinkOn = true;
    _standbyIndicatorVisible = true; // punctul porneste aprins, fara cadru negru

    BatteryCheckViewModel m;
    _batteryCheck.fillModel(m);
    m.standbyIndicatorOn = true;
    _batteryCheck.view().renderStandby(m);
}

void PageNavigator::wakeFromStandby()
{
    _screenOn = true;
    _batteryCheck.render(true); // redesenare completa a paginii
}

// Actualizeaza faza de clipit; true cand vizibilitatea s-a schimbat.
bool PageNavigator::updateStandbyBlinkPhase()
{
    unsigned long now = millis();
    unsigned long elapsed = now - _standbyBlinkTimerMs;

    bool newVisible = _standbyIndicatorVisible;

    if (_standbyBlinkOn) {
        if (elapsed >= STANDBY_BLINK_ON_MS) {
            _standbyBlinkOn = false;
            _standbyBlinkTimerMs = now;
            newVisible = false;
        } else {
            newVisible = true;
        }
    } else {
        if (elapsed >= STANDBY_BLINK_OFF_MS) {
            _standbyBlinkOn = true;
            _standbyBlinkTimerMs = now;
            newVisible = true;
        } else {
            newVisible = false;
        }
    }

    if (newVisible == _standbyIndicatorVisible) return false;
    _standbyIndicatorVisible = newVisible;
    return true;
}

// ---------------------------------------------------------------------------
// Pagini
// ---------------------------------------------------------------------------

void PageNavigator::updateBatteryCheckPage(int16_t x, int16_t y, bool touched)
{
    (void)x; (void)y; // Battery Check nu are controale (vezi ui_requirements.md)

    unsigned long now = millis();

    // Atingerea: reaprinde pagina din standby si reseteaza timerul.
    // NU este o comanda si nu modifica starea bateriei.
    if (touched) {
        if (!_screenOn) {
            wakeFromStandby();
        }
        _lastActivityMs = now;
    }

    // In standby: clipitul avanseaza aici (timing), nu in render().
    if (!_screenOn) {
        if (updateStandbyBlinkPhase()) {
            BatteryCheckViewModel m;
            _batteryCheck.fillModel(m);
            m.standbyIndicatorOn = _standbyIndicatorVisible;
            _batteryCheck.view().renderStandby(m);
        }
    }

    // Redesenare (partiala/completa dupa flag) - anti-flicker e in screen.
    _batteryCheck.render(_pageNeedsRender);
    _pageNeedsRender = false;

    // Timeout de 5 secunde fara atingere.
    if (_screenOn && (now - _lastActivityMs >= SCREEN_ON_TIMEOUT_MS)) {
        BatteryState state = _battery.getState();

        if (NavigationPolicy::autoAdvanceAllowed(state)) {
            // Nivel suficient sau incarcare finalizata: avansare la START.
            Serial.println("[UI Navigation] Battery Check OK -> START");
            enterPage(UiPageId::START);
        } else {
            // LOW, CRITICAL sau CHARGING: ramane in Battery Check (standby).
            enterStandby();
        }
    }
}

void PageNavigator::updateStartPage(int16_t x, int16_t y, bool touched)
{
    if (_pageNeedsRender) {
        _startup.render(true);
        _pageNeedsRender = false;
    }

    switch (_startup.update(x, y, touched)) {
        case StartupScreen::TouchRequest::START:
            Serial.println("[UI Navigation] START confirmat -> DASHBOARD");
            enterPage(UiPageId::DASHBOARD);
            pushEvent(UiEvent::START_CONFIRMED);
            break;

        case StartupScreen::TouchRequest::SD_CHECK:
            Serial.println("[UI Navigation] SD -> SD_CHECK");
            enterPage(UiPageId::SD_CHECK);
            pushEvent(UiEvent::SD_CHECK_REQUESTED);
            break;

        case StartupScreen::TouchRequest::NONE:
        default:
            break;
    }
}

void PageNavigator::updateSdCheckPage(int16_t x, int16_t y, bool touched)
{
    // Pagina este statica (desenata la intrare); aici doar butonul BACK.
    if (_sdCheck.update(x, y, touched)) {
        Serial.println("[UI Navigation] SD Check BACK -> START");
        bsp_touch_wait_release();
        enterPage(UiPageId::START);
        pushEvent(UiEvent::BACK_TO_START);
    }
}

bool PageNavigator::dashboardBackHit(int16_t x, int16_t y, bool touched)
{
    // Geometrie buton BACK derivata din display (aceeasi ca la desenare):
    // (50,270,140,35) pe 240x320 -> (208,844,583,109) miimi
    UiLayout layout(bsp_display_width(), bsp_display_height());
    UiRect btn = layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                     208, 844, 583, 109);

    bool inBtn = (x >= btn.x && x <= btn.x + btn.w &&
                  y >= btn.y && y <= btn.y + btn.h);

    if (touched && inBtn) {
        _backWasHeld = true;
    } else if (!touched && _backWasHeld) {
        _backWasHeld = false;
        return true; // confirmare la eliberare
    }
    return false;
}

void PageNavigator::updateDashboardPage(int16_t x, int16_t y, bool touched)
{
    if (_pageNeedsRender) {
        // Desenare initiala (baza + valori)
        UiLayout layout(bsp_display_width(), bsp_display_height());
        Adafruit_GFX* tft = bsp_display_gfx();

        tft->fillScreen(UiTheme::COLOR_BACKGROUND);

        uint8_t sTitle = UiTheme::textSizeScaled(layout.height(), 3);
        tft->setTextColor(UiTheme::COLOR_TEXT_MAIN);
        tft->setTextSize(sTitle);
        tft->setCursor(layout.centerTextX(sTitle, strlen("DASHBOARD")),
                       layout.py(141));
        tft->print("DASHBOARD");

        uint8_t sSub = UiTheme::textSizeScaled(layout.height(), 1);
        tft->setTextColor(UiTheme::COLOR_TEXT_MUTED);
        tft->setTextSize(sSub);
        tft->setCursor(layout.centerTextX(sSub, strlen("INA219 (provisional)")),
                       layout.py(297));
        tft->print("INA219 (provisional)");

        // Buton BACK
        UiRect btn = layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                         208, 844, 583, 109);
        int16_t radius = layout.scaleMin(25);
        tft->fillRoundRect(btn.x, btn.y, btn.w, btn.h, radius, UiTheme::COLOR_BTN_BASE);
        tft->drawRoundRect(btn.x, btn.y, btn.w, btn.h, radius, UiTheme::COLOR_TEXT_MAIN);

        uint8_t sBtn = UiTheme::textSizeScaled(layout.height(), 2);
        tft->setTextColor(UiTheme::COLOR_TEXT_MAIN);
        tft->setTextSize(sBtn);
        tft->setCursor(btn.x + (btn.w - 4 * 6 * sBtn) / 2,
                       btn.y + (btn.h - 8 * sBtn) / 2);
        tft->print("BACK");

        _dashboardBaseDrawn = true;
        dashboardUpdate(); // prima afisare a valorilor imediat

        _pageNeedsRender = false;
        Serial.println("[UI Navigation] Dashboard afisat.");
    }

    dashboardUpdate();

    if (dashboardBackHit(x, y, touched)) {
        Serial.println("[UI Navigation] BACK -> START");
        bsp_touch_wait_release();
        enterPage(UiPageId::START);
        pushEvent(UiEvent::BACK_TO_START);
    }
}

// Refresh periodic valori INA219 cat timp dashboard-ul este activ.
void PageNavigator::dashboardUpdate()
{
    if (!_dashboardBaseDrawn) return;

    unsigned long now = millis();
    if (now - _dashboardLastRefreshMs < DASHBOARD_REFRESH_MS) return;
    _dashboardLastRefreshMs = now;

    UiLayout layout(bsp_display_width(), bsp_display_height());
    Adafruit_GFX* tft = bsp_display_gfx();

    // Redesenare doar a zonei valorilor (fara flicker pe restul ecranului)
    UiRect zone = layout.anchoredRect(UiHAnchor::LEFT, UiVAnchor::TOP,
                                      83, 359, 833, 125);
    tft->fillRect(zone.x, zone.y, zone.w, zone.h, UiTheme::COLOR_BACKGROUND);

#if defined(SERVICEBOX_WAVESHARE)
    ina219_data_t data;
    bsp_ina219_read(&data);

    uint8_t sVal = UiTheme::textSizeScaled(layout.height(), 2);
    tft->setTextSize(sVal);

    tft->setTextColor(UiTheme::COLOR_YELLOW, UiTheme::COLOR_BACKGROUND);
    tft->setCursor(zone.x, layout.py(359));
    tft->print("V: ");
    tft->print(data.bus_voltage_V, 2);
    tft->print(" V  ");

    tft->setTextColor(UiTheme::COLOR_GREEN, UiTheme::COLOR_BACKGROUND);
    tft->setCursor(zone.x, layout.py(431));
    tft->print("I: ");
    tft->print(data.current_mA, 1);
    tft->print(" mA  ");
#else
    uint8_t sVal = UiTheme::textSizeScaled(layout.height(), 1);
    tft->setTextSize(sVal);
    tft->setTextColor(UiTheme::COLOR_TEXT_MUTED, UiTheme::COLOR_BACKGROUND);
    tft->setCursor(zone.x, layout.py(359));
    tft->print("INA219: n/a pe acest target");
#endif
}

// ---------------------------------------------------------------------------
// Punctul de intrare: o iteratie de loop
// ---------------------------------------------------------------------------

void PageNavigator::update(int16_t touchX, int16_t touchY, bool touched)
{
    // Regula globala: CRITICAL forteaza Battery Check din orice pagina.
    if (NavigationPolicy::forceBatteryCheck(_battery.getState()) &&
        _page != UiPageId::BATTERY_CHECK) {
        Serial.println("[UI Navigation] CRITICAL battery -> BATTERY_CHECK");
        enterPage(UiPageId::BATTERY_CHECK);
        pushEvent(UiEvent::FORCED_LOW_POWER);
    }

    switch (_page) {
        case UiPageId::BATTERY_CHECK:
            updateBatteryCheckPage(touchX, touchY, touched);
            break;
        case UiPageId::START:
            updateStartPage(touchX, touchY, touched);
            break;
        case UiPageId::DASHBOARD:
            updateDashboardPage(touchX, touchY, touched);
            break;
        case UiPageId::SD_CHECK:
            updateSdCheckPage(touchX, touchY, touched);
            break;
    }
}

void PageNavigator::render()
{
    // Desenarea este integrata in update*() (fiecare pagina isi consuma
    // singura flag-ul _pageNeedsRender). Metoda exista pentru claritatea
    // buclei din main si pentru extensii viitoare.
}
