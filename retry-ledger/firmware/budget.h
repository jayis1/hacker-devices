/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef RL_BUDGET_H
#define RL_BUDGET_H
#include <stdbool.h>
#include <stdint.h>
#define INPUT_FAIL 1u
#define INPUT_SUCCESS 2u
#define INPUT_LOCK 4u
#define INPUT_ACCESS 8u
#define QUALIFY_MS 3u
#define BASELINE_MS 20u
#define STRIKE_LIMIT 3u
#define LOCK_GRACE_MS 20u
#define LOCKOUT_MS 5000u
#define EVENT_CAPACITY 32u
#define FAULT_NO_LOCK 1u
#define FAULT_EARLY_RELEASE 2u
#define FAULT_ACCESS 4u
#define FAULT_CONFLICT 8u
#define FAULT_LOCKED_SUCCESS 16u
#define FAULT_GAP 32u
#define FAULT_UNKNOWN_ACTIVITY 64u

typedef enum { UNKNOWN, TRACKING, LOCKOUT, RECOVERY } Phase;
typedef enum {
    EV_SEGMENT = 1, EV_FAILURE, EV_SUCCESS, EV_EXHAUSTED,
    EV_LOCK_SEEN, EV_EXPIRED, EV_FAULT, EV_GAP
} EventKind;
typedef struct {
    uint32_t sequence;
    uint32_t time;
    uint32_t kind;
    uint32_t value;
} Event;
typedef struct {
    uint32_t time;
    uint32_t faults;
    uint32_t failures;
    uint32_t successes;
    uint32_t cycles;
    uint32_t blocked_failures;
    uint32_t access_ms;
    uint32_t missing_lock_ms;
    uint32_t gaps;
    uint32_t missed;
    uint32_t segments;
    uint32_t dropped;
    uint32_t event_sequence;
    uint32_t lock_age;
    uint32_t baseline_age;
    uint32_t candidate_age[4];
    uint8_t qualified_mask;
    uint8_t candidate;
    uint8_t levels;
    uint8_t strikes;
    bool qualified;
    bool lock_seen;
    Phase phase;
    Event events[EVENT_CAPACITY];
    unsigned head;
    unsigned used;
} Budget;
uint32_t budget_add(uint32_t value, uint32_t amount);
void budget_init(Budget *b, uint32_t now);
void budget_tick(Budget *b, uint32_t now, uint8_t raw);
bool budget_pop(Budget *b, Event *out);
#endif
