#ifndef __BSP_INA219_H__
#define __BSP_INA219_H__

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// INA219 - monitor de curent / tensiune / putere pe I2C1 (Waveshare RP2350)
//
// Imparte magistrala I2C1 (SDA=GP6, SCL=GP7, 400 kHz) cu touch-ul CST328
// si IMU QMI8658. Adresa 0x40 corespunde modulelor cu A0=A1=GND.
//
// Calibrarea de mai jos presupune shunt R100 (0.1 ohm), uzual pe module:
//   - domeniu tensiune bus: 32 V
//   - domeniu tensiune shunt: +/-320 mV (PGA /8)
//   - rezolutie curent: 100 uA/LSB  -> domeniu +/-3.2 A
//   - rezolutie putere: 2 mW/LSB
// ============================================================================

#define INA219_DEVICE_ADDR 0x40

// Registri INA219
typedef enum
{
    INA219_REG_CONFIG       = 0x00,  // configurare (R/W)
    INA219_REG_SHUNT_VOLT   = 0x01,  // tensiune shunt (R, signed)
    INA219_REG_BUS_VOLT     = 0x02,  // tensiune bus (R)
    INA219_REG_POWER        = 0x03,  // putere (R)
    INA219_REG_CURRENT      = 0x04,  // curent (R, signed)
    INA219_REG_CALIBRATION  = 0x05   // calibrare (R/W)
} ina219_reg_t;

// CONFIG: BRNG=32V | PGA=/8 (320mV) | BADC=12bit | SADC=12bit | MODE=shunt+bus continuu
#define INA219_CONFIG_VALUE 0x399F

// Calibrare pentru R_shunt = 0.1 ohm si Current_LSB = 100 uA:
//   Cal = trunc(0.04096 / (Current_LSB * R_shunt)) = trunc(0.04096 / 1e-5) = 4096
#define INA219_CALIB_VALUE   4096
#define INA219_CURRENT_LSB_MA 0.1f   // 100 uA = 0.1 mA
#define INA219_POWER_LSB_MW   2.0f   // 20 * Current_LSB

// Sensul curentului depinde de orientarea shunt-ului (IN+/IN-) pe montaj.
// 1 = curent pozitiv la INCARCARE (validat pe hardware: inainte de inversare,
//     la incarcare se citea -130 mA).
#define INA219_INVERT_CURRENT 1

typedef struct
{
    float bus_voltage_V;    // tensiunea masurata pe bus (V)
    float shunt_voltage_mV; // caderea de tensiune pe shunt (mV)
    float current_mA;       // curentul prin shunt (mA, semnat)
    float power_mW;         // puterea (mW)
} ina219_data_t;

// Initializeaza INA219 (config + calibrare).
// Returneaza true daca device-ul raspunde la adresa 0x40.
bool bsp_ina219_init(void);

// Citeste toate marimile (bus, shunt, curent, putere) intr-o singura structura.
void bsp_ina219_read(ina219_data_t *data);

// Citiri individuale
float bsp_ina219_get_bus_voltage_V(void);
float bsp_ina219_get_shunt_voltage_mV(void);
float bsp_ina219_get_current_mA(void);
float bsp_ina219_get_power_mW(void);

#endif /* __BSP_INA219_H__ */
