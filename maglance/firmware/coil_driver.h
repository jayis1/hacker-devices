/*
 * coil_driver.h — H-bridge coil driver interface for MagLance
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Controls the bipolar H-bridge that drives the coil assembly.
 * Uses HRTIM for sub-nanosecond pulse timing.
 */

#ifndef COIL_DRIVER_H
#define COIL_DRIVER_H

#include "board.h"
#include "registers.h"

/* ---- H-bridge state ---- */
typedef enum {
    BRIDGE_IDLE = 0,       /* All MOSFETs off, coil disconnected */
    BRIDGE_FORWARD,        /* Q1+Q4 on: current left→right (NORTH polarity) */
    BRIDGE_REVERSE,        /* Q2+Q3 on: current right→left (SOUTH polarity) */
    BRIDGE_BRAKE,          /* Q2+Q4 on: coil shorted (fast decay) */
    BRIDGE_FAULT,          /* Safety trip — all off, latched */
} bridge_state_t;

/* ---- PI controller state (for closed-loop current control) ---- */
typedef struct {
    int32_t  integral;     /* Integral accumulator */
    int32_t  kp;           /* Proportional gain (Q15 fixed-point) */
    int32_t  ki;           /* Integral gain (Q15 fixed-point) */
    int32_t  out_min;      /* Minimum output (0% duty) */
    int32_t  out_max;      /* Maximum output (100% duty) */
} pi_controller_t;

/* ---- Sweep state ---- */
typedef struct {
    uint32_t start_hz;      /* Sweep start frequency */
    uint32_t stop_hz;       /* Sweep stop frequency */
    uint16_t steps;         /* Number of frequency steps */
    uint16_t dwell_ms;     /* Dwell time per step */
    uint16_t current_step;  /* Current step index */
    uint32_t step_timer;   /* Counter for dwell timing */
    uint8_t  active;       /* Sweep running flag */
} sweep_state_t;

/* ---- Public API ---- */

/* Initialize HRTIM and H-bridge GPIOs */
void coil_driver_init(void);

/* Arm the coil driver (requires safety switch engaged) */
int coil_driver_arm(void);

/* Disarm the coil driver (immediate stop, all MOSFETs off) */
void coil_driver_disarm(void);

/* Fire a single pulse */
int coil_driver_pulse(uint32_t width_ns, uint32_t current_ma,
                      polarity_t polarity);

/* Fire multiple pulses with delay between them */
int coil_driver_pulse_burst(uint32_t width_ns, uint32_t current_ma,
                             polarity_t polarity,
                             uint32_t count, uint32_t delay_us);

/* Start continuous DC field */
int coil_driver_dc_start(uint32_t current_ma, polarity_t polarity);

/* Stop DC field */
void coil_driver_dc_stop(void);

/* Start frequency sweep */
int coil_driver_sweep_start(uint32_t start_hz, uint32_t stop_hz,
                            uint16_t steps, uint16_t dwell_ms,
                            uint32_t current_ma);

/* Stop frequency sweep */
void coil_driver_sweep_stop(void);

/* Update closed-loop current control (call from main loop) */
void coil_driver_pi_update(void);

/* Get current bridge state */
bridge_state_t coil_driver_get_state(void);

/* Get measured coil current in milliamps */
uint32_t coil_driver_get_current_ma(void);

/* Emergency stop (called from safety interrupt) */
void coil_driver_emergency_stop(void);

/* Get/set PI controller gains */
void coil_driver_set_pi_gains(int32_t kp, int32_t ki);
void coil_driver_get_pi_gains(int32_t *kp, int32_t *ki);

/* Convert desired current (mA) to HRTIM compare value */
uint32_t coil_driver_current_to_cmp(uint32_t current_ma);

/* Convert desired pulse width (ns) to HRTIM period value */
uint32_t coil_driver_width_to_period(uint32_t width_ns);

/* Update sweep state machine (call from 1 ms tick) */
void coil_driver_sweep_tick(void);

/* ---- Externs ---- */
extern bridge_state_t g_bridge_state;
extern pi_controller_t g_pi;
extern sweep_state_t  g_sweep;

#endif /* COIL_DRIVER_H */