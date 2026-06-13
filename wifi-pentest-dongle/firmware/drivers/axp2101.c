/*
 * WFP-100 — AXP2101 PMIC Driver Implementation
 * I2C driver for X-Powers AXP2101 power management IC.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include "axp2101.h"
#include "../registers.h"

/* I2C read/write via JH7110 I2C0 controller */
static int i2c0_write(uint8_t dev_addr, uint8_t reg, uint8_t val)
{
    uintptr_t base = I2C0_BASE;
    uint32_t status;

    /* Set target address */
    mmio_write32(base + I2C_TAR, dev_addr);

    /* Write register address */
    mmio_write32(base + I2C_DR, reg);
    while (!(mmio_read32(base + I2C_SR) & (1U << 2)))
        ;

    /* Write data */
    mmio_write32(base + I2C_DR, val);
    while (!(mmio_read32(base + I2C_SR) & (1U << 2)))
        ;

    /* Check for NACK */
    status = mmio_read32(base + I2C_SR);
    if (status & (1U << 3))
        return -1;

    return 0;
}

static int i2c0_read(uint8_t dev_addr, uint8_t reg, uint8_t *val)
{
    uintptr_t base = I2C0_BASE;
    uint32_t status;

    /* Set target address */
    mmio_write32(base + I2C_TAR, dev_addr);

    /* Write register address (read phase) */
    mmio_write32(base + I2C_DR, reg);
    while (!(mmio_read32(base + I2C_SR) & (1U << 2)))
        ;

    /* Read data */
    status = mmio_read32(base + I2C_DR);
    while (!(status & (1U << 4)))
        status = mmio_read32(base + I2C_SR);

    *val = (uint8_t)(mmio_read32(base + I2C_DR) & 0xFF);
    return 0;
}

/* Initialize AXP2101 for WFP-100 power sequence */
int axp2101_init(void)
{
    int i, ret;

    for (i = 0; i < WFP100_NUM_RAILS; i++) {
        ret = i2c0_write(AXP2101_I2C_ADDR, wfp100_rails[i].reg,
                         wfp100_rails[i].value);
        if (ret)
            return ret;
    }

    return 0;
}

/* Enable or disable a specific power rail */
int axp2101_enable_rail(uint8_t reg, int enable)
{
    uint8_t val;
    int ret;

    ret = i2c0_read(AXP2101_I2C_ADDR, reg, &val);
    if (ret)
        return ret;

    if (enable)
        val |= (1U << 7);   /* Enable bit */
    else
        val &= ~(1U << 7);

    return i2c0_write(AXP2101_I2C_ADDR, reg, val);
}

/* Read power status register */
int axp2101_read_status(void)
{
    uint8_t val;
    int ret;

    ret = i2c0_read(AXP2101_I2C_ADDR, AXP2101_POWER_STATUS, &val);
    if (ret)
        return ret;

    return val;
}

/* Read battery voltage (returns millivolts, or negative on error) */
int axp2101_read_battery_voltage(void)
{
    uint8_t val;
    int ret;

    /* AXP2101 battery voltage register at 0x34 */
    ret = i2c0_read(AXP2101_I2C_ADDR, 0x34, &val);
    if (ret)
        return ret;

    /* Battery voltage = 2.6V + val * 10mV */
    return 2600 + (val * 10);
}