/*
 * fuel_gauge.c — MAX17048 I²C LiPo fuel gauge driver
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include "fuel_gauge.h"
#include "../board.h"
#include "../registers.h"

#define I2C1 ((i2c_t *)I2C1_BASE)

static int g_init = 0;

static int i2c_write(uint8_t addr, uint8_t reg, uint16_t val) {
    I2C1->CR2 = (addr << 1) | (2U << 16) | I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
        if (I2C1->ISR & 0x800) { I2C1->ICR = 0x800; return -1; }
    }
    I2C1->TXDR = reg;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) { }
    I2C1->TXDR = (val >> 8) & 0xFF;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) { }
    I2C1->TXDR = val & 0xFF;
    while (!(I2C1->ISR & I2C_CR2_STOP)) { }
    I2C1->CR2 |= I2C_CR2_STOP;
    return 0;
}

static int i2c_read16(uint8_t addr, uint8_t reg, uint16_t *val) {
    I2C1->CR2 = (addr << 1) | (1U << 16) | I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) { }
    I2C1->TXDR = reg;
    I2C1->CR2 |= I2C_CR2_STOP;
    /* restart for read */
    I2C1->CR2 = (addr << 1) | (2U << 16) | I2C_CR2_START | I2C_CR2_RD_WRN;
    uint8_t hi, lo;
    while (!(I2C1->ISR & I2C_ISR_RXNE)) { }
    hi = I2C1->RXDR;
    while (!(I2C1->ISR & I2C_ISR_RXNE)) { }
    lo = I2C1->RXDR;
    I2C1->CR2 |= I2C_CR2_STOP;
    *val = (hi << 8) | lo;
    return 0;
}

int fuel_gauge_init(void) {
    /* Configure I2C1 GPIO PB8/PB9 AF4 already done elsewhere */
    I2C1->CR1 = 0;
    /* 100 kHz: TIMINGR for 120 MHz APB1 -> 0x10909CEC (typical) */
    I2C1->TIMINGR = 0x10909CEC;
    I2C1->CR1 = I2C_CR1_PE;
    /* Quick power-on reset of MAX17048 */
    i2c_write(FUEL_GAUGE_ADDR, 0x16, 0x0001); /* cmd: reset */
    uint32_t t0 = 0;
    while ((0 - t0) < 20) { } /* wait 20ms */
    g_init = 1;
    return 0;
}

uint16_t fuel_gauge_voltage_mv(void) {
    uint16_t vcell;
    if (i2c_read16(FUEL_GAUGE_ADDR, 0x09, &vcell) != 0) return 0;
    /* VCELL = (reg >> 7) * 5 mV per LSB (per datasheet shift) */
    return (vcell >> 3) * 5;
}

uint8_t fuel_gauge_percent(void) {
    uint16_t soc;
    if (i2c_read16(FUEL_GAUGE_ADDR, 0x06, &soc) != 0) return 0;
    /* SOC = reg >> 8 (%) ; low byte fractional */
    return (uint8_t)(soc >> 8);
}

uint16_t fuel_gauge_soc_raw(void) {
    uint16_t soc;
    i2c_read16(FUEL_GAUGE_ADDR, 0x06, &soc);
    return soc;
}