#ifndef BSP_BATTERY_PROVIDER_H
#define BSP_BATTERY_PROVIDER_H

#include <Arduino.h>
#include "IBatteryProvider.h"

// ============================================================================
// Battery Manager comun pentru Marble Pico si Waveshare RP2350.
//
// Sursa unica de masurare: INA219 (I2C, adresa 0x40). NU se folosesc
// ADC-ul bateriei (GP27) sau pinii digitali GP23/GP24 pentru decizii.
//
// Masina de stari (conform politicii bateriei):
//
//   CRITICAL  V <= 3.40 V
//             Blocheaza testele; main.cpp revine fortat la Battery Check.
//
//   LOW       3.40 V < V <= 3.60 V
//             Iesire numai la V >= 3.70 V (histerezis).
//             Avertizare; blocheaza pornirea testelor noi.
//
//   NORMAL    V > 3.60 V (si nu e incarcare)
//             Functionare normala.
//
//   CHARGING  V > 4.00 V si I < -50 mA (curent negativ = incarcare)
//             Semnaleaza incarcare activa.
//
//   CHARGED   V > 4.00 V si -0.5 mA < I < 5 mA (curent ~0 la V ridicata)
//             Semnaleaza finalizarea incarcarii.
//
// Prioritate stari (evaluate in aceasta ordine):
//   CRITICAL > LOW > CHARGED > CHARGING > NORMAL
//
// Filtrare: o tranzitie de stare este acceptata numai dupa
// STATE_DEBOUNCE_TICKS citiri consecutive care o cer (anti-oscilatie).
//
// ----------------------------------------------------------------------------
// Jumper DEVELOPMENT pe GP28 (INPUT_PULLUP):
//   GP28 = 0 (la GND) -> mod DEVELOPMENT
//   GP28 = 1 (pull-up) -> functionare normala
//
// In mod DEVELOPMENT valorile INA219 reale sunt inlocuite cu valori
// simulate (V, I) configurabile din cod sau din serial. Masina de stari
// ruleaza identic pe datele simulate. Valorile simulate sunt persistate
// in EEPROM si supravietuiesc reboot-ului.
//
// Comenzi seriale (active DOAR in mod DEVELOPMENT):
//   bat setv <volts>      ex: bat setv 3.55
//   bat seti <mA>         ex: bat seti -120
//   bat preset critical   V=3.30V
//   bat preset low        V=3.55V
//   bat preset recover    V=3.75V (test histerezis iesire LOW)
//   bat preset normal     V=3.90V
//   bat preset charging   V=4.10V, I=-120mA
//   bat preset charged    V=4.15V, I=0mA
//   bat status            afiseaza V, I, stare, mod (DEV/NORMAL)
//   bat help              lista comenzi
// ============================================================================

class BspBatteryProvider : public IBatteryProvider {
public:
    // Constructor: NU acceseaza hardware. begin() trebuie apelat din setup().
    BspBatteryProvider();

    // Initializeaza pinul GP28 (jumper DEV), EEPROM si citeste prima masuratoare.
    // Trebuie apelat dupa bsp_ina219_init().
    void begin();

    // Citeste INA219 (sau aplica valorile simulate in DEV), actualizeaza
    // masina de stari si proceseaza comenzile seriale.
    // Apelat in fiecare iteratie loop().
    void update();

    // IBatteryProvider
    uint8_t     getLevelPercent() override;
    bool        isCharging() override;
    BatteryState getState() override;
    float       getVoltageV() override;
    float       getCurrentmA() override;

    // true daca jumperul GP28 este la GND (mod DEVELOPMENT activ).
    bool isDevelopmentMode() const;

private:
    // ---------------- Masuratori curente (reale sau simulate) --------------
    float _voltage_V;
    float _current_mA;

    // ---------------- Stare discretizata -----------------------------------
    BatteryState _state;
    BatteryState _pendingState;      // candidat in curs de debounce
    uint8_t      _pendingTicks;

    // ---------------- Mod DEVELOPMENT --------------------------------------
    bool  _devMode;
    float _simVoltage_V;
    float _simCurrent_mA;

    // ---------------- Timing ------------------------------------------------
    unsigned long _lastSampleMs;

    // ---------------- Constante politica baterie ----------------------------
    static constexpr float V_CRITICAL_MAX     = 3.40f;   // V <= -> CRITICAL
    static constexpr float V_LOW_ENTER        = 3.60f;   // V <= -> LOW
    static constexpr float V_LOW_EXIT         = 3.70f;   // V >= -> iesire LOW
    static constexpr float V_CHARGE_MIN       = 4.00f;   // V >  -> zona incarcare
    static constexpr float I_CHARGING_MAX_MA  = -50.0f;  // I <  -> CHARGING
    static constexpr float I_CHARGED_MIN_MA   = -0.5f;   // I >  -> CHARGED (cu V>4V)
    static constexpr float I_CHARGED_MAX_MA   = 5.0f;    // I <  -> CHARGED

    // Mapare tensiune -> procent (liniara): 3.40 V = 0 %, 4.20 V = 100 %
    static constexpr float V_PERCENT_MIN = 3.40f;
    static constexpr float V_PERCENT_MAX = 4.20f;

    // Sampling INA219 si debounce tranzitii
    static constexpr unsigned long SAMPLE_INTERVAL_MS = 500UL;
    static constexpr uint8_t STATE_DEBOUNCE_TICKS = 4;   // ~2 s la 500 ms

    // ---------------- Persistenta EEPROM (doar valorile simulate DEV) -------
    // Offset 512 (aceeasi zona ca fostul stub, dar format nou).
    static constexpr uint16_t SIM_EEPROM_OFFSET = 512;
    static constexpr uint16_t EEPROM_SIZE       = (SIM_EEPROM_OFFSET + 16);
    static constexpr uint32_t SIM_MAGIC         = 0x424D4752;  // "BMGR"
    static constexpr uint16_t SIM_VERSION       = 1;

    struct __attribute__((packed)) SimState {
        uint32_t magic;
        uint16_t version;
        float    voltage_V;
        float    current_mA;
    };

    void loadSimState();
    void saveSimState();

    // ---------------- Interne ------------------------------------------------
    void sample();
    BatteryState evaluate(float v, float i) const;
    void transitionTo(BatteryState candidate);
    uint8_t voltageToPercent(float v) const;
    void processCommand(const String& cmd);
    void printStatus();
    void printHelp();
};

#endif // BSP_BATTERY_PROVIDER_H
