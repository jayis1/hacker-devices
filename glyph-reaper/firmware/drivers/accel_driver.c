/**
 * @file accel_driver.c
 * @brief LIS2DH12 accelerometer driver implementation
 *
 * I2C communication with the LIS2DH12 3-axis accelerometer for
 * motion-triggered display capture.
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "accel_driver.h"
#include "board.h"
#include "registers.h"

/*===========================================================================
 * LIS2DH12 REGISTER DEFINITIONS
 *===========================================================================*/

#define LIS2DH_REG_WHO_AM_I     0x0F
#define LIS2DH_REG_CTRL1        0x20  /* ODR, axis enable */
#define LIS2DH_REG_CTRL2        0x21  /* HPF */
#define LIS2DH_REG_CTRL3        0x22  /* Interrupt routing */
#define LIS2DH_REG_CTRL4        0x23  /* Scale, resolution */
#define LIS2DH_REG_CTRL5        0x24  /* Reboot, FIFO */
#define LIS2DH_REG_CTRL6        0x25  /* Interrupt pin config */
#define LIS2DH_REG_INT1_CFG     0x30  /* Interrupt 1 configuration */
#define LIS2DH_REG_INT1_SRC     0x31  /* Interrupt 1 source */
#define LIS2DH_REG_INT1_THS     0x32  /* Interrupt 1 threshold */
#define LIS2DH_REG_INT1_DUR     0x33  /* Interrupt 1 duration */
#define LIS2DH_REG_OUT_X_L      0x28  /* X-axis low byte */
#define LIS2DH_REG_OUT_X_H      0x29  /* X-axis high byte */
#define LIS2DH_REG_OUT_Y_L      0x2A
#define LIS2DH_REG_OUT_Y_H      0x2B
#define LIS2DH_REG_OUT_Z_L      0x2C
#define LIS2DH_REG_OUT_Z_H      0x2D

#define LIS2DH_WHO_AM_I_VAL     0x33

/* CTRL1: ODR = 100Hz (0x50), XYZ axes enabled (0x07) */
#define LIS2DH_CTRL1_VAL        0x57  /* 100Hz, XYZ enabled */

/* CTRL4: Scale ±2g (0x00), High resolution (0x08) */
#define LIS2DH_CTRL4_VAL        0x08

/* CTRL3: INT1 on activity (0x20) */
#define LIS2DH_CTRL3_VAL        0x20

/* CTRL5: Latch interrupt (0x08) */
#define LIS2DH_CTRL5_VAL        0x08

/* CTRL6: INT2 on click (0x80) */
#define LIS2DH_CTRL6_VAL        0x00

/*===========================================================================
 * I2C HELPER FUNCTIONS
 *===========================================================================*/

static void i2c_write(uint8_t reg, uint8_t value)
{
    /* Configure TWIM0 for I2C write */
    /* In real implementation, use nRF52840 TWIM peripheral:
     * - Set address to ACCEL_I2C_ADDR
     * - Write register address + value
     */

    /* Simplified: direct register access */
    volatile uint32_t *twim_address = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x514);
    volatile uint32_t *twim_tx_ptr = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x544);

    uint8_t buf[2] = { reg, value };
    *twim_address = ACCEL_I2C_ADDR;
    *twim_tx_ptr = (uint32_t)buf;
    volatile uint32_t *twim_tx_maxcnt = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x548);
    *twim_tx_maxcnt = 2;

    volatile uint32_t *twim_tasks_starttx = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x008);
    *twim_tasks_starttx = 1;

    volatile uint32_t *twim_events_stopped = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x104);
    while (*twim_events_stopped == 0);
    *twim_events_stopped = 0;
}

static uint8_t i2c_read(uint8_t reg)
{
    /* Write register address, then read one byte */
    uint8_t tx_buf[1] = { reg };
    uint8_t rx_buf[1] = { 0 };

    volatile uint32_t *twim_address = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x514);
    *twim_address = ACCEL_I2C_ADDR;

    /* Write register address */
    volatile uint32_t *twim_tx_ptr = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x544);
    *twim_tx_ptr = (uint32_t)tx_buf;
    volatile uint32_t *twim_tx_maxcnt = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x548);
    *twim_tx_maxcnt = 1;

    volatile uint32_t *twim_tasks_starttx = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x008);
    *twim_tasks_starttx = 1;

    volatile uint32_t *twim_events_stopped = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x104);
    while (*twim_events_stopped == 0);
    *twim_events_stopped = 0;

    /* Read data */
    volatile uint32_t *twim_rx_ptr = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x54C);
    *twim_rx_ptr = (uint32_t)rx_buf;
    volatile uint32_t *twim_rx_maxcnt = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x550);
    *twim_rx_maxcnt = 1;

    volatile uint32_t *twim_tasks_startrx = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x004);
    *twim_tasks_startrx = 1;

    while (*twim_events_stopped == 0);
    *twim_events_stopped = 0;

    return rx_buf[0];
}

/*===========================================================================
 * DRIVER IMPLEMENTATION
 *===========================================================================*/

void accel_driver_init(void)
{
    /* Initialize TWIM0 for I2C communication */
    NRF_P0->PIN_CNF[BOARD_PIN_I2C_SDA] = GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_PULLUP;
    NRF_P0->PIN_CNF[BOARD_PIN_I2C_SCL] = GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_PULLUP;

    /* Configure TWIM0 */
    volatile uint32_t *twim_psel_scl = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x508);
    volatile uint32_t *twim_psel_sda = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x50C);
    *twim_psel_scl = BOARD_PIN_I2C_SCL;
    *twim_psel_sda = BOARD_PIN_I2C_SDA;

    volatile uint32_t *twim_frequency = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x524);
    *twim_frequency = 0x01980000UL;  /* 400 kHz */

    volatile uint32_t *twim_enable = (volatile uint32_t *)(NRF_TWIM0_BASE + 0x500);
    *twim_enable = 6;  /* Enable TWIM */

    /* Verify accelerometer identity */
    uint8_t who_am_i = i2c_read(LIS2DH_REG_WHO_AM_I);
    if (who_am_i != LIS2DH_WHO_AM_I_VAL) {
        /* Accelerometer not present or wrong device */
        return;
    }

    /* Configure accelerometer */
    /* CTRL1: 100 Hz ODR, all axes enabled */
    i2c_write(LIS2DH_REG_CTRL1, LIS2DH_CTRL1_VAL);

    /* CTRL2: Normal mode, HPF bypassed for INT1 */
    i2c_write(LIS2DH_REG_CTRL2, 0x00);

    /* CTRL4: ±2g scale, high resolution mode */
    i2c_write(LIS2DH_REG_CTRL4, LIS2DH_CTRL4_VAL);

    /* CTRL5: Latch interrupt request */
    i2c_write(LIS2DH_REG_CTRL5, LIS2DH_CTRL5_VAL);
}

void accel_enable_motion_interrupt(uint8_t threshold_g)
{
    /* Convert g to LSB (1 LSB ≈ 16mg at ±2g scale) */
    uint8_t threshold_lsb = (uint8_t)(threshold_g * 1000 / 16);
    if (threshold_lsb > 127) threshold_lsb = 127;

    /* Configure INT1 for activity detection */
    /* INT1_CFG: OR of X/Y/Z high events (0x2A) */
    i2c_write(LIS2DH_REG_INT1_CFG, 0x2A);

    /* INT1_THS: Threshold */
    i2c_write(LIS2DH_REG_INT1_THS, threshold_lsb);

    /* INT1_DUR: Duration (1 LSB = 1/ODR = 10ms at 100Hz) */
    i2c_write(LIS2DH_REG_INT1_DUR, 1);  /* 10ms minimum duration */

    /* CTRL3: Route INT1 to interrupt pin */
    i2c_write(LIS2DH_REG_CTRL3, LIS2DH_CTRL3_VAL);
}

void accel_disable_motion_interrupt(void)
{
    /* Disable interrupt routing */
    i2c_write(LIS2DH_REG_CTRL3, 0x00);
    i2c_write(LIS2DH_REG_INT1_CFG, 0x00);
}

uint8_t accel_read_interrupt_source(void)
{
    return i2c_read(LIS2DH_REG_INT1_SRC);
}

void accel_read_xyz(int16_t *x_out, int16_t *y_out, int16_t *z_out)
{
    /* Read 6 bytes starting from OUT_X_L (auto-increment) */
    uint8_t x_l = i2c_read(LIS2DH_REG_OUT_X_L);
    uint8_t x_h = i2c_read(LIS2DH_REG_OUT_X_H);
    uint8_t y_l = i2c_read(LIS2DH_REG_OUT_Y_L);
    uint8_t y_h = i2c_read(LIS2DH_REG_OUT_Y_H);
    uint8_t z_l = i2c_read(LIS2DH_REG_OUT_Z_L);
    uint8_t z_h = i2c_read(LIS2DH_REG_OUT_Z_H);

    /* Combine high and low bytes (12-bit left-justified) */
    *x_out = (int16_t)((x_h << 8) | x_l) >> 4;
    *y_out = (int16_t)((y_h << 8) | y_l) >> 4;
    *z_out = (int16_t)((z_h << 8) | z_l) >> 4;

    /* Convert to mg (±2g scale: 1 LSB ≈ 1mg in high-res mode) */
    *x_out *= 1;  /* Already in mg approximately */
    *y_out *= 1;
    *z_out *= 1;
}