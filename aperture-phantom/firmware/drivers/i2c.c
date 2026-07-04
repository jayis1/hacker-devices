/*
 * aperture-phantom / firmware / drivers / i2c.c
 *
 * Two-channel I²C CCI master for the sensor control bus. Channel A drives
 * the sensor side of the link; channel B can mirror/spoof the host side or
 * talk to the INA219 power monitor (addr 0x40) on side-B.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include "board.h"

static uint32_t side_base(cci_side_t s) {
    return (s == CCI_SIDE_A) ? I2C1_BASE : I2C2_BASE;
}

int cci_init(cci_side_t side) {
    uint32_t b = side_base(side);
    I2C_CR1(b) &= ~(1u << 0); /* disable */
    /* TIMING for 400 kHz @ 120 MHz PCLK: PRESC=1 SCLL SCLH SDADEL SCLDEL
     * approximate; tune for the real board. */
    I2C_TIMINGR(b) = (1u << 28)        /* PRESC=1 */
                   | (0x13u << 16)     /* SCLL */
                   | (0x0Fu << 8)      /* SCLH */
                   | (0x02u << 0);     /* SCLDEL/SDADEL */
    I2C_OAR1(b) = (1u << 14);          /* OA1EN, own addr 0 (we are master) */
    I2C_CR1(b) |= (1u << 0);           /* enable */
    return 0;
}

int cci_scan(cci_side_t side, uint8_t *found, int max) {
    uint32_t b = side_base(side);
    int n = 0;
    for (uint8_t addr = 1; addr < 128 && n < max; addr++) {
        I2C_CR2(b) = ((uint32_t)addr << 1)
                   | (1u << 16)        /* NBYTES=1 */
                   | (1u << 25)         /* AUTOEND */
                   | (1u << 10)         /* WRITE */
                   | (1u << 13);        /* START */
        /* Wait for TXE to write a dummy byte, then check NACK. */
        uint32_t to = 0;
        while (((I2C_ISR(b) & I2C_ISR_TXE) == 0) && to < 1000) to++;
        I2C_TXDR(b) = 0x00;
        to = 0;
        while (((I2C_ISR(b) & (I2C_ISR_TC | (1u << 2))) == 0) && to < 5000) to++;
        if ((I2C_ISR(b) & (1u << 2)) == 0) { /* no NACK → device present */
            found[n++] = addr;
        }
        I2C_CR1(b) |= (1u << 3); /* clear NACK */
        I2C_CR1(b) &= ~(1u << 3);
    }
    return n;
}

int cci_write(cci_side_t side, uint8_t addr, uint16_t reg, uint16_t val) {
    uint32_t b = side_base(side);
    I2C_CR2(b) = ((uint32_t)addr << 1)
               | (3u << 16)         /* NBYTES=3 (reg hi, reg lo, val hi, val lo) */
               | (1u << 25)          /* AUTOEND */
               | (0u << 10)          /* WRITE */
               | (1u << 13);         /* START */
    while ((I2C_ISR(b) & I2C_ISR_TXE) == 0) { }
    I2C_TXDR(b) = (uint8_t)(reg >> 8);
    while ((I2C_ISR(b) & I2C_ISR_TXE) == 0) { }
    I2C_TXDR(b) = (uint8_t)(reg & 0xFF);
    while ((I2C_ISR(b) & I2C_ISR_TXE) == 0) { }
    I2C_TXDR(b) = (uint8_t)(val >> 8);
    while ((I2C_ISR(b) & I2C_ISR_TXE) == 0) { }
    I2C_TXDR(b) = (uint8_t)(val & 0xFF);
    while ((I2C_ISR(b) & I2C_ISR_TC) == 0 &&
           (I2C_ISR(b) & (1u << 4)) == 0) { } /* STOPF */
    if (I2C_ISR(b) & (1u << 2)) { /* NACK */
        I2C_CR1(b) |= (1u << 3);
        return -1;
    }
    return 0;
}

int cci_read(cci_side_t side, uint8_t addr, uint16_t reg, uint16_t *val) {
    uint32_t b = side_base(side);
    /* Write 2-byte reg addr with RELOAD, then restart for read. */
    I2C_CR2(b) = ((uint32_t)addr << 1)
               | (2u << 16)
               | (1u << 24)          /* RELOAD */
               | (0u << 10)
               | (1u << 13);
    while ((I2C_ISR(b) & I2C_ISR_TXE) == 0) { }
    I2C_TXDR(b) = (uint8_t)(reg >> 8);
    while ((I2C_ISR(b) & I2C_ISR_TXE) == 0) { }
    I2C_TXDR(b) = (uint8_t)(reg & 0xFF);
    while ((I2C_ISR(b) & (1u << 24)) == 0) { } /* TCR */
    I2C_CR2(b) = ((uint32_t)addr << 1)
               | (2u << 16)
               | (1u << 25)          /* AUTOEND */
               | (1u << 10)          /* READ */
               | (1u << 13);         /* START */
    while ((I2C_ISR(b) & I2C_ISR_RXNE) == 0) { }
    uint8_t hi = (uint8_t)I2C_RXDR(b);
    while ((I2C_ISR(b) & I2C_ISR_RXNE) == 0) { }
    uint8_t lo = (uint8_t)I2C_RXDR(b);
    *val = (uint16_t)((hi << 8) | lo);
    return 0;
}

int cci_read_block(cci_side_t side, uint8_t addr, uint16_t reg,
                   uint8_t *buf, uint16_t len) {
    uint32_t b = side_base(side);
    if (len > 255) len = 255;
    I2C_CR2(b) = ((uint32_t)addr << 1)
               | (2u << 16) | (1u << 24) | (0u << 10) | (1u << 13);
    while ((I2C_ISR(b) & I2C_ISR_TXE) == 0) { }
    I2C_TXDR(b) = (uint8_t)(reg >> 8);
    while ((I2C_ISR(b) & I2C_ISR_TXE) == 0) { }
    I2C_TXDR(b) = (uint8_t)(reg & 0xFF);
    while ((I2C_ISR(b) & (1u << 24)) == 0) { }
    I2C_CR2(b) = ((uint32_t)addr << 1)
               | ((uint32_t)len << 16) | (1u << 25) | (1u << 10) | (1u << 13);
    for (uint16_t i = 0; i < len; i++) {
        while ((I2C_ISR(b) & I2C_ISR_RXNE) == 0) { }
        buf[i] = (uint8_t)I2C_RXDR(b);
    }
    return 0;
}