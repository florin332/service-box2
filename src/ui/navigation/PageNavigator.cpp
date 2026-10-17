#include "PageNavigator.h"

#include "../StartupScreen.h"
#include "../BatteryCheckScreen.h"
#include "../SdCheckScreen.h"
#include "../DashboardScreen.h"
#include "../FlashDutScreen.h"
#include "NavigationPolicy.h"
#include "../graphic/UiLayout.h"
#include "../graphic/UiTheme.h"
#include "bsp/bsp_display.h"
#include "bsp/bsp_touch.h"

// ============================================================================
// PageNavigator - implementare.
//
// Dashboard-ul si pagina Flash DUT sunt screen-uri proprii (DashboardScreen /
// FlashDutScreen); navigatorul doar deleaga inputul si aplica tranzitiile.
// ============================================================================

PageNavigator::PageNavigator(StartupScreen& startup,
                             BatteryCheckScreen& batteryCheck,
                             SdCheckScreen& sdCheck,
                             DashboardScreen& dashboard,
                             FlashDutScreen& flashDut,
                             IBatteryProvider& battery)
    : _startup(startup)
    , _batteryCheck(batteryCheck)
    , _sdCheck(sdCheck)
    , _dashboard(dashboard)
    , _flashDut(flashDut)
    , _battery(battery)
    , _page(UiPageId::BATTERY_CHECK)
    , _pageNeedsRender(true)
    , _evHead(0)
    , _evTail(0)
    , _screenOn(true)
    , _lastActivityMs(0)
    , _standbyBlinkTimerMs(0)
    , _standbyBlinkOn(false)
    , _standbyIndicatorVisible(false)
    , _standbyCharging(false)
{
}

void PageNavigator::begin()
{
    enterPage(UiPageId::BATTERY_CHECK);
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
            _screenOn = true;
            _lastActivityMs = millis();  // standby CHARGING porneste de aici
            // Fara handshake aici: mb_01 (SRV ENTER) pune DUT-ul in
            // Service Mode, deci se trimite doar la apasarea efectiva a
            // butonului START (vezi ServiceActions::handleStartConfirmed).
            break;

        case UiPageId::DASHBOARD:
            _dashboard.init();
            _pageNeedsRender = false;  // init() deseneaza complet
            break;

        case UiPageId::SD_CHECK:
            // Snapshot-ul SD este citit si desenat aici, la intrarea
            // in pagina (init contine si desenarea completa).
            _sdCheck.init();
            _pageNeedsRender = false;
            break;

        case UiPageId::FLASH_DUT:
            // Lista .uf2 + starea DUT sunt citite si desenate la intrare.
            _flashDut.init();
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
    _standbyCharging = false;
    _batteryCheck.view().clearAll();

    _standbyBlinkTimerMs = millis();
    _standbyBlinkOn = true;
    _standbyIndicatorVisible = true; // punctul porneste aprins, fara cadru negru

    BatteryCheckViewModel m;
    _batteryCheck.fillModel(m);
    m.standbyIndicatorOn = true;
    _batteryCheck.view().renderStandby(m);
}

// Standby CHARGING: ecran negru + punct verde clipitor (din Battery
// Check sau START, dupa 10s fara atingere). Pagina curenta NU se
// schimba: la atingere se revine exact la ea.
void PageNavigator::enterChargingStandby()
{
    Serial.println("[UI Navigation] CHARGING inactiv -> standby");
    _screenOn = false;
    _standbyCharging = true;

    _standbyBlinkTimerMs = millis();
    _standbyBlinkOn = true;
    _standbyIndicatorVisible = true;

    _batteryCheck.view().renderChargingStandby(true);
}

void PageNavigator::wakeFromStandby()
{
    _screenOn = true;
    _lastActivityMs = millis();
    _batteryCheck.view().clearAll(); // reseteaza si flag-ul de standby CHARGING
    _pageNeedsRender = true;       // pagina activa se redeseneaza complet
    bsp_touch_wait_release();      // nu propagam atingerea care a trezit ecranul
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

    // Atingerea reseteaza timerul de inactivitate. Exceptie CHARGING:
    // atingerea este AVANS MANUAL catre START (in aceasta stare nu
    // exista avansare automata la timeout). NU modifica starea bateriei.
    // (In standby nu se ajunge aici: update() intercepteaza mai devreme.)
    if (touched) {
        if (_battery.getState() == BatteryState::CHARGING) {
            Serial.println("[UI Navigation] CHARGING + atingere -> START (avans manual)");
            bsp_touch_wait_release();
            enterPage(UiPageId::START);
            return;
        }
        _lastActivityMs = now;
    }

    // Redesenare (partiala/completa dupa flag) - anti-flicker e in screen.
    _batteryCheck.render(_pageNeedsRender);
    _pageNeedsRender = false;

    // Timeout de 5 secunde fara atingere.
    if (now - _lastActivityMs >= SCREEN_ON_TIMEOUT_MS) {
        BatteryState state = _battery.getState();

        if (NavigationPolicy::autoAdvanceAllowed(state)) {
            // Nivel suficient sau incarcare finalizata: avansare la START.
            Serial.println("[UI Navigation] Battery Check OK -> START");
            enterPage(UiPageId::START);
        } else if (state == BatteryState::CHARGING) {
            // CHARGING: fara avansare automata; dupa 10s fara atingere
            // intra in standby (ecran negru + punct verde clipitor).
            if (now - _lastActivityMs >= CHARGING_STANDBY_MS) {
                enterChargingStandby();
            }
        } else {
            // LOW sau CRITICAL: ramane in Battery Check (standby).
            enterStandby();
        }
    }
}

void PageNavigator::updateStartPage(int16_t x, int16_t y, bool touched)
{
    // In CHARGING: 10s fara atingere -> standby (ecran negru + punct
    // verde clipitor); orice atingere reseteaza timerul de inactivitate.
    if (touched) {
        _lastActivityMs = millis();
    }
    if (_battery.getState() == BatteryState::CHARGING &&
        millis() - _lastActivityMs >= CHARGING_STANDBY_MS) {
        enterChargingStandby();
        return;
    }

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

void PageNavigator::updateDashboardPage(int16_t x, int16_t y, bool touched)
{
    if (_pageNeedsRender) {
        _dashboard.render(true);
        _pageNeedsRender = false;
    }

    // Telemetria INA219 este reimprospatata intern, rate-limited.
    _dashboard.render();

    switch (_dashboard.update(x, y, touched)) {
        case DashboardScreen::NavRequest::FLASH:
            Serial.println("[UI Navigation] FLASH -> FLASH_DUT");
            enterPage(UiPageId::FLASH_DUT);
            pushEvent(UiEvent::FLASH_DUT_REQUESTED);
            break;

        case DashboardScreen::NavRequest::SVC_OUT:
            Serial.println("[UI Navigation] SVC OUT -> START");
            bsp_touch_wait_release();
            enterPage(UiPageId::START);
            pushEvent(UiEvent::BACK_TO_START);
            break;

        case DashboardScreen::NavRequest::TEST:
            Serial.println("[UI Navigation] TEST (placeholder - pas viitor)");
            break;

        case DashboardScreen::NavRequest::INFO:
            Serial.println("[UI Navigation] INFO (placeholder - pas viitor)");
            break;

        case DashboardScreen::NavRequest::NONE:
        default:
            break;
    }
}

void PageNavigator::updateFlashDutPage(int16_t x, int16_t y, bool touched)
{
    // Terminarea unui flux de flash (DONE/FAILED): panelul reboot-eaza si
    // iese singur din Service Mode, deci sesiunea nu mai este valida ->
    // revenire la START; noul firmware este redescoperit de handshake-ul
    // de la urmatoarea apasare START (conform prompt1.md).
    // BACK_TO_START declanseaza SRV EXIT in Actions.
    // Verificata inaintea lui update(): are prioritate fata de un BACK
    // eliberat chiar in iteratia terminarii flash-ului.
    if (_flashDut.flashFinished()) {
        Serial.println("[UI Navigation] Flash DUT terminat -> START");
        enterPage(UiPageId::START);
        pushEvent(UiEvent::BACK_TO_START);
        return;  // NU atinge _pageNeedsRender: apartine paginii START
    }

    // BACK: revenire la DASHBOARD (pagina parinte, de unde s-a deschis
    // Flash DUT). Sesiunea de service ramane activa -> NU se emite
    // BACK_TO_START (fara SRV EXIT catre DUT).
    if (_flashDut.update(x, y, touched)) {
        Serial.println("[UI Navigation] Flash DUT BACK -> DASHBOARD");
        bsp_touch_wait_release();
        enterPage(UiPageId::DASHBOARD);
        return;
    }

    _flashDut.render();
    _pageNeedsRender = false;
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

    // Fara timp minim de stationare: debounce-ul touch din screen-uri
    // (armare pe front + eliberare confirmata pe timp) protejeaza impotriva
    // activarii accidentale a butoanelor la tranzitii.

    // Standby activ (LOW/CRITICAL din Battery Check sau CHARGING din
    // Battery Check / START): ruleaza doar clipitul; o atingere trezeste
    // ecranul si revine la pagina activa, fara a ajunge la controale.
    if (!_screenOn) {
        if (updateStandbyBlinkPhase()) {
            if (_standbyCharging) {
                _batteryCheck.view().renderChargingStandby(_standbyIndicatorVisible);
            } else {
                BatteryCheckViewModel m;
                _batteryCheck.fillModel(m);
                m.standbyIndicatorOn = _standbyIndicatorVisible;
                _batteryCheck.view().renderStandby(m);
            }
        }
        if (touched) {
            wakeFromStandby();
        }
        return;
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
        case UiPageId::FLASH_DUT:
            updateFlashDutPage(touchX, touchY, touched);
            break;
    }
}

void PageNavigator::render()
{
    // Desenarea este integrata in update*() (fiecare pagina isi consuma
    // singura flag-ul _pageNeedsRender). Metoda exista pentru claritatea
    // buclei din main si pentru extensii viitoare.
}
