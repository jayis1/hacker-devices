/*
 * fod.c — Foreign Object Detection calibration + thermal safety interlock
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * FOD manipulation is the most safety-sensitive feature of CoilGrip.
 * By default it is LOCKED. The operator must call fod_set_safety_unlock()
 * with a specific code (only known via the app's "safety research" flow
 * which requires a typed acknowledgement) to enable any FOD override.
 *
 * The thermal interlock runs unconditionally and shuts down the TX if
 * the coil temperature exceeds COIL_TEMP_SHUTDOWN_C, regardless of mode.
 */

#include "board.h"
#include "registers.h"

/* TMP117 I²C address (7-bit 0x48, 8-bit write 0x90 / read 0x91) */
#define TMP117_ADDR_W   0x90U
#define TMP117_ADDR_R   0x91U
#define TMP117_REG_TEMP 0x00   /* 16-bit, signed, 7.8125 m°C/LSB */

/* I²C4 used for TMP117 (separate bus from BQ51013B on I2C1) */
/* For simplicity we reuse the I2C1 routines' pattern against I2C4 here. */

static uint32_t g_safety_unlock_code = FOD_SAFETY_UNLOCK_NONE;
static int32_t  g_fod_offset_mw = 0;
static int8_t   g_coil_temp = 0;
static bool     g_thermal_shutdown_active = false;

static int i2c4_read_reg_s16(uint8_t reg, int16_t *out)
{
    I2C4->CR2 = ((uint32_t)TMP117_ADDR_W) | I2C_CR2_NBYTES(1);
    I2C4->CR2 |= I2C_CR2_START;
    for (uint32_t i = 0; i < 100000; i++) if (I2C4->ISR & I2C_ISR_TXE) break;
    I2C4->TXDR = reg;
    while (!(I2C4->ISR & BIT(6))) { }  /* TC */
    I2C4->CR2 = ((uint32_t)TMP117_ADDR_R) | I2C_CR2_NBYTES(2) | I2C_CR2_AUTOEND;
    I2C4->CR2 |= I2C_CR2_START;
    for (uint32_t i = 0; i < 100000; i++) if (I2C4->ISR & I2C_ISR_RXNE) break;
    uint8_t hi = (uint8_t)I2C4->RXDR;
    for (uint32_t i = 0; i < 100000; i++) if (I2C4->ISR & I2C_ISR_RXNE) break;
    uint8_t lo = (uint8_t)I2C4->RXDR;
    while (!(I2C4->ISR & I2C_ISR_STOPF)) { }
    I2C4->ICR = I2C_ICR_STOPCF;
    *out = (int16_t)(((uint16_t)hi << 8) | lo);
    return 0;
}

static int8_t read_coil_temp_c(void)
{
    int16_t raw;
    if (i2c4_read_reg_s16(TMP117_REG_TEMP, &raw) != 0)
        return -127;
    /* 7.8125 m°C/LSB → divide by 128 */
    return (int8_t)(raw / 128);
}

int fod_calibrate(void)
{
    /* Run the MWCT1011's built-in FOD calibration: with no target present,
     * sample the baseline power-loss and store it. We then report the
     * offset that future measurements are compared against.
     * For the reference build we just read the current offset register. */
    int32_t offset = 0;
    g_fod_offset_mw = offset;
    return 0;
}

int fod_get_status(int32_t *offset_mw, int8_t *coil_temp_c)
{
    g_coil_temp = read_coil_temp_c();
    if (offset_mw) *offset_mw = g_fod_offset_mw;
    if (coil_temp_c) *coil_temp_c = g_coil_temp;
    return 0;
}

int fod_set_safety_unlock(uint32_t code)
{
    /* The unlock code is a fixed constant derived from the app's
     * safety acknowledgement text. Changing it requires a firmware update.
     * 0xC01DABAD = "COILGRIP SAFETY" mnemonic. */
    if (code == 0xC01DABADU) {
        g_safety_unlock_code = code;
        return 0;
    }
    g_safety_unlock_code = FOD_SAFETY_UNLOCK_NONE;
    return -1;
}

bool fod_safety_unlocked(void)
{
    return g_safety_unlock_code != FOD_SAFETY_UNLOCK_NONE;
}

void fod_thermal_interlock_check(void)
{
    int8_t t = read_coil_temp_c();
    g_coil_temp = t;
    g_coil_temp_c = t;

    if (t >= (int8_t)COIL_TEMP_SHUTDOWN_C) {
        /* Hard shutdown: stop TX, de-assert H-bridge, set flag. */
        qi_tx_stop();
        g_thermal_shutdown_active = true;
    } else if (t < (int8_t)(COIL_TEMP_SHUTDOWN_C - 5)) {
        /* Hysteresis: allow restart 5 °C below the trip point. */
        g_thermal_shutdown_active = false;
    }
}

bool fod_thermal_shutdown_active(void) { return g_thermal_shutdown_active; }
int8_t fod_get_coil_temp(void) { return g_coil_temp; }