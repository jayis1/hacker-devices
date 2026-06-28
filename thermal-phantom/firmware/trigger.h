/*
 * trigger.h - Hardware trigger I/O engine header
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#ifndef TRIGGER_H
#define TRIGGER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TRIG_SOURCE_NONE = 0,
    TRIG_SOURCE_EXTERNAL,
    TRIG_SOURCE_MANUAL,
    TRIG_SOURCE_AUTO
} trigger_source_t;

typedef enum {
    TRIG_EDGE_NONE = 0,
    TRIG_EDGE_RISING,
    TRIG_EDGE_FALLING,
    TRIG_EDGE_BOTH
} trigger_edge_t;

typedef struct {
    trigger_source_t source;
    trigger_edge_t edge;
    uint32_t delay_us;       /* Delay after trigger before action */
    uint32_t pulse_width_us; /* Output pulse width */
    bool armed;
    bool triggered;
} trigger_config_t;

void trigger_init(void);
void trigger_arm(trigger_source_t source, trigger_edge_t edge, uint32_t delay_us);
void trigger_disarm(void);
bool trigger_is_armed(void);
bool trigger_is_pending(void);
void trigger_clear_pending(void);
void trigger_output_pulse(uint32_t width_us);
trigger_config_t *trigger_get_config(void);
void trigger_set_config(const trigger_config_t *config);

#endif /* TRIGGER_H */