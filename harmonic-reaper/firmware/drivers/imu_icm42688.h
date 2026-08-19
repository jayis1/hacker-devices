/*
 * imu_icm42688.h — IMU driver for sweep-angle tagging
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef HARMONIC_REAPER_IMU_H
#define HARMONIC_REAPER_IMU_H

#include <stdint.h>

typedef struct {
    int16_t accel_x, accel_y, accel_z;   /* mg */
    int16_t gyro_x, gyro_y, gyro_z;     /* mdps */
    int16_t pitch_deg;                   /* derived from accel + gyro */
    int16_t yaw_deg;                     /* derived from gyro integration */
} imu_sample_t;

void imu_init(void);
uint8_t imu_read(imu_sample_t *out);
void imu_reset_yaw(void);   /* zero the yaw reference for a new sweep */

#endif /* HARMONIC_REAPER_IMU_H */
/* EOF — imu_icm42688.h — jayis1 */