#ifndef SERVICE_ACTIONS_H
#define SERVICE_ACTIONS_H

#include <Arduino.h>
#include "ActionIntent.h"
#include "battery/IBatteryProvider.h"

// ============================================================================
// ServiceActions - executia efectiva a actiunilor svc box.
//
// Primeste evenimente de la Navigation (UiEvent) si le executa:
//   - START_CONFIRMED  -> "mb_01" catre DUT (SRV ENTER: intra in Service
//                         Mode); placeholder pentru pipeline-ul de teste
//   - BACK_TO_START    -> "mb_00" catre DUT (SRV EXIT: iese din Service
//                         Mode) la SVC OUT / revenire la START
//   - FORCED_LOW_POWER -> oprire controlata (deocamdata doar log; la
//                         integrarea reala va opri motoare/teste active)
//
// Comenzile catre DUT merg pe USB-ul host PIO (dut_protocol) si sunt
// conditionate de legatura CDC: fara DUT conectat, sunt no-op cu log.
//
// NU include Adafruit_GFX, nu deseneaza, nu cunoaste paginile.
// Depinde doar de BSP (bsp_battery, etc.), IBatteryProvider si dut/*.
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
