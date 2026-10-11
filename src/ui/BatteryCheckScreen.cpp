#include "BatteryCheckScreen.h"

#include "navigation/NavigationPolicy.h"

// Regulile de navigatie (standby, timeout, avansare) sunt in PageNavigator.
// Aici: doar prezentarea - anti-flicker + modelul pentru view.

BatteryCheckScreen::BatteryCheckScreen(Adafruit_GFX* tft, IBatteryProvider& battery)
    : _view(tft)
    , _battery(battery)
    , _isRendered(false)
    , _lastBatteryUpdateMs(0)
    , _displayedLevel(0xFF)
    , _displayedCharging(false)
{
}

void BatteryCheckScreen::init()
{
    _isRendered = false;
    _lastBatteryUpdateMs = 0;
    _displayedLevel = 0xFF;
    _displayedCharging = false;
}

void BatteryCheckScreen::render(bool forceRedraw)
{
    unsigned long now = millis();

    // Actualizare periodica a valorii bateriei (anti-flicker: redesenare
    // completa doar la schimbare reala de nivel/incarcare).
    if (now - _lastBatteryUpdateMs >= BATTERY_UPDATE_MS) {
        _lastBatteryUpdateMs = now;

        uint8_t currentLevel = _battery.getLevelPercent();
        bool currentCharging = _battery.isCharging();

        if (currentLevel != _displayedLevel || currentCharging != _displayedCharging) {
            _isRendered = false;  // Forteaza re-desenare cu noile valori
            _displayedLevel = currentLevel;
            _displayedCharging = currentCharging;
        }
    }

    if (_isRendered && !forceRedraw) return;

    BatteryCheckViewModel m;
    fillModel(m);
    _view.renderMain(m);

    _displayedLevel = _battery.getLevelPercent();
    _displayedCharging = _battery.isCharging();
    _isRendered = true;
}

// Construieste modelul pasiv pentru view din starea curenta a bateriei.
// Culoarea indicatorului vine rezolvata din NavigationPolicy.
void BatteryCheckScreen::fillModel(BatteryCheckViewModel& m) const
{
    uint8_t level = _battery.getLevelPercent();
    bool charging = _battery.isCharging();
    BatteryState state = _battery.getState();

    m.voltageV           = _battery.getVoltageV();
    m.percent            = level;
    m.indicatorColor     = NavigationPolicy::indicatorColor(level, charging);
    m.standbyIndicatorOn = true;

    if (state == BatteryState::CHARGING || state == BatteryState::CHARGED) {
        m.chargeText  = (state == BatteryState::CHARGING) ? "CHARGING" : "CHARGED";
        m.chargeColor = UiTheme::COLOR_YELLOW;
    } else {
        m.chargeText  = nullptr;
        m.chargeColor = UiTheme::COLOR_TEXT_MAIN;
    }
}

