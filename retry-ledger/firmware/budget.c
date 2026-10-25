/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "budget.h"
#include <string.h>
#include <limits.h>

uint32_t budget_add(uint32_t value, uint32_t amount)
{
    if (UINT32_MAX - value < amount)
        return UINT32_MAX;
    return value + amount;
}

static void event(Budget *b, EventKind kind, uint32_t value)
{
    /* Sequence counts attempted events, so queue loss remains detectable. */
    b->event_sequence++;
    if (b->used == EVENT_CAPACITY) {
        b->dropped = budget_add(b->dropped, 1);
        return;
    }
    unsigned slot = (b->head + b->used) % EVENT_CAPACITY;
    b->events[slot].sequence = b->event_sequence;
    b->events[slot].time = b->time;
    b->events[slot].kind = (uint32_t)kind;
    b->events[slot].value = value;
    b->used++;
}

static void fault(Budget *b, uint32_t mask)
{
    /* Sticky masks are lifetime evidence, not current-state indicators. */
    uint32_t added = mask & ~b->faults;
    b->faults |= mask;
    if (added)
        event(b, EV_FAULT, added);
}

bool budget_pop(Budget *b, Event *out)
{
    if (!b || !out || !b->used)
        return false;
    *out = b->events[b->head];
    b->head = (b->head + 1u) % EVENT_CAPACITY;
    b->used--;
    return true;
}

void budget_init(Budget *b, uint32_t now)
{
    memset(b, 0, sizeof *b);
    b->time = now;
    b->phase = UNKNOWN;
}

static void invalidate(Budget *b, uint32_t dt, uint8_t raw)
{
    b->gaps = budget_add(b->gaps, 1);
    b->missed = budget_add(b->missed, dt - 1u);
    b->phase = UNKNOWN;
    b->qualified = false;
    b->candidate = raw;
    memset(b->candidate_age, 0, sizeof b->candidate_age);
    b->qualified_mask = 0;
    b->baseline_age = 0;
    b->lock_age = 0;
    b->lock_seen = false;
    fault(b, FAULT_GAP);
    event(b, EV_GAP, dt - 1u);
}

static bool qualify(Budget *b, uint8_t raw)
{
    /* Independent channels prevent unrelated chatter hiding sustained levels. */
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw ^ b->candidate) & bit) {
            b->candidate ^= bit;
            b->candidate_age[i] = 0;
        } else {
            b->candidate_age[i] = budget_add(b->candidate_age[i], 1);
            if (b->candidate_age[i] >= QUALIFY_MS) {
                b->levels = (uint8_t)((b->levels & ~bit) | (raw & bit));
                b->qualified_mask |= bit;
            }
        }
    }
    return b->qualified_mask == 15u;
}

static void baseline(Budget *b, uint8_t levels)
{
    /* A new segment is only prospective; it cannot reconstruct old attempts. */
    if (levels != 0 || b->candidate != 0) {
        b->baseline_age = 0;
        return;
    }
    b->baseline_age = budget_add(b->baseline_age, 1);
    if (b->baseline_age < BASELINE_MS)
        return;
    b->phase = TRACKING;
    b->strikes = 0;
    b->lock_age = 0;
    b->lock_seen = false;
    b->baseline_age = 0;
    b->segments = budget_add(b->segments, 1);
    event(b, EV_SEGMENT, b->segments);
}

static void exhaust(Budget *b, uint8_t levels)
{
    b->phase = LOCKOUT;
    b->lock_age = 0;
    b->lock_seen = (levels & INPUT_LOCK) != 0;
    b->cycles = budget_add(b->cycles, 1);
    event(b, EV_EXHAUSTED, b->cycles);
    if (b->lock_seen)
        event(b, EV_LOCK_SEEN, 0);
}

static void attempts(Budget *b, uint8_t rising, uint8_t levels)
{
    bool failed = (rising & INPUT_FAIL) != 0;
    bool success = (rising & INPUT_SUCCESS) != 0;
    /* Conflict includes overlapping levels, not merely simultaneous edges. */
    if ((levels & (INPUT_FAIL | INPUT_SUCCESS)) == (INPUT_FAIL | INPUT_SUCCESS))
        fault(b, FAULT_CONFLICT);
    if (failed) {
        b->failures = budget_add(b->failures, 1);
        event(b, EV_FAILURE, b->failures);
        if (b->phase == TRACKING) {
            if (b->strikes < STRIKE_LIMIT)
                b->strikes++;
            if (b->strikes == STRIKE_LIMIT)
                exhaust(b, levels);
        } else if (b->phase == LOCKOUT || b->phase == RECOVERY) {
            b->blocked_failures = budget_add(b->blocked_failures, 1);
        } else {
            fault(b, FAULT_UNKNOWN_ACTIVITY);
        }
    }
    if (success) {
        if (b->phase == LOCKOUT || b->phase == RECOVERY || (levels & INPUT_LOCK)) {
            fault(b, FAULT_LOCKED_SUCCESS);
        } else if (b->phase == TRACKING && !(levels & INPUT_FAIL)) {
            b->strikes = 0;
            b->successes = budget_add(b->successes, 1);
            event(b, EV_SUCCESS, b->successes);
        } else if (b->phase == UNKNOWN) {
            fault(b, FAULT_UNKNOWN_ACTIVITY);
        }
    }
}

static void lock_observation(Budget *b, uint8_t levels)
{
    if (b->phase != LOCKOUT)
        return;
    if (levels & INPUT_LOCK) {
        if (!b->lock_seen) {
            b->lock_seen = true;
            event(b, EV_LOCK_SEEN, b->lock_age);
        }
    } else {
        if (b->lock_seen)
            fault(b, FAULT_EARLY_RELEASE);
        if (b->lock_age >= LOCK_GRACE_MS) {
            fault(b, FAULT_NO_LOCK);
            b->missing_lock_ms = budget_add(b->missing_lock_ms, 1);
        }
    }
}

static void access_observation(Budget *b, uint8_t levels)
{
    /* Evaluate before expiry: a same-tick release does not establish ordering. */
    if (!(levels & INPUT_ACCESS))
        return;
    if (b->phase != TRACKING || (levels & INPUT_LOCK)) {
        fault(b, FAULT_ACCESS);
        b->access_ms = budget_add(b->access_ms, 1);
    }
}

void budget_tick(Budget *b, uint32_t now, uint8_t raw)
{
    uint32_t dt = now - b->time;
    if (!dt)
        return;
    b->time = now;
    raw &= 15u;
    if (dt != 1u) {
        invalidate(b, dt, raw);
        return;
    }
    /* Lock age is elapsed observation time, not time since last input edge. */
    if (b->phase == LOCKOUT)
        b->lock_age = budget_add(b->lock_age, 1);
    uint8_t rising = 0;
    uint8_t previous = b->levels;
    if (qualify(b, raw)) {
        if (b->qualified)
            rising = (uint8_t)(b->levels & ~previous);
        else {
            /* Startup asserted inputs are observations, not invented edges. */
            if (b->levels)
                fault(b, FAULT_UNKNOWN_ACTIVITY);
            b->qualified = true;
        }
    }
    if (!b->qualified)
        return;
    attempts(b, rising, b->levels);
    lock_observation(b, b->levels);
    access_observation(b, b->levels);
    if (b->phase == LOCKOUT && b->lock_age >= LOCKOUT_MS) {
        b->phase = RECOVERY;
        b->baseline_age = 0;
        event(b, EV_EXPIRED, b->lock_age);
    }
    if (b->phase == UNKNOWN || b->phase == RECOVERY)
        baseline(b, b->levels);
}
