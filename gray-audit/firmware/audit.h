/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef GRAY_AUDIT_H
#define GRAY_AUDIT_H
#include <stdbool.h>
#include <stdint.h>
#define AXES 2u
#define EVENT_CAPACITY 32u
#define HISTOGRAM_BINS 8u
typedef enum {
    EVENT_ILLEGAL = 1,
    EVENT_REVERSAL = 2,
    EVENT_GAP = 3,
    EVENT_SATURATION = 4
} event_kind_t;
typedef struct {
    uint32_t time;
    uint32_t sequence;
    uint8_t axis;
    uint8_t before;
    uint8_t after;
    event_kind_t kind;
} audit_event_t;
typedef struct {
    int32_t position;
    int8_t direction;
    int8_t partial_cycle;
    uint8_t state;
    bool saturated;
    bool have_edge;
    uint32_t forward;
    uint32_t reverse;
    uint32_t illegal;
    uint32_t reversals;
    uint32_t edges;
    uint32_t cycles_forward;
    uint32_t cycles_reverse;
    uint32_t phase_a;
    uint32_t phase_b;
    uint32_t last_edge;
    uint32_t min_interval;
    uint32_t max_interval;
    uint32_t dwell[4];
    uint32_t histogram[HISTOGRAM_BINS];
} axis_t;
typedef struct {
    axis_t axis[AXES];
    audit_event_t events[EVENT_CAPACITY];
    uint32_t last_sample;
    uint32_t samples;
    uint32_t gaps;
    uint32_t missed;
    uint32_t dropped;
    uint32_t tx_failed;
    uint32_t sequence;
    uint8_t head;
    uint8_t count;
} audit_t;
void audit_init(audit_t *a, uint32_t now, uint8_t levels);
void audit_sample(audit_t *a, uint32_t now, uint8_t levels);
bool audit_pop(audit_t *a, audit_event_t *out);
void audit_tx_failure(audit_t *a);
uint32_t audit_age(const axis_t *axis, uint32_t now);
#endif
