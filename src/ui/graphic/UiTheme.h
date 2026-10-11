#ifndef UI_THEME_H
#define UI_THEME_H

#include <stdint.h>

// ============================================================================
// UiTheme - paleta unica de culori + reguli de scalare a fonturilor.
//
// Singurul loc din UI cu valori de culoare. Culorile sunt absolute (RGB565),
// geometria NU este aici (se calculeaza relativ in UiLayout / view-uri).
//
// Maparea stare -> culoare NU este aici: decizia de nivel (LOW/MEDIUM/GOOD)
// apartine straturilor superioare; view-urile primesc culoarea deja rezolvata.
// ============================================================================

namespace UiTheme {

    constexpr uint16_t COLOR_BACKGROUND = 0x0000; // Black
    constexpr uint16_t COLOR_TEXT_MAIN  = 0xFFFF; // White
    constexpr uint16_t COLOR_TEXT_MUTED = 0x9DF3; // Gray
    constexpr uint16_t COLOR_BTN_BASE   = 0x1967; // Deep Blue
    constexpr uint16_t COLOR_GREEN      = 0x07E0; // Bright Green
    constexpr uint16_t COLOR_YELLOW     = 0xFFE0; // Yellow
    constexpr uint16_t COLOR_RED        = 0xF800; // Red

    // Dimensiune font scalata dupa inaltimea display-ului.
    // baseAt320 = dimensiunea de referinta pentru 320 px inaltime
    // (aspectul validat pe hardware). Minim 1, fara limita superioara.
    inline uint8_t textSizeScaled(uint16_t displayHeight, uint8_t baseAt320)
    {
        uint16_t s = (displayHeight * baseAt320 + 160) / 320;
        return (s < 1) ? 1 : (uint8_t)s;
    }

} // namespace UiTheme

#endif // UI_THEME_H
