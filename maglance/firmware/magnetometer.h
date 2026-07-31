/*
 * magnetometer.h — RM3100 3-axis magnetometer driver interface
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Driver for PNI RM3100 3-axis magnetometer via SPI.
 * Three sensors are used at different distances from the coil
 * for gradient-based source localization.
 */

#ifndef MAGNETOMETER_H
#define MAGNETOMETER_H

#include "board.h"
#include "registers.h"
#include <stdint.h>

/* ---- RM3100 Register Addresses ---- */
#define RM3100_REVID            0x36  /* Revision ID */
#define RM3100_REVID_EXPECTED   0x22  /* Expected revision */

#define RM3100_CMM              0x01  /* Continuous Measurement Mode */
#define RM3100_CMM_START        (1U << 0)
#define RM3100_CMM_DRDM         (1U << 7)  /* Data-ready data mode */
#define RM3100_CMM_ALL_AXES     (0x70U << 1)  /* Measure X, Y, Z */

#define RM3100_CCR              0x04  /* Cycle Count Registers (TMRC) */
#define RM3100_POLL             0x00  /* Poll measurement */
#define RM3100_POLL_X           (1U << 0)
#define RM3100_POLL_Y           (1U << 1)
#define RM3100_POLL_Z           (1U << 2)
#define RM3100_POLL_ALL         0x07

#define RM3100_CMM_X_AXIS       (1U << 1)
#define RM3100_CMM_Y_AXIS       (1U << 2)
#define RM3100_CMM_Z_AXIS       (1U << 3)

#define RM3100_BIST             0x02  /* Built-in self test */
#define RM3100_BIST_START       (1U << 0)
#define RM3100_BIST_X          (1U << 4)
#define RM3100_BIST_Y          (1U << 5)
#define RM3100_BIST_Z          (1U << 6)

#define RM3100_MX               0x24  /* X-axis measurement (24-bit, 3 bytes) */
#define RM3100_MY               0x27  /* Y-axis measurement */
#define RM3100_MZ               0x2A  /* Z-axis measurement */

#define RM3100_CCX              0x05  /* Cycle count X (16-bit) */
#define RM3100_CCY              0x07  /* Cycle count Y */
#define RM3100_CCZ              0x09  /* Cycle count Z */

#define RM3100_STATUS           0x34  /* Status register */
#define RM3100_STATUS_DRDY      (1U << 7)  /* Data ready */
#define RM3100_STATUS_NAK       (1U << 0)  /* NAK error */

#define RM3100_TMRC             0x0B  /* Update rate */
#define RM3100_TMRC_600HZ       0x92  /* 600 Hz update rate */
#define RM3100_TMRC_300HZ       0x93  /* 300 Hz */
#define RM3100_TMRC_150HZ       0x94  /* 150 Hz */
#define RM3100_TMRC_75HZ        0x95  /* 75 Hz */
#define RM3100_TMRC_37HZ        0x96  /* 37 Hz */

/* ---- Data Structures ---- */

/* Single 3-axis reading (in µT × 100, i.e., centi-µT) */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} mag_reading_t;

/* Full array reading from all 3 sensors */
typedef struct {
    mag_reading_t sensor[3];  /* sensor[0]=0mm, [1]=15mm, [2]=30mm */
    uint32_t      timestamp_ms;
} mag_array_t;

/* ---- Public API ---- */

/* Initialize SPI2 and all three RM3100 sensors */
int magnetometer_init(void);

/* Read all three sensors */
int magnetometer_read_all(mag_array_t *reading);

/* Read a single sensor (0, 1, or 2) */
int magnetometer_read(uint8_t sensor_idx, mag_reading_t *reading);

/* Start continuous measurement mode at specified rate */
int magnetometer_start_continuous(uint8_t tmrc_rate);

/* Stop continuous measurement */
void magnetometer_stop_continuous(void);

/* Check if data is ready */
int magnetometer_data_ready(uint8_t sensor_idx);

/* Convert raw 24-bit reading to µT × 100 (centi-µT) */
int16_t magnetometer_raw_to_centiut(int32_t raw, uint16_t cycle_count);

/* Calculate gradient between sensors (for source localization) */
void magnetometer_calculate_gradient(const mag_array_t *reading,
                                      float *grad_x,
                                      float *grad_y,
                                      float *grad_z);

/* Run built-in self test for a specific sensor */
int magnetometer_self_test(uint8_t sensor_idx);

/* Set cycle count (sensitivity vs. speed tradeoff) */
int magnetometer_set_cycle_count(uint8_t sensor_idx, uint16_t count);

/* Get the latest reading (cached from last read) */
const mag_array_t *magnetometer_get_latest(void);

#endif /* MAGNETOMETER_H */