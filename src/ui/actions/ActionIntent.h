#ifndef ACTION_INTENT_H
#define ACTION_INTENT_H

#include <stdint.h>
#include "../navigation/UiEvents.h"

// ============================================================================
// ActionIntent - mapare intre evenimentele UI si actiunile concrete.
//
// Actions NU include nimic din UI: primeste UiEvent si il traduce in
// apeluri BSP / logica de business. Rezultatele se intorc prin
// ActionResult (pentru diagnostic / eventual feedback catre Navigation).
// ============================================================================

enum class ActionResult : uint8_t {
    OK = 0,
    BLOCKED,      // politica a blocat (ex: baterie insuficienta)
    NOT_READY,    // hardware/serviciu indisponibil
    ERROR         // eroare la executie
};

struct ActionOutcome {
    ActionResult result;
    const char*  detail;   // text scurt pentru serial/debug (nu UI)
};

#endif // ACTION_INTENT_H
