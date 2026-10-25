/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef ES_HEALTH_H
#define ES_HEALTH_H
#include <stdbool.h>
#include <stdint.h>
#define REPEAT_LIMIT 32u
#define WINDOW_SIZE 64u
#define MATCH_LIMIT 49u
#define MIN_PERIOD_MS 10u
#define MAX_PERIOD_MS 100u
#define SETUP_MS 2u
#define FAULT_REPEAT 1u
#define FAULT_PROPORTION 2u
#define FAULT_SETUP 4u
#define FAULT_RATE 8u
#define EVENT_CAPACITY 32u
/* No raw bit history is stored, transmitted, or recoverable from this API. */
typedef struct {
    uint32_t sequence;
    uint32_t time;
    uint32_t kind;
    uint32_t value;
} Event;
typedef struct {
    uint32_t time;
    uint32_t samples;
    uint32_t windows;
    uint32_t failures;
    uint32_t gaps;
    uint32_t missed;
    uint32_t stalls;
    uint32_t unsafe_episodes;
    uint32_t unsafe_ms;
    uint32_t unannounced_ms;
    uint32_t dropped;
    uint32_t sequence;
    uint32_t edge_age;
    uint32_t bit_age;
    uint32_t run;
    uint32_t max_run;
    uint32_t window_used;
    uint32_t matches;
    uint32_t last_matches;
    uint8_t faults;
    uint8_t previous;
    uint8_t observed;
    uint8_t last_bit;
    uint8_t reference_bit;
    bool initialized;
    bool have_edge;
    bool have_bit;
    bool qualified;
    bool stalled;
    bool unsafe;
    Event events[EVENT_CAPACITY];
    unsigned head;
    unsigned count;
} Health;
uint32_t health_add(uint32_t a, uint32_t b);
void health_init(Health *h, uint32_t now);
void health_tick(Health *h, uint32_t now, uint8_t levels);
bool health_pop(Health *h, Event *event);
#endif
