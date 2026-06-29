/*
 * qi_rx.c — Qi wireless-power receiver driver (upstream side of CoilGrip)
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * CoilGrip presents a Qi 1.1 receiver to the *upstream* charger so it can
 * draw power and — critically — so it can intercept the PR→PT load-
 * modulation packets the target sends. The BQ51013B handles rectification
 * and output regulation; its I²C telemetry gives us VRECT, IOUT, and temp.
 * The load-modulation line is routed to the FPGA for packet capture.
 *
 * This driver reads telemetry over I²C1 and drives the load-modulation
 * switch (ADG849 on PA0) for CoilGrip's own packet injection toward the
 * upstream charger (used in MITM and EXFIL modes).
 */

#include "board.h"
#include "registers.h"

/* BQ51013B I²C slave address (7-bit 0x6B, 8-bit write 0xD6 / read 0xD7) */
#define BQ_I2C_ADDR_W        0xD6U
#define BQ_I2C_ADDR_R        0xD7U

/* BQ51013B register map (subset) */
#define BQ_REG_VRECT          0x02  /* rectified voltage (mV, 2 bytes, BE)    */
#define BQ_REG_IOUT            0x04  /* output current (mA, 2 bytes)            */
#define BQ_REG_TEMP            0x06  /* temperature (°C, signed)                 */
#define BQ_REG_CTRL_STATE      0x0A  /* control-loop state                       */
#define BQ_REG_FOD             0x0C  /* FOD calibration                          */
#define BQ_REG_CTRL            0x0E  /* control byte (enable, ILIM)              */

/* Load-modulation packet definitions (Qi 1.1 back-channel).
 * Header format: [1 bit start=1][3 bits type][4 bits length-1]
 * Data: (length) bytes
 * CRC-8: poly 0x07. */

#define LM_START_MASK        0x80U
#define LM_TYPE_MASK         0x70U
#define LM_LEN_MASK          0x0FU

enum {
    LM_PKT_SIGNAL_STRENGTH = 0,
    LM_PKT_CTRL_ERROR       = 1,
    LM_PKT_RECEIVED_POWER   = 2,
    LM_PKT_END_POWER        = 3,
    LM_PKT_CONFIG            = 4,
    LM_PKT_IDENTIFICATION    = 5,
    LM_PKT_EXT_IDENT         = 6,
    LM_PKT_PROPRIETARY       = 7
};

static bool g_load_mod_on = false;

/* ---- I²C1 helpers ------------------------------------------------------ */

static int i2c1_wait_flag(uint32_t mask, uint32_t timeout_loops)
{
    for (uint32_t i = 0; i < timeout_loops; i++) {
        if (I2C1->ISR & mask) return 0;
    }
    return -1;
}

static int i2c1_write_reg(uint8_t reg, uint8_t val)
{
    I2C1->CR2 = ((uint32_t)BQ_I2C_ADDR_W)
             | I2C_CR2_NBYTES(2)
             | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;
    if (i2c1_wait_flag(I2C_ISR_TXE, 100000)) return -1;
    I2C1->TXDR = reg;
    if (i2c1_wait_flag(I2C_ISR_TXE, 100000)) return -1;
    I2C1->TXDR = val;
    /* Wait for STOPF (auto-end) */
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ICR_STOPCF;
    return 0;
}

static int i2c1_read_reg_u16(uint8_t reg, uint16_t *out)
{
    /* Write reg addr with restart */
    I2C1->CR2 = ((uint32_t)BQ_I2C_ADDR_W)
             | I2C_CR2_NBYTES(1);
    I2C1->CR2 |= I2C_CR2_START;
    if (i2c1_wait_flag(I2C_ISR_TXE, 100000)) return -1;
    I2C1->TXDR = reg;
    /* Wait transfer complete then issue repeated start for read */
    while (!(I2C1->ISR & BIT(6))) { }  /* TC */
    I2C1->CR2 = ((uint32_t)BQ_I2C_ADDR_R)
             | I2C_CR2_NBYTES(2)
             | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;
    /* Read 2 bytes */
    if (i2c1_wait_flag(I2C_ISR_RXNE, 100000)) return -1;
    uint8_t hi = (uint8_t)I2C1->RXDR;
    if (i2c1_wait_flag(I2C_ISR_RXNE, 100000)) return -1;
    uint8_t lo = (uint8_t)I2C1->RXDR;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ICR_STOPCF;
    *out = ((uint16_t)hi << 8) | lo;
    return 0;
}

static int i2c1_read_reg_s8(uint8_t reg, int8_t *out)
{
    I2C1->CR2 = ((uint32_t)BQ_I2C_ADDR_W) | I2C_CR2_NBYTES(1);
    I2C1->CR2 |= I2C_CR2_START;
    if (i2c1_wait_flag(I2C_ISR_TXE, 100000)) return -1;
    I2C1->TXDR = reg;
    while (!(I2C1->ISR & BIT(6))) { }
    I2C1->CR2 = ((uint32_t)BQ_I2C_ADDR_R) | I2C_CR2_NBYTES(1) | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;
    if (i2c1_wait_flag(I2C_ISR_RXNE, 100000)) return -1;
    *out = (int8_t)I2C1->RXDR;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ICR_STOPCF;
    return 0;
}

/* ---- Public API -------------------------------------------------------- */

int qi_rx_init(void)
{
    /* Enable I2C1 peripheral */
    I2C1->CR1 = 0;
    /* Timing for 400 kHz fast mode from 120 MHz PCLK */
    I2C1->TIMINGR = (1U << 28)   /* PRESC = 1 */
                  | (0x9U << 20) /* SCLL */
                  | (0x3U << 16) /* SCLH */
                  | (0x1U << 8)  /* SDADEL */
                  | (0x3U << 0); /* SCLDEL */
    I2C1->CR1 = I2C_CR1_PE;
    g_load_mod_on = false;
    GPIOA->BSRR = BIT(PIN_PA0 + 16);  /* load mod off */
    return 0;
}

int qi_rx_read_telemetry(uint16_t *vrect_mv, uint16_t *iout_ma,
                         int8_t *temp_c, uint8_t *ctrl_state)
{
    uint16_t v = 0, i = 0;
    int8_t t = 0;
    uint8_t st = 0;
    int rc = 0;
    if (vrect_mv && i2c1_read_reg_u16(BQ_REG_VRECT, &v) == 0) {
        *vrect_mv = v;
    } else rc = -1;
    if (iout_ma && i2c1_read_reg_u16(BQ_REG_IOUT, &i) == 0) {
        *iout_ma = i;
    } else rc = -1;
    if (temp_c && i2c1_read_reg_s8(BQ_REG_TEMP, &t) == 0) {
        *temp_c = t;
    } else rc = -1;
    /* ctrl_state read as single byte via the s8 path */
    int8_t cs;
    if (ctrl_state && i2c1_read_reg_s8(BQ_REG_CTRL_STATE, &cs) == 0) {
        *ctrl_state = (uint8_t)cs;
    } else rc = -1;
    return rc;
}

/* Build a Qi load-modulation packet header byte */
static uint8_t lm_header(uint8_t type, uint8_t data_len)
{
    return (uint8_t)(LM_START_MASK | ((type & 0x7U) << 4) | ((data_len - 1) & 0xFU));
}

/* CRC-8 (same polynomial as TX side) */
static uint8_t lm_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
    }
    return crc;
}

int qi_rx_send_packet(const uint8_t *pkt, uint8_t len)
{
    /* Frame: [header][data][crc]. We toggle PA0 (ADG849 select) to
     * encode bits at 2 kbaud. The FPGA could also be used for precise
     * timing; here we do a software bit-bang at the Qi back-channel rate. */
    if (len == 0 || len > 15) return -1;

    uint8_t frame[17];
    uint8_t flen = 0;
    frame[flen++] = lm_header(LM_PKT_PROPRIETARY, len);
    for (uint8_t i = 0; i < len; i++) frame[flen++] = pkt[i];
    frame[flen++] = lm_crc8(frame, flen - 1);  /* CRC over header+data */

    /* Bit-bang at 2 kbaud (500 µs/bit). MSB-first.
     * Pre-condition: hold load-mod on for 8 bit-times of preamble (0xFF). */
    uint32_t bit_delay = (HCLK_HZ / 8000U);  /* rough loop count for 500 µs */

    GPIOA->BSRR = BIT(PIN_PA0);  /* load mod on */
    for (volatile uint32_t d = 0; d < bit_delay; d++) { }
    /* Preamble */
    for (int b = 0; b < 8; b++) {
        GPIOA->BSRR = BIT(PIN_PA0);  /* '1' */
        for (volatile uint32_t d = 0; d < bit_delay; d++) { }
        GPIOA->BSRR = BIT(PIN_PA0 + 16);  /* '0' */
        for (volatile uint32_t d = 0; d < bit_delay; d++) { }
    }
    /* Frame */
    for (uint8_t i = 0; i < flen; i++) {
        for (int b = 7; b >= 0; b--) {
            if (frame[i] & BIT(b))
                GPIOA->BSRR = BIT(PIN_PA0);     /* '1' */
            else
                GPIOA->BSRR = BIT(PIN_PA0 + 16);/* '0' */
            for (volatile uint32_t d = 0; d < bit_delay; d++) { }
        }
    }
    GPIOA->BSRR = BIT(PIN_PA0 + 16);  /* off */
    return 0;
}

void qi_rx_set_load_modulation(bool on)
{
    g_load_mod_on = on;
    if (on)
        GPIOA->BSRR = BIT(PIN_PA0);
    else
        GPIOA->BSRR = BIT(PIN_PA0 + 16);
}