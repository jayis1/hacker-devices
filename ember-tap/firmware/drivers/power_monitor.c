/*
 * power_monitor.c — TPS25982 eFuse driver, I2C1 low-level, VBUS glitch
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "power_monitor.h"
#include "board.h"
#include "registers.h"

/* ---- I2C1 low-level ----
 * I2C1 is on PB6 (SCL) / PB7 (SDA), AF4.
 * Clock: APB1 = 170 MHz. We program TIMINGR for 400 kHz Fast Mode.
 * The STM32G4 I2C timing value for 400 kHz @ 170 MHz I2CCLK from the
 * reference manual table: 0x10B17DB5 (approx). We use a conservative
 * 0x30707DB1 for 100 kHz to be robust across bus capacitance.
 */
#define I2C_TIMING_100K   0x30707DB1U
#define I2C_TIMING_400K   0x10B17DB5U

void power_monitor_init(void) {
    /* Enable I2C1 clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
    for (volatile int i = 0; i < 100; i++);

    /* Configure PB6/PB7 as AF4 (I2C1), open-drain, pull-up */
    GPIOB->MODER &= ~(0x3U << (I2C1_SCL_PIN * 2));
    GPIOB->MODER |=  (GPIO_MODE_AF << (I2C1_SCL_PIN * 2));
    GPIOB->AFRL  &= ~(0xFU << (I2C1_SCL_PIN * 4));
    GPIOB->AFRL  |=  (0x4U << (I2C1_SCL_PIN * 4));

    GPIOB->MODER &= ~(0x3U << (I2C1_SDA_PIN * 2));
    GPIOB->MODER |=  (GPIO_MODE_AF << (I2C1_SDA_PIN * 2));
    GPIOB->AFRL  &= ~(0xFU << (I2C1_SDA_PIN * 4));
    GPIOB->AFRL  |=  (0x4U << (I2C1_SDA_PIN * 4));

    GPIOB->OTYPER |= (1U << I2C1_SCL_PIN) | (1U << I2C1_SDA_PIN); /* open-drain */
    GPIOB->PUPDR  |= (1U << (I2C1_SCL_PIN * 2)) | (1U << (I2C1_SDA_PIN * 2));

    /* Disable I2C before config */
    I2C1->CR1 = 0;
    I2C1->TIMINGR = I2C_TIMING_400K;
    I2C1->CR1 = I2C_CR1_PE | I2C_CR1_NACKIE | I2C_CR1_STOPIE;

    /* Configure TPS25982 enable pin (PB14) as output, high (enable) */
    GPIOB->MODER &= ~(0x3U << (TPS_EN_PIN * 2));
    GPIOB->MODER |=  (GPIO_MODE_OUTPUT << (TPS_EN_PIN * 2));
    GPIOB->OTYPER &= ~(1U << TPS_EN_PIN);
    GPIOB->PUPDR  &= ~(0x3U << (TPS_EN_PIN * 2));
    GPIOB->BSRR = (1U << TPS_EN_PIN);   /* enable eFuse */

    /* Configure TPS_FLTB (PB13) as input, pull-up */
    GPIOB->MODER &= ~(0x3U << (TPS_FLTB_PIN * 2));
    GPIOB->PUPDR |= (0x1U << (TPS_FLTB_PIN * 2));

    /* Configure GLITCH_GATE (PB12) as output, low (off) */
    GPIOB->MODER &= ~(0x3U << (GLITCH_GATE_PIN * 2));
    GPIOB->MODER |=  (GPIO_MODE_OUTPUT << (GLITCH_GATE_PIN * 2));
    GPIOB->OTYPER &= ~(1U << GLITCH_GATE_PIN);
    GPIOB->BSRR = (1U << (GLITCH_GATE_PIN + 16)); /* reset (low) */

    /* Set safe defaults on TPS25982 */
    power_set_ovp(TPS_OVP_20V);
    power_set_ilim(5000);  /* 5 A */
}

/* ---- I2C1 write ----
 * Sends `len` bytes from `data` to 7-bit address `addr`.
 * First byte is assumed to be the register pointer (for the TPS25982).
 */
int i2c1_write(uint8_t addr, const uint8_t *data, uint8_t len) {
    if (len == 0) return 0;

    /* Wait if busy */
    uint32_t to = 0xFFFFF;
    while ((I2C1->ISR & I2C_ISR_BUSY) && to--) ;

    /* Program CR2: 7-bit address, write, NBYTES=len, AUTOEND */
    I2C1->CR2 = ((addr & 0x7F) << 1)
              | I2C_CR2_NBYTES(len)
              | I2C_CR2_AUTOEND
              | I2C_CR2_START;

    for (uint8_t i = 0; i < len; i++) {
        to = 0xFFFFF;
        while (!(I2C1->ISR & I2C_ISR_TXIS) && to--) {
            if (I2C1->ISR & I2C_ISR_NACKF) {
                I2C1->ICR = I2C_ICR_NACKCF;
                return -1;
            }
        }
        if (!to) return -1;
        I2C1->TXDR = data[i];
    }

    /* Wait for STOP */
    to = 0xFFFFF;
    while (!(I2C1->ISR & I2C_ISR_STOPF) && to--) ;
    I2C1->ICR = I2C_ICR_STOPCF;
    return 0;
}

/* ---- I2C1 read (with register pointer write first) ---- */
int i2c1_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len) {
    if (len == 0) return 0;

    /* Write the register pointer */
    uint8_t regbuf[1] = { reg };
    uint32_t to;

    /* Wait if busy */
    to = 0xFFFFF;
    while ((I2C1->ISR & I2C_ISR_BUSY) && to--) ;

    /* Write 1 byte (reg), no stop (TCR reload) */
    I2C1->CR2 = ((addr & 0x7F) << 1)
              | I2C_CR2_NBYTES(1)
              | I2C_CR2_START;
    to = 0xFFFFF;
    while (!(I2C1->ISR & I2C_ISR_TXIS) && to--) ;
    if (!to) return -1;
    I2C1->TXDR = reg;

    /* Wait for transfer complete */
    to = 0xFFFFF;
    while (!(I2C1->ISR & I2C_ISR_TC) && to--) ;

    /* Read phase */
    I2C1->CR2 = ((addr & 0x7F) << 1)
              | I2C_CR2_RD_WRN
              | I2C_CR2_NBYTES(len)
              | I2C_CR2_AUTOEND
              | I2C_CR2_START;

    for (uint8_t i = 0; i < len; i++) {
        to = 0xFFFFF;
        while (!(I2C1->ISR & I2C_ISR_RXNE) && to--) {
            if (I2C1->ISR & I2C_ISR_NACKF) {
                I2C1->ICR = I2C_ICR_NACKCF;
                return -1;
            }
        }
        if (!to) return -1;
        buf[i] = I2C1->RXDR;
    }

    to = 0xFFFFF;
    while (!(I2C1->ISR & I2C_ISR_STOPF) && to--) ;
    I2C1->ICR = I2C_ICR_STOPCF;
    return 0;
}

/* ---- TPS25982 helpers ---- */
static void tps_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    i2c1_write(TPS25982_I2C_ADDR, buf, 2);
}

static uint8_t tps_read_reg(uint8_t reg) {
    uint8_t v = 0;
    i2c1_read(TPS25982_I2C_ADDR, reg, &v, 1);
    return v;
}

void power_set_ovp(uint16_t tenths_v) {
    /* The TPS25982 OVP register codes threshold in 0.1 V steps.
     * We clamp to the supported range [4.0 V .. 23.0 V]. */
    if (tenths_v < 40)  tenths_v = 40;
    if (tenths_v > 230) tenths_v = 230;
    tps_write_reg(TPS_REG_OVP, (uint8_t)tenths_v);
}

void power_set_ilim(uint16_t ma) {
    /* Current limit register: 0 = 0.5 A, 255 = 5.0 A (approx linear).
     * Code = (ma - 500) / 17.6 */
    if (ma < 500)  ma = 500;
    if (ma > 5000) ma = 5000;
    uint8_t code = (uint8_t)((ma - 500) / 18);
    tps_write_reg(TPS_REG_ILIM, code);
}

void power_enable(int on) {
    if (on) GPIOB->BSRR = (1U << TPS_EN_PIN);
    else    GPIOB->BSRR = (1U << (TPS_EN_PIN + 16));
}

uint32_t power_get_vbus_mv(void) {
    /* Read TPS25982 VIN register (2 bytes, signed, 48 mV/LSB). */
    uint8_t buf[2] = {0, 0};
    i2c1_read(TPS25982_I2C_ADDR, TPS_REG_VIN, buf, 2);
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    /* Shift right 4 (per datasheet format) then scale */
    raw >>= 4;
    if (raw < 0) raw = 0;
    return (uint32_t)raw * 48;  /* mV */
}

uint32_t power_get_vbus_ma(void) {
    /* TPS25982 IIN register: 2 bytes, 8 mA/LSB (approx). */
    uint8_t buf[2] = {0, 0};
    i2c1_read(TPS25982_I2C_ADDR, TPS_REG_IIN, buf, 2);
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    raw >>= 4;
    if (raw < 0) raw = 0;
    return (uint32_t)raw * 8;
}

int8_t power_get_temp_c(void) {
    /* Read PB15 ADC (thermistor divider).
     * In a real build we'd configure ADC1. Here we do a crude Vref-int
     * reading. For brevity we approximate using the TPS25982 status
     * temperature bits. */
    uint8_t st = tps_read_reg(TPS_REG_STATUS);
    /* If the thermal-warning bit is set, return 85°C; else 35°C nominal. */
    if (st & 0x08) return 85;
    return 35;
}

int power_kill_asserted(void) {
    /* KILL_HW is PA3, active low (pulled high). Asserted = pin low. */
    return (KILL_HW_PORT->IDR & (1U << KILL_HW_PIN)) ? 0 : 1;
}

/* ---- VBUS glitch ---- */
int power_vbus_glitch(uint32_t us, int type) {
    /* Hardware interlock: refuse if KILL is asserted or VBUS > 20 V. */
    if (power_kill_asserted()) return -1;
    if (power_get_vbus_mv() > GLITCH_VBUS_MAX_MV) return -1;

    /* type=0: droop — pulse the glitch FET low-impedance briefly.
     * type=1: crowbar — longer pulse (harder on the FET, more aggressive). */
    uint32_t pulse_us = (type == 1) ? (us * 2) : us;
    if (pulse_us > 100000) pulse_us = 100000;  /* cap at 100 ms */

    /* Disable eFuse first so we don't fight the supply */
    power_enable(0);

    /* Pulse the glitch FET */
    GPIOB->BSRR = (1U << GLITCH_GATE_PIN);            /* gate high (FET on) */

    /* Busy-wait for pulse_us microseconds.
     * At 170 MHz, 1 µs ≈ 170 iterations of a tight loop (2-3 cycles each). */
    volatile uint32_t cycles = pulse_us * 50;  /* calibrated conservative */
    while (cycles--) __asm__ volatile ("nop");

    GPIOB->BSRR = (1U << (GLITCH_GATE_PIN + 16));     /* gate low (FET off) */

    /* Re-enable eFuse */
    power_enable(1);
    return 0;
}

/* end of file — author: jayis1 */