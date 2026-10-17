#ifndef STARTUP_SCREEN_H
#define STARTUP_SCREEN_H

#include <Arduino.h>
#include "battery/IBatteryProvider.h"
#include "graphic/StartupView.h"
#include "graphic/UiTheme.h"
#include "navigation/NavigationPolicy.h"

// Decuplat de HAL pe branch-ul graphic_ui: primeste doar o instanta
// Adafruit_GFX (display real ST7789 pe Waveshare / simulator pe Marble)
// si o referinta la furnizorul de stare baterie.
//
// Politică baterie:
//   - START confirmă navigarea către Dashboard doar dacă starea bateriei
//     este NORMAL sau CHARGING.
//   - LOW sau CRITICAL blochează inițierea unui test nou (returnează false),
//     conform politicii de navigație. Testele în desfășurare NU sunt oprite
//     de această pagină; oprirea controlată la CRITICAL se face în main.cpp.

class StartupScreen {
private:
    StartupView _view;      // renderer pasiv (deseneaza ce primeste)
    IBatteryProvider& _battery;
    bool _isRendered;
    bool _buttonPressed;
    bool _sdButtonPressed;  // hit-tracking buton SD (activare la eliberare)
    bool _touchDown;        // front de atingere (debounce)
    uint32_t _releaseStartMs; // inceputul eliberarii (0 = niciuna)
    bool _sdAvailable;      // stare card (setata prin updateStorageInfo)
    uint32_t _sdFreeMB;     // capacitati reale (retine pentru renderFull)
    uint32_t _sdTotalMB;

    // Battery display tracking (anti-flicker: redesenare doar la schimbare)
    uint8_t _displayedLevel;
    bool _displayedCharging;
    unsigned long _lastBatteryUpdateMs;

    // Button display tracking (avoid redraw flicker).
    // Butonul nu are feedback vizual la apasare, deci doar textul conteaza.
    String _displayedButtonText;

    // Pragurile de nivel si gating-ul baterie sunt in NavigationPolicy.
    static constexpr unsigned long BATTERY_UPDATE_MS = 1500UL;
    // Debounce touch: eliberarea este acceptata dupa atatea ms continui fara
    // atingere (loop() nu are delay, un prag pe cadre ar fi inutil).
    static constexpr uint32_t TOUCH_RELEASE_CONFIRM_MS = 60;

    void drawButton();
    void drawBatteryInfo();

public:
    StartupScreen(Adafruit_GFX* tftInstance, IBatteryProvider& battery);
    void init();
    void render(bool forceRedraw = false);

    // Interactiune tactila. Returneaza o cerere de navigare (sau NONE).
    // START ramane conditionat de politica baterie; SD este gri/dezactivat
    // cand lipseste cardul.
    enum class TouchRequest : uint8_t { NONE, START, SD_CHECK };
    TouchRequest update(int touchX, int touchY, bool isTouched);

    void updateStorageInfo(uint32_t freeMB, uint32_t totalMB, bool cardAvailable);
};

#endif // STARTUP_SCREEN_H
