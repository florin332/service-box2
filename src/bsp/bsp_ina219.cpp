#include "bsp_ina219.h"
#include "bsp_i2c.h"

#include <stdio.h>

// ----------------------------------------------------------------------------
// Acces la registri: INA219 foloseste adrese de registru pe 8 biti si
// valori pe 16 biti, big-endian (MSB primul).
// ----------------------------------------------------------------------------

static bool bsp_ina219_reg_write(uint8_t reg_addr, uint16_t value)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)(value >> 8);
    buf[1] = (uint8_t)(value & 0xFF);
    return bsp_i2c_write_reg8(INA219_DEVICE_ADDR, reg_addr, buf, 2) == 3;
}

static bool bsp_ina219_reg_read(uint8_t reg_addr, uint16_t *value)
{
    uint8_t buf[2] = {0, 0};
    if (bsp_i2c_read_reg8(INA219_DEVICE_ADDR, reg_addr, buf, 2) != 2)
        return false;
    *value = (uint16_t)((buf[0] << 8) | buf[1]);
    return true;
}

bool bsp_ina219_init(void)
{
    // INA219 nu are registru WHO_AM_I; verificam prezenta prin
    // scriere + citire a registrului CONFIG. Un device absent NAK-eaza
    // si functiile de mai sus intorc false (fara blocaj).
    if (!bsp_ina219_reg_write(INA219_REG_CONFIG, INA219_CONFIG_VALUE))
    {
        printf("INA219 not found at 0x%02X (write NAK)!\r\n", INA219_DEVICE_ADDR);
        return false;
    }

    uint16_t cfg = 0;
    if (!bsp_ina219_reg_read(INA219_REG_CONFIG, &cfg) || cfg != INA219_CONFIG_VALUE)
    {
        printf("INA219 not found at 0x%02X (read-back fail)!\r\n", INA219_DEVICE_ADDR);
        return false;
    }

    bsp_ina219_reg_write(INA219_REG_CALIBRATION, INA219_CALIB_VALUE);

    printf("Find INA219 at 0x%02X!\r\n", INA219_DEVICE_ADDR);
    return true;
}

float bsp_ina219_get_bus_voltage_V(void)
{
    // Bitii 15..3 = tensiune, LSB = 4 mV (bitii 2..0 sunt flag-uri CNVR/OVF)
    uint16_t raw = 0;
    bsp_ina219_reg_read(INA219_REG_BUS_VOLT, &raw);
    return (float)(raw >> 3) * 0.004f;
}

float bsp_ina219_get_shunt_voltage_mV(void)
{
    // Valoare semnata, LSB = 10 uV
    uint16_t raw = 0;
    bsp_ina219_reg_read(INA219_REG_SHUNT_VOLT, &raw);
    float v = (float)(int16_t)raw * 0.01f;
#if INA219_INVERT_CURRENT
    v = -v;
#endif
    return v;
}

float bsp_ina219_get_current_mA(void)
{
    // Valoare semnata; trebuie sa fie programata calibrarea inainte
    uint16_t raw = 0;
    bsp_ina219_reg_read(INA219_REG_CURRENT, &raw);
    float i = (float)(int16_t)raw * INA219_CURRENT_LSB_MA;
#if INA219_INVERT_CURRENT
    i = -i;
#endif
    return i;
}

float bsp_ina219_get_power_mW(void)
{
    uint16_t raw = 0;
    bsp_ina219_reg_read(INA219_REG_POWER, &raw);
    return (float)raw * INA219_POWER_LSB_MW;
}

void bsp_ina219_read(ina219_data_t *data)
{
    data->bus_voltage_V    = bsp_ina219_get_bus_voltage_V();
    data->shunt_voltage_mV = bsp_ina219_get_shunt_voltage_mV();
    data->current_mA       = bsp_ina219_get_current_mA();
    data->power_mW         = bsp_ina219_get_power_mW();
}
