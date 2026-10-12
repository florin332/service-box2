#ifndef UI_EVENTS_H
#define UI_EVENTS_H

#include <stdint.h>

// ============================================================================
// Evenimente emise de stratul Navigation catre Actions / main.
//
// Navigation NU executa nimic: doar raporteaza intentii. Coada de
// evenimente este consumata de main (si, la pasul 4, de stratul Actions).
// ============================================================================

enum class UiEvent : uint8_t {
    NONE = 0,
    START_CONFIRMED,    // utilizatorul a confirmat START si politica a permis
    BACK_TO_START,      // navigare inapoi catre START (ex: buton BACK)
    FORCED_LOW_POWER,   // baterie CRITICAL: nav fortata la Battery Check
                        // (Actions poate opri testele in desfasurare)
    SD_CHECK_REQUESTED, // utilizatorul a deschis pagina SD Check
};

#endif // UI_EVENTS_H
