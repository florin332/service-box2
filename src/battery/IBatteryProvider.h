#ifndef I_BATTERY_PROVIDER_H
#define I_BATTERY_PROVIDER_H

#include <Arduino.h>

// ============================================================================
// Stare bateriei determinată de Battery Manager (hardware-dependent).
// UI-ul / sistemul de navigație aplică politica în funcție de această stare.
//
//   NORMAL   - V > 3.60 V, fără încărcare; operațiuni normale.
//   LOW      - 3.40 V < V <= 3.60 V; avertizare, fără teste noi (histerezis:
//              iesire numai la V >= 3.70 V).
//   CRITICAL - V <= 3.40 V; sistemul întrerupe controlat testele și revine
//              la Battery Check.
//   CHARGING - V > 4.00 V și I < -50 mA; încărcare activă.
//   CHARGED  - V > 4.00 V și -0.5 mA < I < 5 mA; încărcare finalizată.
// ============================================================================
enum class BatteryState {
    NORMAL,
    LOW,
    CRITICAL,
    CHARGING,
    CHARGED
};

// ============================================================================
// Interfață abstractă pentru furnizorul de stare baterie.
//
// Pagina BatteryCheckScreen primește o referință la această interfață și nu
// depinde de implementarea concretă (stub sau driver hardware real).
//
// La integrarea pe branch-urile hardware marble/waveshare, se va implementa
// o clasă reală (ex. BspBatteryProvider) care respectă această interfață și
// se va înlocui instanța stub în main.cpp, fără modificări în UI.
// ============================================================================

class IBatteryProvider {
public:
    virtual ~IBatteryProvider() = default;

    // Returnează nivelul bateriei în procente (0 .. 100).
    virtual uint8_t getLevelPercent() = 0;

    // Returnează true dacă bateria este în curs de încărcare.
    virtual bool isCharging() = 0;

    // Returnează starea discretă a bateriei.
    // Stabilirea stării ține de Battery Manager (hardware-dependent); UI-ul
    // aplică doar politica asociată stării curente.
    virtual BatteryState getState() = 0;

    // Returnează tensiunea bateriei în volți (afișată de UI în funcționare
    // normală sau CHARGING/CHARGED).
    virtual float getVoltageV() = 0;

    // Returnează curentul în mA (negativ = încărcare).
    virtual float getCurrentmA() = 0;
};

#endif // I_BATTERY_PROVIDER_H
