/*
 * thermal_ctl.c - Dual-loop PID thermal controller
 *
 * Implements cascaded PID control:
 *   Outer loop (100ms): IR sensor -> setpoint error -> TEC target adjust
 *   Inner loop (1ms):   Cold plate sensor -> fast PWM duty update
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#include "thermal_ctl.h"
#include "tec_driver.h"
#include "laser_driver.h"
#include "temp_sensor.h"
#include "registers.h"
#include <string.h>

static pid_state_t outer_pid;   /* IR sensor -> TEC setpoint */
static pid_state_t inner_pid;   /* Cold plate -> PWM duty */
static thermal_mode_t current_mode = THERMAL_MODE_IDLE;
static bool pid_enabled = false;

/* Profile execution state */
static thermal_profile_t active_profile;
static bool profile_running = false;
static uint8_t profile_step_idx = 0;
static uint32_t profile_step_start_ms = 0;
static uint16_t profile_loop_count = 0;
static uint8_t profile_loop_target = 0;
static uint8_t profile_loop_start = 0;

/* Runtime tracking */
static float current_ir_temp = 25.0f;
static float current_plate_temp = 25.0f;
static uint32_t system_tick_ms = 0;

extern volatile uint32_t systick_ms;  /* From main.c */

/* ============================================================
 * PID Controller Core
 * ============================================================ */

static float pid_compute(pid_state_t *pid, float measured, float dt)
{
    float error = pid->setpoint - measured;
    
    /* Proportional term */
    float p_term = pid->kp * error;
    
    /* Integral term with anti-windup */
    pid->integral += error * dt;
    if (pid->integral > PID_INTEGRAL_LIMIT)
        pid->integral = PID_INTEGRAL_LIMIT;
    if (pid->integral < -PID_INTEGRAL_LIMIT)
        pid->integral = -PID_INTEGRAL_LIMIT;
    float i_term = pid->ki * pid->integral;
    
    /* Derivative term */
    float derivative = (error - pid->prev_error) / dt;
    float d_term = pid->kd * derivative;
    pid->prev_error = error;
    
    /* Compute output */
    pid->output = p_term + i_term + d_term;
    
    /* Clamp output */
    if (pid->output > pid->output_max)
        pid->output = pid->output_max;
    if (pid->output < pid->output_min)
        pid->output = pid->output_min;
    
    return pid->output;
}

static void pid_init(pid_state_t *pid, float kp, float ki, float kd,
                     float out_min, float out_max)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->setpoint = 25.0f;
    pid->output = 0.0f;
    pid->output_min = out_min;
    pid->output_max = out_max;
}

/* ============================================================
 * Public API
 * ============================================================ */

void thermal_ctl_init(void)
{
    /* Outer loop: slower, wider integral range */
    pid_init(&outer_pid, PID_DEFAULT_KP, PID_DEFAULT_KI, PID_DEFAULT_KD,
             -100.0f, 100.0f);
    
    /* Inner loop: fast response, tighter limits */
    pid_init(&inner_pid, 15.0f, 1.0f, 5.0f,
             -100.0f, 100.0f);
    
    current_mode = THERMAL_MODE_IDLE;
    pid_enabled = false;
    profile_running = false;
    profile_step_idx = 0;
    
    memset(&active_profile, 0, sizeof(active_profile));
}

void thermal_ctl_set_target(float temp_c)
{
    /* Safety: clamp to allowed range */
    if (temp_c > SAFETY_MAX_TEMP_C) {
        temp_c = SAFETY_MAX_TEMP_C;
    }
    if (temp_c < SAFETY_MIN_TEMP_C) {
        temp_c = SAFETY_MIN_TEMP_C;
    }
    outer_pid.setpoint = temp_c;
    inner_pid.setpoint = temp_c;
    pid_enabled = true;
}

float thermal_ctl_get_target(void)
{
    return outer_pid.setpoint;
}

float thermal_ctl_get_current(void)
{
    return current_ir_temp;
}

void thermal_ctl_set_mode(thermal_mode_t mode)
{
    current_mode = mode;
    if (mode == THERMAL_MODE_IDLE) {
        pid_enabled = false;
        tec_set_duty(0.0f);
        laser_disable();
    }
}

thermal_mode_t thermal_ctl_get_mode(void)
{
    return current_mode;
}

void thermal_ctl_set_pid(float kp, float ki, float kd)
{
    outer_pid.kp = kp;
    outer_pid.ki = ki;
    outer_pid.kd = kd;
    outer_pid.integral = 0.0f;
    outer_pid.prev_error = 0.0f;
}

void thermal_ctl_get_pid(float *kp, float *ki, float *kd)
{
    if (kp) *kp = outer_pid.kp;
    if (ki) *ki = outer_pid.ki;
    if (kd) *kd = outer_pid.kd;
}

void thermal_ctl_reset_pid(void)
{
    outer_pid.integral = 0.0f;
    outer_pid.prev_error = 0.0f;
    outer_pid.output = 0.0f;
    inner_pid.integral = 0.0f;
    inner_pid.prev_error = 0.0f;
    inner_pid.output = 0.0f;
}

void thermal_ctl_stop(void)
{
    pid_enabled = false;
    profile_running = false;
    tec_set_duty(0.0f);
    laser_disable();
    current_mode = THERMAL_MODE_IDLE;
}

/* ============================================================
 * Fast tick - called every 1ms from SysTick
 * Inner PID loop: uses cold plate temperature for fast PWM adjust
 * ============================================================ */
void thermal_ctl_fast_tick(float plate_temp)
{
    current_plate_temp = plate_temp;
    
    if (!pid_enabled)
        return;
    
    float dt = 0.001f;  /* 1ms */
    
    /* Inner loop PID: maintain plate temp at outer loop setpoint */
    float duty = pid_compute(&inner_pid, plate_temp, dt);
    
    /* Apply to TEC driver */
    /* Positive duty = heat, negative = cool */
    tec_set_duty(duty);
}

/* ============================================================
 * Slow tick - called every 100ms
 * Outer PID loop: uses IR sensor (target surface) temperature
 * Adjusts inner loop setpoint based on outer loop error
 * ============================================================ */
void thermal_ctl_slow_tick(float ir_temp)
{
    current_ir_temp = ir_temp;
    
    if (!pid_enabled)
        return;
    
    float dt = 0.1f;  /* 100ms */
    
    /* Outer loop PID: drive IR temp to setpoint */
    float outer_output = pid_compute(&outer_pid, ir_temp, dt);
    
    /* Adjust inner loop setpoint: outer output offsets the plate target
     * from the IR setpoint. If IR is below target, plate must be hotter
     * to push heat into the target. */
    float inner_target = outer_pid.setpoint + outer_output * 0.5f;
    
    /* Clamp inner target to safe range */
    if (inner_target > SAFETY_MAX_TEMP_C + 10.0f)
        inner_target = SAFETY_MAX_TEMP_C + 10.0f;
    if (inner_target < SAFETY_MIN_TEMP_C - 10.0f)
        inner_target = SAFETY_MIN_TEMP_C - 10.0f;
    
    inner_pid.setpoint = inner_target;
    
    /* Profile execution */
    if (profile_running) {
        thermal_ctl_profile_tick();
    }
}

/* ============================================================
 * Profile Execution
 * ============================================================ */

bool thermal_ctl_load_profile(const thermal_profile_t *profile)
{
    if (!profile || profile->step_count == 0 || 
        profile->step_count > MAX_PROFILE_STEPS)
        return false;
    
    memcpy(&active_profile, profile, sizeof(thermal_profile_t));
    profile_step_idx = 0;
    profile_loop_count = 0;
    return true;
}

bool thermal_ctl_start_profile(void)
{
    if (active_profile.step_count == 0)
        return false;
    
    profile_running = true;
    profile_step_idx = 0;
    profile_step_start_ms = systick_ms;
    profile_loop_count = 0;
    current_mode = THERMAL_MODE_PROFILE;
    pid_enabled = true;
    
    /* Execute first step */
    thermal_ctl_profile_tick();
    return true;
}

void thermal_ctl_stop_profile(void)
{
    profile_running = false;
    profile_step_idx = 0;
    thermal_ctl_stop();
}

bool thermal_ctl_is_profile_running(void)
{
    return profile_running;
}

uint8_t thermal_ctl_get_profile_step(void)
{
    return profile_step_idx;
}

uint32_t thermal_ctl_get_profile_step_time_ms(void)
{
    return systick_ms - profile_step_start_ms;
}

void thermal_ctl_profile_tick(void)
{
    if (!profile_running || profile_step_idx >= active_profile.step_count)
        return;
    
    profile_step_t *step = &active_profile.steps[profile_step_idx];
    uint32_t elapsed = systick_ms - profile_step_start_ms;
    
    switch (step->type) {
    case PROFILE_STEP_HEAT:
        thermal_ctl_set_target(step->target_temp);
        current_mode = THERMAL_MODE_HEAT;
        break;
        
    case PROFILE_STEP_COOL:
        thermal_ctl_set_target(step->target_temp);
        current_mode = THERMAL_MODE_COOL;
        break;
        
    case PROFILE_STEP_HOLD:
        /* Maintain current setpoint */
        break;
        
    case PROFILE_STEP_LASER:
        /* Fire laser for specified duration */
        if (elapsed == 0 || elapsed < step->laser_duration) {
            float power = CLAMP(step->laser_power, 0.0f, 100.0f);
            laser_fire(power, step->laser_duration);
        }
        break;
        
    case PROFILE_STEP_TRIGGER:
        /* Output trigger pulse */
        trigger_output_pulse(10);  /* 10µs pulse */
        break;
        
    case PROFILE_STEP_WAIT_TRIGGER:
        /* Wait for external trigger before continuing */
        if (!trigger_is_pending()) {
            return;  /* Stay on this step */
        }
        trigger_clear_pending();
        break;
        
    case PROFILE_STEP_LOOP:
        if (profile_loop_count == 0) {
            profile_loop_target = step->loop_count;
            profile_loop_start = profile_step_idx;
        }
        profile_loop_count++;
        if (profile_loop_count < profile_loop_target) {
            profile_step_idx = profile_loop_start;
            profile_step_start_ms = systick_ms;
            return;
        }
        /* Loop complete, reset for next time */
        profile_loop_count = 0;
        break;
        
    case PROFILE_STEP_END:
        profile_running = false;
        thermal_ctl_stop();
        return;
    }
    
    /* Advance to next step if duration elapsed */
    if (step->type != PROFILE_STEP_WAIT_TRIGGER &&
        step->type != PROFILE_STEP_END &&
        elapsed >= step->duration_ms) {
        profile_step_idx++;
        profile_step_start_ms = systick_ms;
        
        if (profile_step_idx >= active_profile.step_count) {
            profile_running = false;
            thermal_ctl_stop();
        }
    }
}

/* Forward declarations for trigger functions used above */
extern void trigger_output_pulse(uint32_t us);
extern bool trigger_is_pending(void);
extern void trigger_clear_pending(void);