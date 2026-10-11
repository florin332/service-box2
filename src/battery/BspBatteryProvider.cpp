#include "BspBatteryProvider.h"
#include "bsp/bsp_ina219.h"
#include <EEPROM.h>

// ============================================================================
// Battery Manager comun (Marble + Waveshare) - vezi BspBatteryProvider.h
// ============================================================================

#define DEV_MODE_PIN 28   // Jumper DEVELOPMENT: INPUT_PULLUP, activ LOW (GND)

BspBatteryProvider::BspBatteryProvider()
    : _voltage_V(0.0f)
    , _current_mA(0.0f)
    , _state(BatteryState::CRITICAL)   // stare sigura pana la prima masuratoare
    , _pendingState(BatteryState::CRITICAL)
    , _pendingTicks(0)
    , _devMode(false)
    , _simVoltage_V(3.90f)             // default: NORMAL
    , _simCurrent_mA(0.0f)
    , _lastSampleMs(0)
{
}

void BspBatteryProvider::begin()
{
    pinMode(DEV_MODE_PIN, INPUT_PULLUP);
    delayMicroseconds(2000);  // stabilizare pull-up
    _devMode = (digitalRead(DEV_MODE_PIN) == LOW);

    EEPROM.begin(EEPROM_SIZE);
    if (_devMode) {
        loadSimState();
        _voltage_V  = _simVoltage_V;
        _current_mA = _simCurrent_mA;
        _state      = evaluate(_voltage_V, _current_mA);
    } else {
        sample();  // prima masuratoare reala, fara debounce
        _state = evaluate(_voltage_V, _current_mA);
    }
    _pendingState = _state;
    _pendingTicks = 0;

    Serial.print("[BatteryMgr] mode=");
    Serial.print(_devMode ? "DEVELOPMENT (GP28=0)" : "NORMAL (GP28=1)");
    Serial.print(" V=");
    Serial.print(_voltage_V, 2);
    Serial.print(" I=");
    Serial.print(_current_mA, 1);
    Serial.print(" state=");
    Serial.println((int)_state);
}

bool BspBatteryProvider::isDevelopmentMode() const
{
    return _devMode;
}

// ----------------------------------------------------------------------------
// Citire valori curente: reale (INA219) sau simulate (DEV)
// ----------------------------------------------------------------------------
void BspBatteryProvider::sample()
{
    if (_devMode) {
        _voltage_V  = _simVoltage_V;
        _current_mA = _simCurrent_mA;
        return;
    }

    ina219_data_t data;
    bsp_ina219_read(&data);
    _voltage_V  = data.bus_voltage_V;
    _current_mA = data.current_mA;
}

// ----------------------------------------------------------------------------
// Masina de stari - prioritate: CRITICAL > LOW > CHARGED > CHARGING > NORMAL
// LOW are histerezis: intrare la V <= 3.60, iesire numai la V >= 3.70.
// ----------------------------------------------------------------------------
BatteryState BspBatteryProvider::evaluate(float v, float i) const
{
    if (v <= V_CRITICAL_MAX) {
        return BatteryState::CRITICAL;
    }

    // Histerezis LOW: daca suntem deja in LOW, iesim doar la V >= 3.70.
    if (_state == BatteryState::LOW) {
        if (v < V_LOW_EXIT) {
            return BatteryState::LOW;
        }
    } else {
        if (v <= V_LOW_ENTER) {
            return BatteryState::LOW;
        }
    }

    if (v > V_CHARGE_MIN) {
        if (i > I_CHARGED_MIN_MA && i < I_CHARGED_MAX_MA) {
            return BatteryState::CHARGED;
        }
        if (i < I_CHARGING_MAX_MA) {
            return BatteryState::CHARGING;
        }
    }

    return BatteryState::NORMAL;
}

void BspBatteryProvider::transitionTo(BatteryState candidate)
{
    if (candidate == _state) {
        _pendingTicks = 0;
        return;
    }

    if (candidate != _pendingState) {
        _pendingState = candidate;
        _pendingTicks = 1;
        return;
    }

    if (++_pendingTicks >= STATE_DEBOUNCE_TICKS) {
        Serial.print("[BatteryMgr] state ");
        Serial.print((int)_state);
        Serial.print(" -> ");
        Serial.println((int)candidate);
        _state = candidate;
        _pendingTicks = 0;
    }
}

void BspBatteryProvider::update()
{
    unsigned long now = millis();
    if (now - _lastSampleMs >= SAMPLE_INTERVAL_MS) {
        _lastSampleMs = now;
        sample();
        transitionTo(evaluate(_voltage_V, _current_mA));
    }

    // Comenzi seriale active doar in DEVELOPMENT
    if (_devMode) {
        while (Serial.available()) {
            String cmd = Serial.readStringUntil('\n');
            cmd.trim();
            if (cmd.length() > 0) {
                processCommand(cmd);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// IBatteryProvider
// ----------------------------------------------------------------------------
uint8_t BspBatteryProvider::getLevelPercent()
{
    return voltageToPercent(_voltage_V);
}

bool BspBatteryProvider::isCharging()
{
    return (_state == BatteryState::CHARGING);
}

BatteryState BspBatteryProvider::getState()
{
    return _state;
}

float BspBatteryProvider::getVoltageV()
{
    return _voltage_V;
}

float BspBatteryProvider::getCurrentmA()
{
    return _current_mA;
}

uint8_t BspBatteryProvider::voltageToPercent(float v) const
{
    if (v <= V_PERCENT_MIN) return 0;
    if (v >= V_PERCENT_MAX) return 100;
    return (uint8_t)(((v - V_PERCENT_MIN) * 100.0f) / (V_PERCENT_MAX - V_PERCENT_MIN));
}

// ----------------------------------------------------------------------------
// Persistenta valorilor simulate (doar DEV)
// ----------------------------------------------------------------------------
void BspBatteryProvider::loadSimState()
{
    SimState s{};  // zero-init: magic/version valide doar daca EEPROM le-a suprascris
    EEPROM.get(SIM_EEPROM_OFFSET, s);

    if (s.magic != SIM_MAGIC || s.version != SIM_VERSION) {
        Serial.println("[BatteryMgr] no valid sim state, using defaults");
        _simVoltage_V  = 3.90f;
        _simCurrent_mA = 0.0f;
        saveSimState();
        return;
    }

    _simVoltage_V  = s.voltage_V;
    _simCurrent_mA = s.current_mA;

    Serial.print("[BatteryMgr] sim state loaded: V=");
    Serial.print(_simVoltage_V, 2);
    Serial.print(" I=");
    Serial.println(_simCurrent_mA, 1);
}

void BspBatteryProvider::saveSimState()
{
    SimState s;
    s.magic      = SIM_MAGIC;
    s.version    = SIM_VERSION;
    s.voltage_V  = _simVoltage_V;
    s.current_mA = _simCurrent_mA;

    EEPROM.put(SIM_EEPROM_OFFSET, s);
    bool ok = EEPROM.commit();

    Serial.print("[BatteryMgr] sim state saved, commit=");
    Serial.println(ok ? "OK" : "FAIL");
}

// ----------------------------------------------------------------------------
// Comenzi seriale (doar DEVELOPMENT)
// ----------------------------------------------------------------------------
void BspBatteryProvider::printStatus()
{
    Serial.print("[BatteryMgr] mode=");
    Serial.print(_devMode ? "DEV" : "NORMAL");
    Serial.print(" V=");
    Serial.print(_voltage_V, 2);
    Serial.print("V I=");
    Serial.print(_current_mA, 1);
    Serial.print("mA state=");
    switch (_state) {
        case BatteryState::NORMAL:   Serial.print("NORMAL");   break;
        case BatteryState::LOW:      Serial.print("LOW");      break;
        case BatteryState::CRITICAL: Serial.print("CRITICAL"); break;
        case BatteryState::CHARGING: Serial.print("CHARGING"); break;
        case BatteryState::CHARGED:  Serial.print("CHARGED");  break;
    }
    Serial.print(" level=");
    Serial.print(getLevelPercent());
    Serial.println("%");
}

void BspBatteryProvider::printHelp()
{
    Serial.println("[BatteryMgr] Commands (DEVELOPMENT only):");
    Serial.println("  bat setv <volts>     -> set simulated voltage, ex: bat setv 3.55");
    Serial.println("  bat seti <mA>        -> set simulated current, ex: bat seti -120");
    Serial.println("  bat preset critical  -> V=3.30V");
    Serial.println("  bat preset low       -> V=3.55V (enter LOW)");
    Serial.println("  bat preset recover   -> V=3.75V (exit LOW, hysteresis)");
    Serial.println("  bat preset normal    -> V=3.90V");
    Serial.println("  bat preset charging  -> V=4.10V, I=-120mA");
    Serial.println("  bat preset charged   -> V=4.15V, I=0mA");
    Serial.println("  bat status           -> print current state");
    Serial.println("  bat help             -> this list");
    Serial.println("Simulated values are persisted in EEPROM across reboots.");
}

void BspBatteryProvider::processCommand(const String& cmd)
{
    if (cmd.startsWith("bat setv ")) {
        float v = cmd.substring(9).toFloat();
        if (v >= 2.5f && v <= 5.0f) {
            _simVoltage_V = v;
            saveSimState();
            _voltage_V = _simVoltage_V;
            Serial.print("[BatteryMgr] sim V=");
            Serial.println(_simVoltage_V, 2);
            printStatus();
        } else {
            Serial.println("[BatteryMgr] ERROR: voltage must be 2.5..5.0 V");
        }
    }
    else if (cmd.startsWith("bat seti ")) {
        float i = cmd.substring(9).toFloat();
        if (i >= -500.0f && i <= 500.0f) {
            _simCurrent_mA = i;
            saveSimState();
            _current_mA = _simCurrent_mA;
            Serial.print("[BatteryMgr] sim I=");
            Serial.println(_simCurrent_mA, 1);
            printStatus();
        } else {
            Serial.println("[BatteryMgr] ERROR: current must be -500..500 mA");
        }
    }
    else if (cmd.equalsIgnoreCase("bat preset critical")) {
        _simVoltage_V = 3.30f; _simCurrent_mA = 0.0f;
        saveSimState(); _voltage_V = _simVoltage_V; _current_mA = _simCurrent_mA;
        Serial.println("[BatteryMgr] preset CRITICAL"); printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat preset low")) {
        _simVoltage_V = 3.55f; _simCurrent_mA = 0.0f;
        saveSimState(); _voltage_V = _simVoltage_V; _current_mA = _simCurrent_mA;
        Serial.println("[BatteryMgr] preset LOW"); printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat preset recover")) {
        _simVoltage_V = 3.75f; _simCurrent_mA = 0.0f;
        saveSimState(); _voltage_V = _simVoltage_V; _current_mA = _simCurrent_mA;
        Serial.println("[BatteryMgr] preset RECOVER (exit LOW hysteresis)"); printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat preset normal")) {
        _simVoltage_V = 3.90f; _simCurrent_mA = 0.0f;
        saveSimState(); _voltage_V = _simVoltage_V; _current_mA = _simCurrent_mA;
        Serial.println("[BatteryMgr] preset NORMAL"); printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat preset charging")) {
        _simVoltage_V = 4.10f; _simCurrent_mA = -120.0f;
        saveSimState(); _voltage_V = _simVoltage_V; _current_mA = _simCurrent_mA;
        Serial.println("[BatteryMgr] preset CHARGING"); printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat preset charged")) {
        _simVoltage_V = 4.15f; _simCurrent_mA = 0.0f;
        saveSimState(); _voltage_V = _simVoltage_V; _current_mA = _simCurrent_mA;
        Serial.println("[BatteryMgr] preset CHARGED"); printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat status")) {
        printStatus();
    }
    else if (cmd.equalsIgnoreCase("bat help")) {
        printHelp();
    }
}
