/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef EW_EPOCH_H
#define EW_EPOCH_H
#include <stdbool.h>
#include <stdint.h>
#define INPUT_COMMIT 4u
#define INPUT_EXECUTE 8u
#define QUALIFY_MS 3u
#define BASELINE_MS 20u
#define SETUP_MS 10u
#define MIN_COMMIT_MS 10u
#define MAX_COMMIT_MS 1000u
#define EVENT_CAPACITY 32u
typedef enum { UNKNOWN, READY, CAPTURE } Phase;
typedef enum {
    FAULT_SETUP=1u, FAULT_HOLD=2u, FAULT_WIDTH=4u,
    FAULT_TIMEOUT=8u, FAULT_ROLLBACK=16u, FAULT_EXECUTE=32u,
    FAULT_GAP=64u, FAULT_STARTUP=128u
} Fault;
typedef enum {
    EV_READY=1, EV_BEGIN, EV_ACCEPT, EV_DOWNGRADE, EV_INVALID,
    EV_FAULT, EV_GAP
} EventKind;
typedef struct {
    uint32_t sequence, time, kind, value;
} Event;
typedef struct {
    uint32_t time;
    Phase phase;
    bool qualified, valid, have_highest;
    uint8_t levels, candidate, qualified_mask;
    uint8_t highest, current, pending;
    uint32_t candidate_age[4];
    uint32_t setup_age, baseline_age, capture_age;
    bool bad_capture;
    uint32_t faults, accepted, downgrades, invalid, execute_ms;
    uint32_t gaps, missed, segments, dropped;
    uint32_t event_sequence;
    unsigned head, used;
    Event events[EVENT_CAPACITY];
} Epoch;
uint32_t epoch_add(uint32_t value, uint32_t amount);
void epoch_init(Epoch *e, uint32_t now);
void epoch_tick(Epoch *e, uint32_t now, uint8_t raw);
bool epoch_pop(Epoch *e, Event *out);
#endif
