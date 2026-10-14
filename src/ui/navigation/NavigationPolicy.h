#ifndef NAVIGATION_POLICY_H
#define NAVIGATION_POLICY_H

#include <stdint.h>
#include "battery/IBatteryProvider.h"
#include "../graphic/UiTheme.h"

// ============================================================================
// NavigationPolicy - TOATE regulile si restrictiile de navigatie ale UI-ului,
// centralizate aici. Functii pure: fara desenare, fara BSP, fara efecte.
//
// Reguli acoperite (preluate din StartupScreen / BatteryCheckScreen /
// main.cpp, conform src/ui_requirements.md):
//   - gating START: doar NORMAL / CHARGING / CHARGED permit pornirea;
//     LOW / CRITICAL blocheaza initierea unui test nou
//   - avansare automata din Battery Check: NORMAL / CHARGING / CHARGED,
//     si numai dupa expirarea timeout-ului (gestionat de PageNavigator)
//   - CRITICAL forteaza intoarcerea la Battery Check din orice pagina
//   - praguri nivel baterie pentru culoarea indicatorului (20% / 60%)
//   - baterie complet incarcata (100% + charging) => verde continuu
//   - textul butonului START ("START" / "BATT LOW")
// ============================================================================

namespace NavigationPolicy {

    // Poate utilizatorul initia un test nou din pagina START?
    inline bool startAllowed(BatteryState s)
    {
        return (s == BatteryState::NORMAL) ||
               (s == BatteryState::CHARGING) ||
               (s == BatteryState::CHARGED);
    }

    // Textul afisat pe butonul principal al paginii START.
    inline const char* startButtonText(BatteryState s)
    {
        return startAllowed(s) ? "START" : "BATT LOW";
    }

    // Avansare automata Battery Check -> START la expirarea timeout-ului?
    // Aceleasi stari care permit si initierea unui test (NORMAL, CHARGING,
    // CHARGED); doar LOW si CRITICAL blocheaza avansarea.
    inline bool autoAdvanceAllowed(BatteryState s)
    {
        return startAllowed(s);
    }

    // CRITICAL forteaza navigarea la Battery Check din orice pagina.
    inline bool forceBatteryCheck(BatteryState s)
    {
        return (s == BatteryState::CRITICAL);
    }

    // Stare calitativa a nivelului (doar pentru culoarea indicatorului).
    enum class LevelState : uint8_t { LOW, MEDIUM, GOOD };

    constexpr uint8_t LOW_THRESHOLD_PERCENT    = 20;
    constexpr uint8_t MEDIUM_THRESHOLD_PERCENT = 60;

    inline LevelState evaluateLevel(uint8_t percent)
    {
        if (percent < LOW_THRESHOLD_PERCENT)    return LevelState::LOW;
        if (percent < MEDIUM_THRESHOLD_PERCENT) return LevelState::MEDIUM;
        return LevelState::GOOD;
    }

    // Culoarea indicatorului de baterie, rezolvata complet (inclusiv regula
    // 100% + charging => verde continuu). View-urile primesc aceasta culoare
    // prin ViewModel si nu mai decid nimic.
    inline uint16_t indicatorColor(uint8_t percent, bool charging)
    {
        if (percent == 100 && charging) {
            return UiTheme::COLOR_GREEN;
        }
        switch (evaluateLevel(percent)) {
            case LevelState::LOW:    return UiTheme::COLOR_RED;
            case LevelState::MEDIUM: return UiTheme::COLOR_YELLOW;
            case LevelState::GOOD:
            default:                 return UiTheme::COLOR_GREEN;
        }
    }

} // namespace NavigationPolicy

#endif // NAVIGATION_POLICY_H
