/*
 * power.c — fuel gauge + charger management
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Reads the BQ25896 charger / fuel gauge over I2C4. The BQ25896 reports
 * battery percentage, charge current, and fault status.
 */

#include "power.h"
#include "board.h"
#include "registers.h"

#define BQ25896_ADDR  0x6Bu   /* 7-bit I2C address (>>1 for the W bit) */

static int g_battery_pct = 100;
static int g_low = 0;

/* ----------------------------------------------------------------------- */
/*  I2C4 low-level                                                          */
/* ----------------------------------------------------------------------- */

static void i2c4_hw_init(void) {
    volatile uint32_t *apb4enr = (volatile uint32_t *)(RCC_BASE + 0x0E4u);
    *apb4enr |= (1u << 2);   /* I2C4EN (bit 2 on APB4) */
    I2C4->TIMINGR = 0x10909CECu;
    I2C4->CR1 = I2C_CR1_PE;
}

static int i2c4_read_reg(uint8_t reg, uint8_t *val) {
    /* Write the register pointer, then a repeated-start read of 1 byte. */
    I2C4->CR2 = ((uint32_t)BQ25896_ADDR << 1)             /* write */
             | (1u << I2C_CR2_NBYTES_SHIFT)
             | I2C_CR2_START | I2C_CR2_RELOAD;
    while ((I2C4->ISR & I2C_ISR_TXIS) == 0u) {
        if (I2C4->ISR & I2C_ISR_NACKF) return -1;
    }
    I2C4->TXDR = reg;
    /* Repeated start for read */
    I2C4->CR2 = ((uint32_t)(BQ25896_ADDR << 1) | 1u)       /* read */
             | (1u << I2C_CR2_NBYTES_SHIFT)
             | I2C_CR2_AUTOEND | I2C_CR2_START;
    while ((I2C4->ISR & I2C_ISR_RXNE) == 0u) {
        if (I2C4->ISR & I2C_ISR_NACKF) return -1;
    }
    *val = (uint8_t)I2C4->RXDR;
    while ((I2C4->ISR & I2C_ISR_STOPF) == 0u) { /* spin */ }
    I2C4->ICR = I2C_ISR_STOPF;
    return 0;
}

/* ----------------------------------------------------------------------- */
/*  Init + poll                                                              */
/* ----------------------------------------------------------------------- */

void power_init(void) {
    i2c4_hw_init();
    /* Read the chip ID register (0x14) to confirm the BQ25896 is present. */
    uint8_t id = 0u;
    if (i2c4_read_reg(0x14u, &id) == 0 && (id & 0xC0u) != 0u) {
        /* Chip present; set charge current limit to 2A (register 0x02). */
        /* (Stubbed: a real build writes the full register map.) */
    }
    g_battery_pct = 100;
    g_low = 0;
}

void power_poll(void) {
    uint8_t vbat_reg = 0u;
    if (i2c4_read_reg(0x0Eu, &vbat_reg) != 0) {
        return;  /* I2C read failed — keep last known state */
    }
    /* VBAT register: bits [7:2] = voltage in 20 mV steps above 2304 mV.
     * Convert to a rough 0..100 percentage (3.0 V = 0%, 4.2 V = 100%). */
    uint16_t vbat_mv = (uint16_t)(2304u + ((uint16_t)(vbat_reg >> 2) * 20u));
    if      (vbat_mv >= 4200u) g_battery_pct = 100;
    else if (vbat_mv <= 3000u) g_battery_pct = 0;
    else                       g_battery_pct = (int)(((uint32_t)(vbat_mv - 3000u) * 100u) / 1200u);
    g_low = (g_battery_pct < 20);
}

int power_is_low(void)         { return g_low; }
int power_battery_pct(void)    { return g_battery_pct; }