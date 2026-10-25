/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "epoch.h"
#include <string.h>
#include <limits.h>

uint32_t epoch_add(uint32_t value, uint32_t amount)
{
    if (UINT32_MAX - value < amount)
        return UINT32_MAX;
    return value + amount;
}

static void event(Epoch *e, EventKind kind, uint32_t value)
{
    /* Sequence includes dropped events. Loss never blocks acquisition. */
    e->event_sequence++;
    if (e->used == EVENT_CAPACITY) {
        e->dropped = epoch_add(e->dropped, 1);
        return;
    }
    unsigned slot = (e->head + e->used) % EVENT_CAPACITY;
    e->events[slot].sequence = e->event_sequence;
    e->events[slot].time = e->time;
    e->events[slot].kind = (uint32_t)kind;
    e->events[slot].value = value;
    e->used++;
}

static void fault(Epoch *e, uint32_t mask)
{
    uint32_t added = mask & ~e->faults;
    e->faults |= mask;
    if (added)
        event(e, EV_FAULT, added);
}

bool epoch_pop(Epoch *e, Event *out)
{
    if (!e || !out || !e->used)
        return false;
    *out = e->events[e->head];
    e->head = (e->head + 1u) % EVENT_CAPACITY;
    e->used--;
    return true;
}

void epoch_init(Epoch *e, uint32_t now)
{
    memset(e, 0, sizeof *e);
    e->time = now;
    e->phase = UNKNOWN;
}

static void invalidate(Epoch *e, uint32_t dt, uint8_t raw)
{
    e->gaps = epoch_add(e->gaps, 1);
    e->missed = epoch_add(e->missed, dt - 1u);
    e->phase = UNKNOWN;
    e->valid = false;
    e->qualified = false;
    e->candidate = raw;
    memset(e->candidate_age, 0, sizeof e->candidate_age);
    e->qualified_mask = 0;
    e->setup_age = 0;
    e->baseline_age = 0;
    e->capture_age = 0;
    /* Highest accepted generation survives gaps, unlike current validity. */
    fault(e, FAULT_GAP);
    event(e, EV_GAP, dt - 1u);
}

static bool qualify(Epoch *e, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw ^ e->candidate) & bit) {
            e->candidate ^= bit;
            e->candidate_age[i] = 0;
        } else {
            e->candidate_age[i] = epoch_add(e->candidate_age[i], 1);
            if (e->candidate_age[i] >= QUALIFY_MS) {
                e->levels = (uint8_t)((e->levels & ~bit) | (raw & bit));
                e->qualified_mask |= bit;
            }
        }
    }
    return e->qualified_mask == 15u;
}

static void baseline(Epoch *e)
{
    /* Generation bits need not be zero: only handshake and execution idle. */
    if ((e->levels | e->candidate) & (INPUT_COMMIT | INPUT_EXECUTE)) {
        e->baseline_age = 0;
        return;
    }
    e->baseline_age = epoch_add(e->baseline_age, 1);
    if (e->baseline_age < BASELINE_MS)
        return;
    e->phase = READY;
    e->baseline_age = 0;
    e->segments = epoch_add(e->segments, 1);
    event(e, EV_READY, e->segments);
}

static void begin_capture(Epoch *e)
{
    e->phase = CAPTURE;
    e->valid = false;
    e->capture_age = 0;
    e->pending = e->levels & 3u;
    e->bad_capture = e->setup_age < SETUP_MS;
    if (e->bad_capture)
        fault(e, FAULT_SETUP);
    event(e, EV_BEGIN, e->pending);
}

static void finish_capture(Epoch *e)
{
    if (e->capture_age < MIN_COMMIT_MS) {
        e->bad_capture = true;
        fault(e, FAULT_WIDTH);
    }
    if (e->bad_capture) {
        e->invalid = epoch_add(e->invalid, 1);
        event(e, EV_INVALID, e->pending);
    } else if (e->have_highest && e->pending < e->highest) {
        e->downgrades = epoch_add(e->downgrades, 1);
        fault(e, FAULT_ROLLBACK);
        event(e, EV_DOWNGRADE, e->pending);
    } else {
        e->highest = e->pending;
        e->current = e->pending;
        e->have_highest = true;
        e->valid = true;
        e->accepted = epoch_add(e->accepted, 1);
        event(e, EV_ACCEPT, e->pending);
    }
    e->phase = READY;
}

static void capture(Epoch *e, uint8_t falling)
{
    if (e->phase != CAPTURE)
        return;
    /* Raw generation changes also invalidate hold, even if debounce rejects. */
    if ((e->levels & 3u) != e->pending || (e->candidate & 3u) != e->pending) {
        e->bad_capture = true;
        fault(e, FAULT_HOLD);
    }
    if (e->capture_age > MAX_COMMIT_MS) {
        e->valid = false;
        e->invalid = epoch_add(e->invalid, 1);
        e->phase = UNKNOWN;
        e->baseline_age = 0;
        fault(e, FAULT_TIMEOUT);
        event(e, EV_INVALID, e->pending);
        return;
    }
    if (falling & INPUT_COMMIT)
        finish_capture(e);
}

static void observe_execution(Epoch *e)
{
    if (!(e->levels & INPUT_EXECUTE))
        return;
    /* Evaluated BEFORE a falling commit may authorize the new generation. */
    if (!e->valid || e->phase != READY || (e->levels & INPUT_COMMIT)
            || (e->levels & 3u) != e->current
            || (e->candidate & 3u) != e->current) {
        fault(e, FAULT_EXECUTE);
        e->execute_ms = epoch_add(e->execute_ms, 1);
    }
}

void epoch_tick(Epoch *e, uint32_t now, uint8_t raw)
{
    uint32_t dt = now - e->time;
    if (!dt)
        return;
    e->time = now;
    raw &= 15u;
    if (dt != 1u) {
        invalidate(e, dt, raw);
        return;
    }
    uint8_t previous = e->levels;
    bool was_qualified = e->qualified;
    if (e->phase == CAPTURE)
        e->capture_age = epoch_add(e->capture_age, 1);
    if (qualify(e, raw)) {
        if (!e->qualified && (e->levels & (INPUT_COMMIT | INPUT_EXECUTE)))
            fault(e, FAULT_STARTUP);
        e->qualified = true;
    }
    if (!e->qualified)
        return;
    uint8_t rising = was_qualified ? (uint8_t)(e->levels & ~previous) : 0;
    uint8_t falling = was_qualified ? (uint8_t)(previous & ~e->levels) : 0;
    /* A single raw data transition breaks setup stability. Unrelated execute
       chatter cannot delay a commit or generation qualification. */
    if ((e->levels & 3u) != (previous & 3u)
            || (e->candidate & 3u) != (e->levels & 3u))
        e->setup_age = 0;
    else if (!(e->levels & INPUT_COMMIT))
        e->setup_age = epoch_add(e->setup_age, 1);
    /* Changing generation away from a completed announcement invalidates it;
       changing back does not restore validity without another full commit. */
    if (e->valid && ((e->candidate & 3u) != e->current
                     || (e->levels & 3u) != e->current))
        e->valid = false;
    observe_execution(e);
    if (e->phase == READY && (rising & INPUT_COMMIT))
        begin_capture(e);
    else
        capture(e, falling);
    if (e->phase == UNKNOWN)
        baseline(e);
}
