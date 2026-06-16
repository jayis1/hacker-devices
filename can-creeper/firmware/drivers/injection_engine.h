/**
 * @file injection_engine.h
 * @brief CAN Frame Injection Engine API
 *
 * Manages scripted CAN frame injection sequences for fuzzing,
 * replay attacks, and diagnostic probing.
 */

#ifndef INJECTION_ENGINE_H
#define INJECTION_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "can_fd_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t     channel;
    can_frame_t frame;
    uint32_t    delay_after_us;
    uint32_t    repeat_count;
    uint32_t    repeat_interval_us;
} injection_step_t;

typedef struct {
    char               name[32];
    injection_step_t  *steps;
    uint16_t           step_count;
    uint16_t           current_step;
    uint32_t           total_repeats;
    uint32_t           repeat_counter;
    bool               running;
    bool               paused;
} injection_script_t;

int injection_init(void);
int injection_script_load(injection_script_t *script, const uint8_t *json_data, uint16_t len);
int injection_script_start(injection_script_t *script);
int injection_script_stop(injection_script_t *script);
int injection_script_pause(injection_script_t *script);
int injection_script_resume(injection_script_t *script);
void injection_process(injection_script_t *script);
void injection_timer_isr(void);

#ifdef __cplusplus
}
#endif
#endif
