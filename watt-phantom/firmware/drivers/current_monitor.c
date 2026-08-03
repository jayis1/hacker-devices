/*
 * current_monitor.c — INA226 voltage/current/power monitor driver
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Reads VBUS voltage, current, and power from the INA226 on each port.
 * The INA226 is a 16-bit, bi-directional current/power monitor with
 * an I²C interface. It measures both bus voltage (VBUS) and shunt
 * voltage (across a shunt resistor) to compute current and power.
 *
 * Configuration:
 *   - Shunt resistor: 0.005 Ω (5 mΩ) on each VBUS path
 *   - INA226 ADC: 16-bit, 1.1 ms conversion time
 *   - Averaging: 16 samples
 *   - Alert pin: configured for OVP/OCP watchdog
 */

#include <stdint.h>
#include "board.h"
#include "registers.h"

/* ---- INA226 per-port config ---- */
static uint32_t s_ina_bus[PD_PORT_COUNT] = { I2C1_BUS, I2C2_BUS };
static uint8_t s_ina_addr[PD_PORT_COUNT] = { INA226_ADDR_SRC, INA226_ADDR_SNK };

/* ---- Shunt resistor value (mΩ) ---- */
#define SHUNT_RESISTOR_MOHM  5u   /* 5 mΩ */
#define INA226_CALIB_VALUE   2048u /* Calibration register value */

/* ---- Init INA226 on both ports ---- */
void current_monitor_init(void) {
    for (int p = 0; p < PD_PORT_COUNT; p++) {
        /* Configure INA226:
         * - Config register: 16-sample averaging, 1.1ms VBUS conv, 1.1ms VSH conv
         * - Mode: continuous shunt+bus
         * Config = AVG=16 (0x4<<9) | VBUSCT=1.1ms (0x4<<3) | VSHCT=1.1ms (0x4<<6) | MODE=7
         * = 0x4<<9 | 0x4<<6 | 0x4<<3 | 7 = 0x45C7
         */
        uint16_t config = 0x45C7u;
        uint8_t cfg_buf[2] = { (uint8_t)(config >> 8), (uint8_t)(config & 0xFF) };
        i2c_write_reg(s_ina_bus[p], s_ina_addr[p], INA226_REG_CONFIG, cfg_buf, 2);

        /* Set calibration register (enables current/power readings) */
        uint16_t calib = INA226_CALIB_VALUE;
        uint8_t cal_buf[2] = { (uint8_t)(calib >> 8), (uint8_t)(calib & 0xFF) };
        i2c_write_reg(s_ina_bus[p], s_ina_addr[p], INA226_REG_CALIB, cal_buf, 2);

        /* Set default alert mask: alert on OVP (bus voltage over limit) */
        /* Mask/Enable register: bit 4 = BUS under-voltage, bit 2 = bus over-voltage */
        /* We use the ALERT pin as an interrupt to the MCU */
        uint16_t mask = 0x0004u; /* BUS over-voltage triggers alert */
        uint8_t mask_buf[2] = { (uint8_t)(mask >> 8), (uint8_t)(mask & 0xFF) };
        i2c_write_reg(s_ina_bus[p], s_ina_addr[p], INA226_REG_MASK, mask_buf, 2);
    }
}

/* ---- Read a 16-bit INA226 register ---- */
static uint16_t ina_read16(pd_port_t port, uint8_t reg) {
    uint8_t buf[2];
    if (i2c_read_reg(s_ina_bus[port], s_ina_addr[port], reg, buf, 2) != 0) {
        return 0;
    }
    return ((uint16_t)buf[0] << 8) | buf[1];
}

/* ---- Read voltage, current, and power for a port ---- */
int current_monitor_read(pd_port_t port, uint16_t *voltage_mv,
                         int16_t *current_ma, uint16_t *power_mw) {
    /* Bus voltage register: 1 LSB = 1.25 mV */
    uint16_t bus_raw = ina_read16(port, INA226_REG_BUS_V);
    *voltage_mv = (uint16_t)((uint32_t)bus_raw * 1250u / 1000u);

    /* Shunt voltage register: 1 LSB = 2.5 µV, signed */
    int16_t shunt_raw = (int16_t)ina_read16(port, INA226_REG_SHUNT_V);
    /* Current = shunt_voltage / shunt_resistor
     * shunt_voltage in µV = shunt_raw * 2.5
     * current in mA = shunt_voltage_µV / (shunt_resistor_mΩ * 1000)
     * = shunt_raw * 2.5 / (5 * 1000) = shunt_raw * 0.0005
     * = shunt_raw / 2000
     */
    *current_ma = (int16_t)((int32_t)shunt_raw * 25u / 50000u);
    /* Simplified: *current_ma = shunt_raw / 200; */

    /* Power register: 1 LSB = 25 * current_lsb = 25 * 0.0005 = 0.0125 W = 12.5 mW */
    uint16_t power_raw = ina_read16(port, INA226_REG_POWER);
    *power_mw = (uint16_t)((uint32_t)power_raw * 125u / 10u);

    return 0;
}

/* ---- Set alert thresholds for OVP/OCP ---- */
void current_monitor_set_alert(pd_port_t port, uint16_t ovp_mv, uint16_t ocp_ma) {
    /* Convert OVP to INA226 bus voltage register value: 1 LSB = 1.25 mV */
    uint16_t alert_val = (uint16_t)((uint32_t)ovp_mv * 1000u / 1250u);
    uint8_t buf[2] = { (uint8_t)(alert_val >> 8), (uint8_t)(alert_val & 0xFF) };

    /* Set alert limit register */
    i2c_write_reg(s_ina_bus[port], s_ina_addr[port], INA226_REG_ALERT, buf, 2);

    /* Configure mask register: alert on bus over-voltage */
    uint16_t mask = 0x0004u; /* CNVR bit + BUS over-voltage */
    buf[0] = (uint8_t)(mask >> 8);
    buf[1] = (uint8_t)(mask & 0xFF);
    i2c_write_reg(s_ina_bus[port], s_ina_addr[port], INA226_REG_MASK, buf, 2);

    (void)ocp_ma; /* OCP is handled by the eFuse hardware */
}