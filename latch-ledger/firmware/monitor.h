/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef LATCH_MONITOR_H
#define LATCH_MONITOR_H
#include <stdbool.h>
#include <stdint.h>
#define CHANNELS 4u
#define PAIRS 2u
#define DEBOUNCE_MS 5u
#define EVENT_CAPACITY 64u

typedef struct {
    uint32_t raw_since;
    uint32_t stable_since;
    uint32_t raw_edges;
    uint32_t changes;
    uint32_t short_pulses;
    uint32_t longest_open_ms;
    uint32_t shortest_raw_ms;
    uint8_t raw;
    uint8_t stable;
    bool seen_edge;
} channel_t;

typedef struct {
    uint32_t since;
    uint32_t mismatches;
    uint32_t longest_ms;
    uint8_t active;
} pair_t;

typedef enum {
    EVENT_RAW = 1,
    EVENT_STABLE = 2,
    EVENT_PAIR = 3
} event_kind_t;

typedef struct {
    uint32_t sequence;
    uint32_t time_ms;
    uint32_t duration_ms;
    uint8_t kind;
    uint8_t channel;
    uint8_t value;
} event_t;

typedef struct {
    channel_t channel[CHANNELS];
    pair_t pair[PAIRS];
    event_t events[EVENT_CAPACITY];
    uint32_t sequence;
    uint32_t dropped;
    uint32_t missed_samples;
    uint32_t last_sample;
    uint32_t tx_failures;
    uint8_t head;
    uint8_t count;
} monitor_t;

void monitor_init(monitor_t *m, uint32_t now, uint8_t levels);
void monitor_sample(monitor_t *m, uint32_t now, uint8_t levels);
bool monitor_pop(monitor_t *m, event_t *event);
void monitor_tx_failure(monitor_t *m);
uint32_t monitor_open_ms(const channel_t *c, uint32_t now);
uint32_t monitor_pair_ms(const pair_t *p, uint32_t now);
#endif
