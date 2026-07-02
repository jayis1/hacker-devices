/*
 * power.c — nPM1300 PMIC + status LED
 * Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
 *
 * The nPM1300 PMIC controls the main power rails. Critically, the RF rail
 * (BLE/Wi-Fi) is on a dedicated switch so it can be hard-disabled during
 * covert capture. The single status LED is an RGB driven via RMT.
 */
#include <stdint.h>
#include "../board.h"
#include "power.h"

__attribute__((weak)) int  i2c_write(uint8_t p, uint8_t a, const uint8_t *d, uint32_t l) { (void)p;(void)a;(void)d;(void)l; return 0; }
__attribute__((weak)) int  i2c_read(uint8_t p, uint8_t a, uint8_t *d, uint32_t l) { (void)p;(void)a;(void)d;(void)l; return 0; }
__attribute__((weak)) void rmt_rgb_set(uint8_t r, uint8_t g, uint8_t b) { (void)r;(void)g;(void)b; }

static uint32_t g_rails = 0;

static void pmic_write(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    i2c_write(PMIC_I2C_PORT, PMIC_ADDR, buf, 2);
}

static uint8_t pmic_read(uint8_t reg)
{
    uint8_t v = 0;
    i2c_read(PMIC_I2C_PORT, PMIC_ADDR, &v, 1);
    (void)reg;
    return v;
}

int power_init(void)
{
    /* Enable ESP peripheral domain and FPGA I/O at boot; leave analog + RF off. */
    g_rails = RAIL_ESP_PERIPH | RAIL_FPGA_IO | RAIL_USB;
    pmic_write(0x03, 0x01);  /* main enable */
    pmic_write(0x10, 0x0A);  /* ESP + USB rails on */
    pmic_write(0x12, 0x00);  /* analog + RF off */
    return 0;
}

void power_rail_set(uint32_t mask)
{
    g_rails |= mask;
    /* Map rail mask to PMIC register bits (simplified). */
    uint8_t v = 0;
    if (g_rails & RAIL_SPAD_ANALOG) v |= 0x01;
    if (g_rails & RAIL_FPGA_CORE)   v |= 0x02;
    if (g_rails & RAIL_FPGA_IO)     v |= 0x04;
    if (g_rails & RAIL_RF)          v |= 0x08;
    pmic_write(0x12, v);
}

void power_rail_clear(uint32_t mask)
{
    g_rails &= ~mask;
    uint8_t v = 0;
    if (g_rails & RAIL_SPAD_ANALOG) v |= 0x01;
    if (g_rails & RAIL_FPGA_CORE)   v |= 0x02;
    if (g_rails & RAIL_FPGA_IO)     v |= 0x04;
    if (g_rails & RAIL_RF)          v |= 0x08;
    pmic_write(0x12, v);
}

uint32_t power_rail_state(void) { return g_rails; }

void power_status_led(uint8_t r, uint8_t g, uint8_t b)
{
    /* In cover mode the caller should pass (1,0,0) dim. */
    rmt_rgb_set(r, g, b);
}

uint8_t power_battery_pct(void)
{
    /* nPM1300 fuel gauge via I2C. */
    uint8_t raw = pmic_read(0x20);
    return raw > 100 ? 100 : raw;
}