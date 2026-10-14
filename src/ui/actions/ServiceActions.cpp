#include "ServiceActions.h"
#include "battery/IBatteryProvider.h"

#include "dut/dut_host.h"
#include "dut/dut_protocol.h"

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
        case UiEvent::FLASH_DUT_REQUESTED:
            // Navigare catre pagina Flash DUT: flash-ul este condus direct
            // de FlashDutScreen prin dut_flash (vezi src/dut).
            return { ActionResult::OK, "flash dut page" };
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
    // Handshake la apasarea butonului START (prompt1.md: detect ->
    // handshake -> identificare): "mb_01" (SRV ENTER) pune DUT-ul in
    // Service Mode si intoarce identitatea "D_...". Blocant max ~2.5s,
    // doar daca DUT-ul raspunde. Rezultatul (PanelInfo) este pastrat in
    // dut_protocol si citit de pagina Flash pentru filtrarea firmware-urilor.
    if (dut_cdc_connected()) {
        char identity[24];
        if (dut_enter_service(identity, sizeof(identity))) {
            Serial.printf("[Actions] START handshake: %s\n", identity);
        } else {
            Serial.println("[Actions] START: handshake esuat");
        }
    }
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
    // SVC OUT (sau orice revenire la START): DUT iese din Service Mode
    // ("mb_00" = alias SRV EXIT) si revine la functionarea normala.
    if (dut_cdc_connected()) {
        if (dut_exit_service()) {
            Serial.println("[Actions] DUT a iesit din service mode");
        } else {
            Serial.println("[Actions] DUT: mb_00 fara ACK");
        }
    }
    return { ActionResult::OK, "back to start" };
}
