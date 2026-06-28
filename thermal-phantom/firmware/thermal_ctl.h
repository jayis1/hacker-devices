/*
 * thermal_ctl.h - Thermal PID control system header
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#ifndef THERMAL_CTL_H
#define THERMAL_CTL_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

typedef struct {
    float kp;
    float ki;
    float kd;
    float integral;
    float prev_error;
    float setpoint;
    float output;
    float output_min;
    float output_max;
} pid_state_t;

typedef enum {
    THERMAL_MODE_IDLE = 0,
    THERMAL_MODE_HEAT,
    THERMAL_MODE_COOL,
    THERMAL_MODE_LASER,
    THERMAL_MODE_PROFILE
} thermal_mode_t;

void thermal_ctl_init(void);
void thermal_ctl_set_target(float temp_c);
float thermal_ctl_get_target(void);
float thermal_ctl_get_current(void);
void thermal_ctl_set_mode(thermal_mode_t mode);
thermal_mode_t thermal_ctl_get_mode(void);
void thermal_ctl_set_pid(float kp, float ki, float kd);
void thermal_ctl_get_pid(float *kp, float *ki, float *kd);
void thermal_ctl_reset_pid(void);
void thermal_ctl_stop(void);

/* Called from 1ms systick: fast inner loop */
void thermal_ctl_fast_tick(float plate_temp);

/* Called from 100ms timer: slow outer loop */
void thermal_ctl_slow_tick(float ir_temp);

/* Profile execution */
bool thermal_ctl_load_profile(const thermal_profile_t *profile);
bool thermal_ctl_start_profile(void);
void thermal_ctl_stop_profile(void);
bool thermal_ctl_is_profile_running(void);
uint8_t thermal_ctl_get_profile_step(void);
uint32_t thermal_ctl_get_profile_step_time_ms(void);

#endif /* THERMAL_CTL_H */