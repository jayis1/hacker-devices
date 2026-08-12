/**
 * @file accel_driver.h
 * @brief LIS2DH12 accelerometer driver for motion-triggered capture
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef ACCEL_DRIVER_H
#define ACCEL_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Accelerometer interrupt sources */
#define ACCEL_INT_CLICK     0x01
#define ACCEL_INT_MOTION    0x02
#define ACCEL_INT_FREEFALL  0x04
#define ACCEL_INT_TILT      0x08

/**
 * @brief Initialize accelerometer
 */
void accel_driver_init(void);

/**
 * @brief Enable motion detection interrupt
 * @param threshold_g Threshold in g (1-16)
 */
void accel_enable_motion_interrupt(uint8_t threshold_g);

/**
 * @brief Disable motion detection interrupt
 */
void accel_disable_motion_interrupt(void);

/**
 * @brief Read interrupt source register
 * @return Interrupt source flags
 */
uint8_t accel_read_interrupt_source(void);

/**
 * @brief Read acceleration values
 * @param x_out X acceleration (mg)
 * @param y_out Y acceleration (mg)
 * @param z_out Z acceleration (mg)
 */
void accel_read_xyz(int16_t *x_out, int16_t *y_out, int16_t *z_out);

#ifdef __cplusplus
}
#endif

#endif /* ACCEL_DRIVER_H */