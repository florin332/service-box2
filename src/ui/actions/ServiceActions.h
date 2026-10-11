#ifndef SERVICE_ACTIONS_H
#define SERVICE_ACTIONS_H

#include <Arduino.h>
#include "ActionIntent.h"
#include "battery/IBatteryProvider.h"

// ============================================================================
// ServiceActions - executia efectiva a actiunilor svc box.
//
// Primeste evenimente de la Navigation (UiEvent) si le executa:
//   - START_CONFIRMED  -> pornire secventa de teste (deocamdata placeholder)
//   - FORCED_LOW_POWER -> oprire controlata (deocamdata doar log; la
//                         integrarea reala va opri motoare/teste active)
//   - BACK_TO_START    -> curatenie la iesire din dashboard (deocamdata nop)
//
// NU include Adafruit_GFX, nu deseneaza, nu cunoaste paginile.
// Depinde doar de BSP (bsp_battery, etc.) si de IBatteryProvider.
// ============================================================================

class ServiceActions {
public:
    explicit ServiceActions(IBatteryProvider& battery);

    // Executa evenimentul primit. Returneaza rezultatul pentru diagnostic.
    ActionOutcome execute(UiEvent ev);

    // Apelat periodic din loop (pump) pentru actiuni cu durata.
    // Deocamdata nu are nimic de executat continuu; va gazdui pipeline-ul
    // de teste atunci cand va fi implementat.
    void update();

private:
    IBatteryProvider& _battery;

    // Stare interna: test in desfasurare (pentru FORCED_LOW_POWER)
    bool _testActive;

    ActionOutcome handleStartConfirmed();
    ActionOutcome handleForcedLowPower();
    ActionOutcome handleBackToStart();
};

#endif // SERVICE_ACTIONS_H
