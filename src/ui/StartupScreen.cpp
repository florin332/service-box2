#include "StartupScreen.h"

// Desenarea este delegata catre StartupView (renderer pasiv, layout relativ).
// Gating-ul START si pragurile de nivel sunt in NavigationPolicy; aici ramane
// doar interactiunea tactila cu butonul si anti-flicker-ul.

StartupScreen::StartupScreen(Adafruit_GFX* tftInstance, IBatteryProvider& battery)
    : _view(tftInstance)
    , _battery(battery)
    , _isRendered(false)
    , _buttonPressed(false)
    , _displayedLevel(0xFF)
    , _displayedCharging(false)
    , _lastBatteryUpdateMs(0)
{
}

void StartupScreen::init() {
    _isRendered = false;
    _buttonPressed = false;
    _displayedLevel = 0xFF;
    _displayedCharging = false;
    _lastBatteryUpdateMs = 0;
    _displayedButtonText = "";
}

void StartupScreen::render(bool forceRedraw) {
    if (_isRendered && !forceRedraw) return;

    StartupViewModel m;
    m.batteryVoltageV = _battery.getVoltageV();
    m.batteryColor    = NavigationPolicy::indicatorColor(
                            _battery.getLevelPercent(), _battery.isCharging());
    m.buttonText = NavigationPolicy::startButtonText(_battery.getState());
    m.sdFreeMB     = 0;
    m.sdTotalMB    = 0;
    m.sdAvailable  = false;

    _view.renderFull(m);

    _displayedLevel      = _battery.getLevelPercent();
    _displayedCharging   = _battery.isCharging();
    _displayedButtonText = m.buttonText;
    _isRendered = true;
}

void StartupScreen::drawButton() {
    // Textul butonului vine din politica de navigatie (START / BATT LOW).
    const char* btnText = NavigationPolicy::startButtonText(_battery.getState());

    // Evită redesenarea continuă: redesenează DOAR dacă textul s-a schimbat.
    // Starea "pressed" nu are feedback vizual, deci nu trebuie să declanșeze
    // redesenare (asta era sursa flicker-ului la atingere, vizibil mai ales
    // în starea blocat "BATT LOW" unde utilizatorul atinge repetat).
    if (_displayedButtonText.equals(btnText)) {
        return;
    }
    _displayedButtonText = btnText;

    _view.drawButton(btnText);
}

void StartupScreen::drawBatteryInfo() {
    uint8_t level = _battery.getLevelPercent();
    bool charging = _battery.isCharging();

    StartupViewModel m;
    m.batteryVoltageV = _battery.getVoltageV();
    m.batteryColor    = NavigationPolicy::indicatorColor(level, charging);

    _view.drawBatteryInfo(m);

    _displayedLevel = level;
    _displayedCharging = charging;
}

void StartupScreen::updateStorageInfo(uint32_t freeMB, uint32_t totalMB, bool cardAvailable) {
    _view.updateStorage(freeMB, totalMB, cardAvailable);
}

bool StartupScreen::update(int touchX, int touchY, bool isTouched) {
    bool startTriggered = false;

    // Periodic battery update (same interval as BatteryCheckScreen)
    unsigned long now = millis();
    if (now - _lastBatteryUpdateMs >= BATTERY_UPDATE_MS) {
        _lastBatteryUpdateMs = now;
        uint8_t currentLevel = _battery.getLevelPercent();
        bool currentCharging = _battery.isCharging();
        if (currentLevel != _displayedLevel || currentCharging != _displayedCharging) {
            drawBatteryInfo();
        }
    }

    if (isTouched) {
        // Evaluate boundary box intersection parameters
        // (geometria butonului vine din view, calculata din layout)
        const UiRect& btn = _view.buttonRect();
        if (touchX >= btn.x && touchX <= (btn.x + btn.w) &&
            touchY >= btn.y && touchY <= (btn.y + btn.h)) {

            // Fără redesenare la apăsare: nu există feedback vizual de culoare,
            // redesenarea era sursa flicker-ului.
            _buttonPressed = true;
        } else {
            // Finger drifted outside boundary limits while pressing down
            _buttonPressed = false;
        }
    } else {
        // Finger released from active capacitive/resistive target window
        if (_buttonPressed) {
            _buttonPressed = false;

            // Aplicare politică baterie (NavigationPolicy): LOW/CRITICAL
            // blochează inițierea unui test nou.
            if (!NavigationPolicy::startAllowed(_battery.getState())) {
                Serial.println("[StartupScreen] START blocked: battery LOW/CRITICAL");
                startTriggered = false;
            } else {
                startTriggered = true; // Dispatch execution signal on confirmation release
            }
        }
    }

    // Asigură sincronizarea stării vizuale chiar dacă textul nu s-a schimbat
    // (de ex. la prima afișare sau după init).
    drawButton();

    return startTriggered;
}
