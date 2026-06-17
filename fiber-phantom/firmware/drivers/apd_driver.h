/*
 * apd_driver.h — APD bias control and signal monitoring driver
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Controls the Avalanche Photodiode (APD) reverse bias voltage via a
 * DAC + boost converter, and monitors the TIA output level via ADC.
 */

#ifndef FIBER_PHANTOM_APD_DRIVER_H
#define FIBER_PHANTOM_APD_DRIVER_H

#include <stdint.h>
#include "board.h"

/* APD driver state */
typedef struct {
    uint32_t bias_mv;           /* Current bias voltage in millivolts */
    uint16_t tia_level;        /* TIA output level (ADC counts, 0–4095) */
    uint8_t  boost_enabled;    /* Boost converter state */
    uint8_t  agc_enabled;      /* Auto-gain control enabled */
    uint16_t agc_target;       /* Target TIA level for AGC loop */
} apd_state_t;

/* Initialize APD bias control subsystem */
void apd_init(void);

/* Set APD bias voltage in millivolts (range: APD_BIAS_MIN_MV – APD_BIAS_MAX_MV) */
int apd_set_bias(uint32_t bias_mv);

/* Read TIA output level from ADC (returns 0–4095 for 0–3.3V) */
uint16_t apd_read_tia_level(void);

/* Enable/disable the boost converter that generates APD bias voltage */
void apd_boost_enable(uint8_t enable);

/* Run one AGC iteration: adjust bias to maintain target TIA level.
 * Returns 1 if bias was adjusted, 0 if within tolerance. */
uint8_t apd_agc_step(void);

/* Get current APD state */
const apd_state_t *apd_get_state(void);

/* Auto-calibrate APD bias: sweep bias to find optimal SNR point.
 * Scans from minimum to maximum bias, finds the point where TIA
 * output is ~70% of full-scale. Returns recommended bias in mV. */
uint32_t apd_autocalibrate(void);

/* Check APD health: returns 0 if OK, negative if fault */
int apd_health_check(void);

#endif /* FIBER_PHANTOM_APD_DRIVER_H */