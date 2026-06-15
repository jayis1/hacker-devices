/*
 * drivers/glitch.h — Glitch engine driver interface
 * Voltage/EM/Clock glitch generation with FPGA-assisted timing
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

#ifndef DRIVERS_GLITCH_H
#define DRIVERS_GLITCH_H

#include <stdint.h>
#include <stddef.h>
#include "../board.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Glitch Configuration Structure
 * ======================================================================== */

typedef struct {
    glitch_mode_t    mode;              /* Glitch mode: VCC/EM/Clock */
    glitch_shape_t   shape;             /* Pulse shape */
    trigger_mode_t   trigger_mode;      /* Trigger source */
    uint8_t          trigger_edge;      /* 0=rising, 1=falling, 2=both */
    uint32_t         delay_ns;          /* Trigger-to-glitch delay (ns) */
    uint32_t         width_ns;          /* Glitch pulse width (ns) */
    uint16_t         repeat_count;      /* Burst count (1=single) */
    uint32_t         repeat_delay_ns;   /* Inter-pulse delay (ns) */
    uint16_t         em_amplitude;      /* EM coil DAC value (0-1023) */
    uint32_t         clk_phase_offset;  /* Clock glitch phase offset (ps) */
    uint8_t          uart_pattern[4];   /* UART trigger match pattern */
    uint8_t          uart_mask;         /* UART pattern mask */
    uint8_t          jtag_state;        /* JTAG TAP state trigger */
    uint8_t          gpio_trigger_pin;  /* GPIO trigger pin select */
    uint32_t         vcc_target_mv;    /* Expected target VCC (mV) */
    uint32_t         max_current_ma;   /* Overcurrent threshold (mA) */
    uint8_t          auto_arm;         /* Auto-rearm after glitch */
    uint8_t          safety_limit;     /* Enable safety cutoff */
} glitch_config_t;

/* ========================================================================
 * Glitch Statistics
 * ======================================================================== */

typedef struct {
    uint32_t total_glitches;    /* Lifetime glitch count */
    uint32_t total_triggers;    /* Lifetime trigger count */
    uint32_t successful;        /* User-marked "interesting" results */
    uint32_t faults;            /* Fault events (overcurrent, etc) */
    int32_t  last_vcc_mv;       /* VCC at last glitch time */
    int32_t  last_current_ma;   /* Current at last glitch time */
    uint32_t last_delay_ns;     /* Actual delay of last glitch */
    uint32_t last_width_ns;     /* Actual width of last glitch */
} glitch_stats_t;

/* ========================================================================
 * Glitch Engine API
 * ======================================================================== */

/* Initialize glitch engine with default configuration */
void glitch_init(glitch_config_t *cfg);

/* Apply configuration from USB packet format */
void glitch_config_from_usb(const usb_glitch_config_t *usb_cfg,
                             glitch_config_t *cfg);

/* Convert internal config to USB format */
void glitch_config_to_usb(const glitch_config_t *cfg,
                            usb_glitch_config_t *usb_cfg);

/* Arm the glitch engine — enable trigger detection */
void glitch_arm(const glitch_config_t *cfg);

/* Disarm the glitch engine — disable all outputs */
void glitch_disarm(void);

/* Fire a glitch manually (no trigger needed) */
void glitch_fire_manual(void);

/* Fire a glitch in response to a trigger event.
 * This applies the configured delay and then fires. */
void glitch_fire_on_trigger(void);

/* Emergency cutoff — immediately disable all glitch outputs.
 * Used for overcurrent/overtemperature protection. */
void glitch_emergency_cutoff(void);

/* ========================================================================
 * Glitch Mode-specific Setup
 * ======================================================================== */

/* Configure voltage glitch (VCC shunt via MOSFET Q1) */
void glitch_setup_voltage(const glitch_config_t *cfg);

/* Configure EM glitch (coil driver via MOSFET Q2) */
void glitch_setup_em(const glitch_config_t *cfg);

/* Configure clock glitch (clock stretch via MOSFET Q3) */
void glitch_setup_clock(const glitch_config_t *cfg);

/* ========================================================================
 * Trigger Configuration
 * ======================================================================== */

/* Configure trigger source and parameters */
void glitch_setup_trigger(const glitch_config_t *cfg);

/* Enable/disable specific trigger channels */
void glitch_enable_trigger(trigger_mode_t mode);
void glitch_disable_trigger(trigger_mode_t mode);

/* ========================================================================
 * Profile Storage
 * ======================================================================== */

/* Save current configuration as profile to EEPROM */
int glitch_save_profile(uint8_t profile_id, const glitch_config_t *cfg,
                         const char *name);

/* Load profile from EEPROM */
int glitch_load_profile(uint8_t profile_id, glitch_config_t *cfg);

/* Delete profile from EEPROM */
int glitch_delete_profile(uint8_t profile_id);

/* List all stored profiles (returns count) */
int glitch_list_profiles(uint8_t *ids, char names[][16], uint8_t max);

/* ========================================================================
 * Calibration
 * ======================================================================== */

/* Run self-calibration routine:
 * - Measure baseline VCC and current
 * - Calibrate delay chain in FPGA
 * - Set ADC watchdog thresholds */
void glitch_calibrate(void);

/* Get calibration status */
int glitch_is_calibrated(void);

/* Get calibration data offsets */
int32_t glitch_get_vcc_offset(void);
int32_t glitch_get_current_offset(void);

/* ========================================================================
 * Statistics
 * ======================================================================== */

/* Get glitch statistics */
void glitch_get_stats(glitch_stats_t *stats);

/* Reset statistics counters */
void glitch_reset_stats(void);

/* Mark current glitch as "interesting" (successful attack) */
void glitch_mark_success(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_GLITCH_H */