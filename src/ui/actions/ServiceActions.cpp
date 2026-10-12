#include "ServiceActions.h"
#include "battery/IBatteryProvider.h"

// ============================================================================
// ServiceActions - implementare.
//
// Toate actiunile sunt proxim (placeholder) pana la integrarea reala cu
// hardware-ul de test. Structura este insa finala: eveniment in, rezultat out.
// ============================================================================

ServiceActions::ServiceActions(IBatteryProvider& battery)
    : _battery(battery)
    , _testActive(false)
{
}

ActionOutcome ServiceActions::execute(UiEvent ev)
{
    switch (ev) {
        case UiEvent::START_CONFIRMED:   return handleStartConfirmed();
        case UiEvent::FORCED_LOW_POWER:  return handleForcedLowPower();
        case UiEvent::BACK_TO_START:     return handleBackToStart();
        case UiEvent::SD_CHECK_REQUESTED:
            // Navigare catre pagina SD Check: nimic de executat in Actions.
            return { ActionResult::OK, "sd check page" };
        case UiEvent::NONE:
        default:
            return { ActionResult::OK, "none" };
    }
}

void ServiceActions::update()
{
    // Pump periodic pentru actiuni cu durata (pipeline teste).
    // Deocamdata: daca testul e activ si bateria a devenit CRITICAL,
    // oprirea se face prin FORCED_LOW_POWER (venit din navigator).
}

ActionOutcome ServiceActions::handleStartConfirmed()
{
    // Politica de gating e deja aplicata in Navigation; aici doar executam.
    // TODO(pas viitor): pornire secventa teste panouri ascensoare.
    _testActive = true;
    Serial.println("[Actions] START_CONFIRMED -> secventa teste (placeholder)");
    return { ActionResult::OK, "test started (placeholder)" };
}

ActionOutcome ServiceActions::handleForcedLowPower()
{
    // CRITICAL: oprire controlata a oricarui test in desfasurare.
    // La integrarea reala: dezactivare drivere, salvare stare, etc.
    if (_testActive) {
        _testActive = false;
        Serial.println("[Actions] FORCED_LOW_POWER: test oprit (CRITICAL)");
    } else {
        Serial.println("[Actions] FORCED_LOW_POWER: niciun test activ");
    }
    return { ActionResult::OK, "forced shutdown" };
}

ActionOutcome ServiceActions::handleBackToStart()
{
    // Curatenie la iesire din dashboard (deocamdata: nimic de facut).
    return { ActionResult::OK, "back to start" };
}
