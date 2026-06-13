/*
 * WFP-100 — AXP2101 PMIC Driver
 * I2C driver for X-Powers AXP2101 power management IC.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef WFP100_AXP2101_H
#define WFP100_AXP2101_H

#include <stdint.h>

/* AXP2101 I2C address (7-bit) */
#define AXP2101_I2C_ADDR        0x35

/* AXP2101 Register Map */
#define AXP2101_POWER_STATUS     0x00
#define AXP2101_CHARGE_STATUS    0x01
#define AXP2101_DCDC1_CTRL       0x10
#define AXP2101_DCDC2_CTRL       0x11
#define AXP2101_DCDC3_CTRL       0x12
#define AXP2101_DCDC4_CTRL       0x13
#define AXP2101_LDO1_CTRL        0x20
#define AXP2101_LDO2_CTRL        0x21
#define AXP2101_GPIO0_CTRL        0x30
#define AXP2101_IRQ_ENABLE1       0x40
#define AXP2101_IRQ_STATUS1       0x48

/* Voltage rail definitions for WFP-100 */
struct axp2101_rail {
    const char *name;
    uint8_t reg;
    uint8_t value;
    uint32_t voltage_uv;  /* Microvolts */
};

static const struct axp2101_rail wfp100_rails[] = {
    { "VDD_CORE", AXP2101_DCDC1_CTRL, 0x20,  900000  },
    { "VDD_DDR",  AXP2101_DCDC2_CTRL, 0x2C, 1100000  },
    { "VDD_SOC",  AXP2101_DCDC3_CTRL, 0x24, 1000000  },
    { "VDD_3V3",  AXP2101_DCDC4_CTRL, 0x46, 3300000  },
    { "VDD_1V8",  AXP2101_LDO1_CTRL,  0x1A, 1800000  },
    { "VDD_RTC",  AXP2101_LDO2_CTRL,  0x08,  800000  },
};

#define WFP100_NUM_RAILS  (sizeof(wfp100_rails) / sizeof(wfp100_rails[0]))

/* Function prototypes */
int axp2101_init(void);
int axp2101_set_voltage(uint8_t reg, uint32_t voltage_uv);
uint32_t axp2101_get_voltage(uint8_t reg);
int axp2101_enable_rail(uint8_t reg, int enable);
int axp2101_read_status(void);
int axp2101_read_battery_voltage(void);

#endif /* WFP100_AXP2101_H */