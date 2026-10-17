#include "DashboardScreen.h"

#if defined(SERVICEBOX_WAVESHARE)
#include "bsp/bsp_ina219.h"
#endif

// ============================================================================
// DashboardScreen - implementare. Vezi header-ul pentru responsabilitati.
// ============================================================================

DashboardScreen::DashboardScreen(Adafruit_GFX* tftInstance)
    : _view(tftInstance)
    , _isRendered(false)
    , _btnHeld(-1)
    , _touchDown(false)
    , _releaseStartMs(0)
    , _lastTelemetryMs(0)
{
    memset(&_model, 0, sizeof(_model));
}

void DashboardScreen::init()
{
    _isRendered = false;
    _btnHeld = -1;
    _touchDown = false;
    _releaseStartMs = 0;
    _lastTelemetryMs = 0;
    readTelemetry();
    render(true);
}

void DashboardScreen::readTelemetry()
{
#if defined(SERVICEBOX_WAVESHARE)
    ina219_data_t data;
    bsp_ina219_read(&data);
    _model.ina219Available = true;
    _model.busVoltageV     = data.bus_voltage_V;
    _model.currentMa       = data.current_mA;
#else
    _model.ina219Available = false;
    _model.busVoltageV     = 0.0f;
    _model.currentMa       = 0.0f;
#endif
}

bool DashboardScreen::hitTest(const UiRect& r, int x, int y)
{
    return (x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h);
}

DashboardScreen::NavRequest DashboardScreen::update(int touchX, int touchY,
                                                    bool isTouched)
{
    // Filtru touch (ca in FlashDutScreen): armarea butonului se face doar pe
    // frontul crescator al atingerii, iar eliberarea este acceptata dupa
    // TOUCH_RELEASE_CONFIRM_MS ms continui fara atingere. Fara filtru,
    // glitch-urile CST328 redeclansa actiunea cat degetul era inca pe ecran.
    if (isTouched) {
        _releaseStartMs = 0;
        if (_touchDown) {
            return NavRequest::NONE;   // tinere apasata - nu se re-armeaza
        }
        _touchDown = true;

        _btnHeld = -1;
        for (uint8_t i = 0; i < DASH_BTN_COUNT; i++) {
            if (hitTest(_view.buttonRect(i), touchX, touchY)) {
                _btnHeld = (int8_t)i;
                break;
            }
        }
        return NavRequest::NONE;
    }

    // Fara atingere: asteptam confirmarea eliberarii.
    if (!_touchDown) {
        return NavRequest::NONE;
    }
    if (_releaseStartMs == 0) {
        _releaseStartMs = millis();
        return NavRequest::NONE;
    }
    if ((uint32_t)(millis() - _releaseStartMs) < TOUCH_RELEASE_CONFIRM_MS) {
        return NavRequest::NONE;
    }
    _touchDown = false;
    _releaseStartMs = 0;

    // Eliberare confirmata: doar daca degetul a ramas in acelasi buton.
    if (_btnHeld < 0) {
        return NavRequest::NONE;
    }

    uint8_t btn = (uint8_t)_btnHeld;
    _btnHeld = -1;
    if (!hitTest(_view.buttonRect(btn), touchX, touchY)) {
        return NavRequest::NONE;
    }

    switch (btn) {
        case DASH_BTN_FLASH:   return NavRequest::FLASH;
        case DASH_BTN_TEST:    return NavRequest::TEST;
        case DASH_BTN_INFO:    return NavRequest::INFO;
        case DASH_BTN_SVC_OUT: return NavRequest::SVC_OUT;
        default:               return NavRequest::NONE;
    }
}

void DashboardScreen::render(bool forceRedraw)
{
    if (!_isRendered || forceRedraw) {
        _view.renderFull(_model);
        _isRendered = true;
        _lastTelemetryMs = millis();
        return;
    }

    unsigned long now = millis();
    if (now - _lastTelemetryMs >= TELEMETRY_REFRESH_MS) {
        _lastTelemetryMs = now;
        readTelemetry();
        _view.drawTelemetry(_model);
    }
}
