#include "StartupScreen.h"

// Desenarea este delegata catre StartupView (renderer pasiv, layout relativ).
// Gating-ul START si pragurile de nivel sunt in NavigationPolicy; aici ramane
// doar interactiunea tactila cu butonul si anti-flicker-ul.

StartupScreen::StartupScreen(Adafruit_GFX* tftInstance, IBatteryProvider& battery)
    : _view(tftInstance)
    , _battery(battery)
    , _isRendered(false)
    , _buttonPressed(false)
    , _sdButtonPressed(false)
    , _sdAvailable(false)
    , _sdFreeMB(0)
    , _sdTotalMB(0)
    , _displayedLevel(0xFF)
    , _displayedCharging(false)
    , _lastBatteryUpdateMs(0)
{
}

void StartupScreen::init() {
    _isRendered = false;
    _buttonPressed = false;
    _sdButtonPressed = false;
    _displayedLevel = 0xFF;
    _displayedCharging = false;
    _lastBatteryUpdateMs = 0;
    _displayedButtonText = "";
}

void StartupScreen::render(bool forceRedraw) {
    if (_isRendered && !forceRedraw) return;

    StartupViewModel m;
    m.batteryVoltageV = _battery.getVoltageV();  // tensiunea reala (INA219)
    m.batteryColor    = NavigationPolicy::indicatorColor(
                            _battery.getLevelPercent(), _battery.isCharging());
    m.buttonText = NavigationPolicy::startButtonText(_battery.getState());
    m.sdFreeMB     = _sdFreeMB;    // capacitati reale (montare la boot)
    m.sdTotalMB    = _sdTotalMB;
    m.sdAvailable  = _sdAvailable;

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
    bool changed = (cardAvailable != _sdAvailable);
    _sdAvailable = cardAvailable;
    _sdFreeMB    = cardAvailable ? freeMB  : 0;  // reale, retinute pt renderFull
    _sdTotalMB   = cardAvailable ? totalMB : 0;

    _view.updateStorage(freeMB, totalMB, cardAvailable);

    // Butonul SD este activ/gri in functie de prezenta cardului.
    if (changed || !_isRendered) {
        _view.drawSdButton(cardAvailable);
    }
}

StartupScreen::TouchRequest StartupScreen::update(int touchX, int touchY, bool isTouched) {
    TouchRequest request = TouchRequest::NONE;

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

    // Hit-test butoane (geometria vine din view, calculata din layout)
    const UiRect& btn   = _view.buttonRect();
    const UiRect& sdBtn = _view.sdButtonRect();

    bool inStart = (touchX >= btn.x && touchX <= (btn.x + btn.w) &&
                    touchY >= btn.y && touchY <= (btn.y + btn.h));
    bool inSd    = (touchX >= sdBtn.x && touchX <= (sdBtn.x + sdBtn.w) &&
                    touchY >= sdBtn.y && touchY <= (sdBtn.y + sdBtn.h));

    if (isTouched) {
        // Fara redesenare la apasare: nu exista feedback vizual de culoare,
        // redesenarea era sursa flicker-ului.
        _buttonPressed   = inStart;
        _sdButtonPressed = inSd && !inStart;
    } else {
        // Finger released from active capacitive/resistive target window
        if (_buttonPressed) {
            _buttonPressed = false;

            // Aplicare politica baterie (NavigationPolicy): LOW/CRITICAL
            // blocheaza initierea unui test nou.
            if (!NavigationPolicy::startAllowed(_battery.getState())) {
                Serial.println("[StartupScreen] START blocked: battery LOW/CRITICAL");
            } else {
                request = TouchRequest::START;
            }
        }

        if (_sdButtonPressed) {
            _sdButtonPressed = false;

            if (_sdAvailable) {
                request = TouchRequest::SD_CHECK;
            } else {
                Serial.println("[StartupScreen] SD blocked: card absent");
            }
        }
    }

    // Asigura sincronizarea starii vizuale chiar daca textul nu s-a schimbat
    // (de ex. la prima afisare sau dupa init).
    drawButton();

    return request;
}
