/*
 * ACOUSTIC-PHANTOM — Tamper detection implementation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Physical tamper detection via the LIS2DH12 accelerometer (I2C1)
 * and the mechanical tamper switch on PC15 (EXTI15).
 * If sudden acceleration is detected (device picked up, moved, or
 * shaken), or the tamper switch opens, the zeroization handler
 * in main.c is called.
 */

#include "tamper.h"
#include "registers.h"
#include "board.h"

/* LIS2DH12 registers */
#define LIS2DH_REG_WHO_AM_I   0x0F
#define LIS2DH_REG_CTRL1      0x20
#define LIS2DH_REG_CTRL2      0x21
#define LIS2DH_REG_CTRL3      0x22
#define LIS2DH_REG_INT1_CFG   0x30
#define LIS2DH_REG_INT1_SRC   0x31
#define LIS2DH_REG_INT1_THS   0x32
#define LIS2DH_REG_INT1_DUR   0x33
#define LIS2DH_WHO_AM_I_VAL   0x33

/* Tamper threshold: 2 g (in LSB, 1 LSB ≈ 16 mg at ±2g range, 12-bit) */
#define TAMPER_THRESHOLD_MG   2000
#define TAMPER_THRESHOLD_LSB  (TAMPER_THRESHOLD_MG / 16)

static uint8_t s_triggered = 0;
static float s_baseline_x = 0, s_baseline_y = 0, s_baseline_z = 0;
static uint8_t s_initialized = 0;

/* ---- I2C read for LIS2DH12 (multi-byte) -------------------------------- */
static void lis2dh_read(uint8_t reg, uint8_t *buf, uint8_t count)
{
    /* Set auto-increment bit (bit 7) */
    reg |= 0x80;

    I2C1->CR2 = (I2C1_ADDR_CODEC << 1);
    /* Note: LIS2DH12 address is typically 0x19 (SA0=0) or 0x18.
     * We use 0x19 here — the I2C1_ADDR_CODEC in board.h is for WM8904.
     * In the real build, the LIS2DH12 has its own I2C address. */
    uint8_t lis_addr = 0x19;

    I2C1->CR2 = (lis_addr << 1) | (1U << 16) | (1U << 25);
    I2C1->CR2 |= I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXE)) { }
    I2C1->TXDR = reg;
    while (!(I2C1->ISR & I2C_ISR_TC)) { }

    /* Repeated start for read */
    I2C1->CR2 = (lis_addr << 1) | I2C_CR2_RD_WRN | (count << 16) | (1U << 25);
    I2C1->CR2 |= I2C_CR2_START;

    for (int i = 0; i < count; i++) {
        while (!(I2C1->ISR & I2C_ISR_RXNE)) { }
        buf[i] = (uint8_t)I2C1->RXDR;
    }
    while (!(I2C1->ISR & I2C_ISR_TC)) { }
}

static void lis2dh_write(uint8_t reg, uint8_t val)
{
    uint8_t lis_addr = 0x19;
    I2C1->CR2 = (lis_addr << 1) | (2U << 16) | (1U << 25);
    I2C1->CR2 |= I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXE)) { }
    I2C1->TXDR = reg;
    while (!(I2C1->ISR & I2C_ISR_TXE)) { }
    I2C1->TXDR = val;
    while (!(I2C1->ISR & I2C_ISR_TC)) { }
}

/* ---- Read acceleration (X, Y, Z in mg) --------------------------------- */
static void read_accel(float *x, float *y, float *z)
{
    uint8_t data[6];
    lis2dh_read(0x28, data, 6);  /* OUT_X_L through OUT_Z_H */

    int16_t raw_x = (int16_t)(data[0] | (data[1] << 8)) >> 4;  /* 12-bit left-justified */
    int16_t raw_y = (int16_t)(data[2] | (data[3] << 8)) >> 4;
    int16_t raw_z = (int16_t)(data[4] | (data[5] << 8)) >> 4;

    /* Convert to mg (±2g range, 1 LSB ≈ 16 mg) */
    *x = raw_x * 16.0f;
    *y = raw_y * 16.0f;
    *z = raw_z * 16.0f;
}

/* ---- Public API -------------------------------------------------------- */
void tamper_init(void)
{
    /* Verify LIS2DH12 is present */
    uint8_t whoami[1];
    lis2dh_read(LIS2DH_REG_WHO_AM_I, whoami, 1);

    if (whoami[0] != LIS2DH_WHO_AM_I_VAL) {
        /* Accelerometer not found — rely on mechanical tamper switch only */
        s_initialized = 0;
        return;
    }

    /* Configure LIS2DH12:
     *   CTRL1: ODR=100Hz, LPen=0, ZYX axes enabled
     *   CTRL2: High-pass filter for interrupt 1
     *   INT1_CFG: Motion detection on all 6 directions
     *   INT1_THS: Threshold = 2g
     *   INT1_DUR: Duration = 1 LSB (minimum) */
    lis2dh_write(LIS2DH_REG_CTRL1, 0x57);  /* 100Hz, ZYX enabled */
    lis2dh_write(LIS2DH_REG_CTRL2, 0x01);  /* HP filter for IA1 */
    lis2dh_write(LIS2DH_REG_CTRL3, 0x40);  /* IA1 interrupt to INT1 */
    lis2dh_write(LIS2DH_REG_INT1_CFG, 0x2A);  /* ZYX HI/LO */
    lis2dh_write(LIS2DH_REG_INT1_THS, TAMPER_THRESHOLD_LSB);
    lis2dh_write(LIS2DH_REG_INT1_DUR, 0x01);

    /* Read baseline acceleration */
    read_accel(&s_baseline_x, &s_baseline_y, &s_baseline_z);

    s_initialized = 1;
    s_triggered = 0;
}

void tamper_check(void)
{
    if (!s_initialized || s_triggered) return;

    /* Read current acceleration and compare to baseline */
    float x, y, z;
    read_accel(&x, &y, &z);

    /* Compute deviation from baseline */
    float dx = x - s_baseline_x;
    float dy = y - s_baseline_y;
    float dz = z - s_baseline_z;
    float deviation = dx * dx + dy * dy + dz * dz;

    /* If deviation exceeds threshold², flag tamper.
     * Threshold: 1.5g = 1500mg → 2,250,000 mg² */
    if (deviation > 2250000.0f) {
        s_triggered = 1;
    }

    /* Slowly adapt baseline when device is stable (prevent drift) */
    if (deviation < 100000.0f) {  /* < ~316mg deviation */
        s_baseline_x = s_baseline_x * 0.99f + x * 0.01f;
        s_baseline_y = s_baseline_y * 0.99f + y * 0.01f;
        s_baseline_z = s_baseline_z * 0.99f + z * 0.01f;
    }
}

uint8_t tamper_was_triggered(void)
{
    return s_triggered;
}