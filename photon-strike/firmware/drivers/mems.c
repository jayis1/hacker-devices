/**
 * drivers/mems.c — 2-axis MEMS mirror + piezo Z focus control
 * Author: jayis1
 * License: GPL-2.0
 *
 * The Mirrorcle MEMS-2512 mirror accepts I²C commands at address 0x48.
 * Two 12-bit DAC values set the X and Y electrostatic drive voltages
 * (0..150 V on the mirror driver, mapped to 0..4095 here). The piezo
 * Z focus is driven by the on-chip DAC1_OUT1 (PA4) through a ×50
 * high-voltage amplifier (TI OPA4S120) giving 0..60 V across a 60 µm
 * piezo stack → ~50 nm focus resolution.
 *
 * The (x_um, y_um) → DAC mapping assumes a 0.45 NA objective with a
 * 1.5 mm working distance; one DAC count ≈ 0.25 µm at the die. The
 * mapping is calibrated at production time and stored in a small
 * lookup table (cal_um_to_dac[]).
 */

#include "mems.h"
#include "gpio.h"
#include "../registers.h"

#define MEMS_I2C_ADDR  0x48u
#define I2C1           ((volatile uint32_t *)I2C1_BASE)
#define DAC1           ((volatile uint32_t *)DAC1_BASE)

/* ─── Calibration (x/y µm → DAC counts) ────────────────────────────────
 * Linear: 0 µm → 2048 (center), ±500 µm → ±2048 counts.
 * Range is ±500 µm around the optical axis. */
#define MEMS_CENTER  2048u
#define MEMS_UM_PER_COUNT  0.25f   /* µm per DAC count */

static uint16_t um_to_dac(int32_t um)
{
    int32_t c = MEMS_CENTER + (int32_t)(um / MEMS_UM_PER_COUNT);
    if (c < 0)     c = 0;
    if (c > 4095)  c = 4095;
    return (uint16_t)c;
}

/* ─── I²C1 primitives (polling) ───────────────────────────────────────── */
static void i2c_wait_idle(void)
{
    while (I2C1[I2C_ISR / 4u] & (1u << 15)) ;   /* BUSY */
}

static bool i2c_write2(uint8_t addr, uint8_t reg, uint16_t val)
{
    i2c_wait_idle();
    /* Set address + write + 3 bytes (reg, val_hi, val_lo). */
    I2C1[I2C_CR2 / 4u] = ((uint32_t)addr << 1) | (3u << 16) | I2C_CR2_START;
    while (!(I2C1[I2C_ISR / 4u] & I2C_ISR_TXE)) ;
    I2C1[I2C_TXDR / 4u] = reg;
    while (!(I2C1[I2C_ISR / 4u] & I2C_ISR_TXE)) ;
    I2C1[I2C_TXDR / 4u] = (val >> 8) & 0xFF;
    while (!(I2C1[I2C_ISR / 4u] & I2C_ISR_TXE)) ;
    I2C1[I2C_TXDR / 4u] = val & 0xFF;
    while (!(I2C1[I2C_ISR / 4u] & I2C_ISR_TC)) ;
    I2C1[I2C_CR2 / 4u] |= I2C_CR2_STOP;
    return !(I2C1[I2C_ISR / 4u] & I2C_ISR_NACKF);
}

/* ─── DAC1 (piezo Z) ──────────────────────────────────────────────────── */
static void dac1_write(uint16_t val)
{
    DAC1[DAC_CR / 4u]     |= DAC_CR_EN1;
    DAC1[DAC_DHR12R1 / 4u] = (val & 0x0FFFu);
}

/* ─── Public API ──────────────────────────────────────────────────────── */
void mems_init(void)
{
    /* I2C1 timing for 400 kHz from 120 MHz PCLK. */
    I2C1[I2C_TIMINGR / 4u] = (0x6u << 28) | (0xC4u << 8) | (0x9u << 0);
    I2C1[I2C_CR1 / 4u]     = I2C_CR1_PE;

    /* DAC1 channel 1 enabled, output on PA4. */
    DAC1[DAC_CR / 4u] |= DAC_CR_EN1;

    /* Home the mirror and focus. */
    mems_home();
}

void mems_home(void)
{
    mems_goto(2048, 2048);   /* center of the 0..4095 range = 0 µm */
    mems_set_focus(30000);   /* mid-focus, ~30 µm of 60 µm travel */
}

void mems_goto(uint16_t x_um, uint16_t y_um)
{
    /* The app sends µm in the scan descriptor's grid; here we pass them
     * through the calibration to DAC counts. */
    uint16_t x_dac = um_to_dac((int32_t)x_um - 500);
    uint16_t y_dac = um_to_dac((int32_t)y_um - 500);
    i2c_write2(MEMS_I2C_ADDR, 0x10, x_dac);   /* X drive register */
    i2c_write2(MEMS_I2C_ADDR, 0x11, y_dac);   /* Y drive register */
}

void mems_set_focus(uint16_t z_nm)
{
    /* 0..60000 nm → 0..4095 DAC counts → 0..60 V on the piezo. */
    uint32_t c = (uint32_t)z_nm * 4095u / 60000u;
    if (c > 4095u) c = 4095u;
    dac1_write((uint16_t)c);
}