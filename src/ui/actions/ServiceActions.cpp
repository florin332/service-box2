#include "ServiceActions.h"
#include "battery/IBatteryProvider.h"

#include "dut/dut_host.h"
#include "dut/dut_protocol.h"

// Definit mai jos, folosit de handleBackToStart().
static bool exitServiceWithRecovery();

// Re-enumerare + asteptare re-atasare + pauza de stabilizare.
// Pauza de ~1.5s dupa attach a fost necesara in teste: un retry trimis
// prea repede dupa enumerare inca se loveste de DUT-ul ocupat.
static bool reenumerateAndWait()
{
    dut_host_force_reenumerate();

    uint32_t deadline = millis() + 4000;
    while (!dut_cdc_connected() && (int32_t)(deadline - millis()) > 0)
        delay(20);

    if (!dut_cdc_connected())
        return false;

    delay(1500);   // stabilizare post-attach (vedea sonda: +5s a mers)
    return true;
}

// mb_01 cu recuperare: la esec (link mort dupa inactivitate) se forteaza
// re-enumerarea si se reincearca o data pe link-ul proaspat.
static bool enterServiceWithRecovery(char* identity, size_t len)
{
    if (dut_enter_service(identity, len))
        return true;

    Serial.println("[Actions] mb_01 esuat, re-enumerare DUT si retry...");
    if (!reenumerateAndWait())
        return false;
    return dut_enter_service(identity, len);
}

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
        case UiEvent::START_CONFIRMED: {
            // Handshake la apasarea START (nu la intrarea in pagina):
            // "mb_01" pune DUT-ul in Service Mode si intoarce identitatea.
            // Blocant max ~2.5s, doar daca DUT-ul raspunde. Fara DUT
            // conectat comanda se sare - navigarea nu este blocata.
            //
            // Recuperare la esec: dupa perioade lungi de inactivitate link-ul
            // CDC poate muri la nivel de bus (DUT-ul nu mai trimite nimic -
            // nici macar linii de log "DUT RX:" - iar handshake-ul expira).
            // Resetul boxului repara situatia prin re-enumerare; facem acelasi
            // lucru software: detach fortat + asteptare re-attach + retry o data.
            if (dut_cdc_connected()) {
                char identity[24];
                if (enterServiceWithRecovery(identity, sizeof(identity))) {
                    Serial.printf("[Actions] START handshake: %s\n", identity);
                } else {
                    Serial.println("[Actions] START: handshake esuat definitiv");
                }
            }
            _testActive = true;
            Serial.println("[Actions] START_CONFIRMED -> dashboard");
            return { ActionResult::OK, "test started (placeholder)" };
        }
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

// mb_00 cu recuperare. Imediat dupa mb_01 panoul este ocupat cu frame-ul
// de service si nu consuma OUT (observat in teste: "TX blocat" la retry
// imediat); o pauza de ~5s intre incercari este suficienta. Daca nici dupa
// doua incercari nu trece, re-enumerarea (bus reset -> FIFO golit) plus un
// CRLF gol (inchide un eventual rest de linie in parserul panoului) este
// ultima solutie.
static bool exitServiceWithRecovery()
{
    static const uint32_t GAPS_MS[] = { 0, 5000 };
    for (uint8_t i = 0; i < 2; i++) {
        if (GAPS_MS[i] > 0)
            delay(GAPS_MS[i]);
        if (!dut_cdc_connected())
            break;
        if (dut_exit_service())
            return true;
    }

    Serial.println("[Actions] mb_00: re-enumerare + CRLF flush + retry");
    if (!reenumerateAndWait())
        return false;

    // CRLF gol: inchide un eventual rest de linie in parserul panoului.
    dut_cdc_write_line("");
    delay(500);
    dut_cdc_flush_rx();

    return dut_exit_service();
}

ActionOutcome ServiceActions::handleBackToStart()
{
    // SVC OUT (sau orice revenire la START): DUT iese din Service Mode
    // ("mb_00" = alias SRV EXIT) si revine la functionarea normala.
    if (dut_cdc_connected()) {
        if (exitServiceWithRecovery()) {
            Serial.println("[Actions] DUT a iesit din service mode");
        } else {
            Serial.println("[Actions] DUT: mb_00 fara ACK (inclusiv retry)");
        }
    }
    return { ActionResult::OK, "back to start" };
}
