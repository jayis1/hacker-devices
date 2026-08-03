/*
 * fuel_gauge.c — MAX17048 LiPo fuel gauge driver
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Reads battery state-of-charge (percentage) and voltage from the
 * MAX17048 fuel gauge over I²C2. The MAX17048 uses a ModelGauge
 * algorithm to track battery capacity without requiring current sensing.
 */

#include <stdint.h>
#include "board.h"
#include "registers.h"

#define FG_BUS   I2C2_BUS
#define FG_ADDR  MAX17048_ADDR

/* ---- Init fuel gauge ---- */
int fuel_gauge_init(void) {
    /* Read version register to verify device presence */
    uint8_t buf[2];
    if (i2c_read_reg(FG_BUS, FG_ADDR, MAX17048_REG_VERSION, buf, 2) != 0) {
        return -1;
    }
    /* MAX17048 version should be 0x0040 */
    uint16_t version = ((uint16_t)buf[0] << 8) | buf[1];
    if (version != 0x0040u) {
        return -1; /* Wrong device or not present */
    }

    /* Quick-start: write 0x4000 to MODE register */
    uint8_t qs_buf[2] = { 0x40, 0x00 };
    i2c_write_reg(FG_BUS, FG_ADDR, 0x06u, qs_buf, 2);

    return 0;
}

/* ---- Read battery percentage ---- */
uint8_t fuel_gauge_read_percent(void) {
    uint8_t buf[2];
    if (i2c_read_reg(FG_BUS, FG_ADDR, MAX17048_REG_SOC, buf, 2) != 0) {
        return 0;
    }
    /* SOC register: 1 LSB = 1/256 % */
    uint16_t soc_raw = ((uint16_t)buf[0] << 8) | buf[1];
    return (uint8_t)(soc_raw / 256u);
}

/* ---- Read battery voltage ---- */
uint16_t fuel_gauge_read_voltage_mv(void) {
    uint8_t buf[2];
    if (i2c_read_reg(FG_BUS, FG_ADDR, MAX17048_REG_VCELL, buf, 2) != 0) {
        return 0;
    }
    /* VCELL register: 1 LSB = 78.125 µV, shifted right by 4 */
    uint16_t vcell_raw = (((uint16_t)buf[0] << 8) | buf[1]) >> 4;
    /* Voltage in mV = vcell_raw * 0.078125 ≈ vcell_raw * 78 / 1000 */
    return (uint16_t)((uint32_t)vcell_raw * 78u / 1000u);
}