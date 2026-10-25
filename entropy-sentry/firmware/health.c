/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "health.h"
#include <string.h>

uint32_t health_add(uint32_t a, uint32_t b)
{
    if (UINT32_MAX - a < b)
        return UINT32_MAX;
    return a + b;
}

static void event(Health *h, uint32_t kind, uint32_t value)
{
    /* Sequence advances even on overflow so dropped evidence is detectable. */
    uint32_t sequence = h->sequence++;
    if (h->count == EVENT_CAPACITY) {
        h->dropped = health_add(h->dropped, 1);
        return;
    }
    unsigned index = (h->head + h->count) % EVENT_CAPACITY;
    h->events[index].sequence = sequence;
    h->events[index].time = h->time;
    h->events[index].kind = kind;
    h->events[index].value = value;
    ++h->count;
}

bool health_pop(Health *h, Event *result)
{
    if (!h || !result || !h->count)
        return false;
    *result = h->events[h->head];
    h->head = (h->head + 1) % EVENT_CAPACITY;
    --h->count;
    return true;
}

void health_init(Health *h, uint32_t now)
{
    memset(h, 0, sizeof *h);
    h->time = now;
}

static void invalidate(Health *h)
{
    /* Unknown acquisition is not a passing statistical observation. */
    h->qualified = false;
    h->have_bit = false;
    h->have_edge = false;
    h->window_used = 0;
    h->matches = 0;
    h->run = 0;
    h->edge_age = 0;
    h->bit_age = 0;
}

static void fault(Health *h, uint8_t reason)
{
    if (!(h->faults & reason)) {
        h->failures = health_add(h->failures, 1);
        event(h, 1, reason);
    }
    h->faults |= reason;
    h->qualified = false;
}

static void repetition(Health *h, uint8_t bit)
{
    if (!h->have_bit || bit != h->last_bit) {
        h->run = 1;
    } else {
        h->run = health_add(h->run, 1);
    }
    h->have_bit = true;
    h->last_bit = bit;
    if (h->run > h->max_run)
        h->max_run = h->run;
    if (h->run >= REPEAT_LIMIT)
        fault(h, FAULT_REPEAT);
}

static void proportion(Health *h, uint8_t bit)
{
    if (!h->window_used) {
        h->reference_bit = bit;
        h->matches = 0;
    }
    if (h->reference_bit == bit)
        ++h->matches;
    ++h->window_used;
    /* Fail as soon as the cutoff is reached, not only at window closure. */
    if (h->matches >= MATCH_LIMIT)
        fault(h, FAULT_PROPORTION);
    if (h->window_used == WINDOW_SIZE) {
        h->last_matches = h->matches;
        h->windows = health_add(h->windows, 1);
        h->qualified = !h->faults;
        event(h, 2, h->last_matches);
        h->window_used = 0;
        h->matches = 0;
    }
}

static void rising_edge(Health *h, uint8_t levels)
{
    bool timing_ok = true;
    if (h->bit_age < SETUP_MS) {
        fault(h, FAULT_SETUP);
        timing_ok = false;
    }
    if (h->have_edge && h->edge_age < MIN_PERIOD_MS) {
        fault(h, FAULT_RATE);
        timing_ok = false;
    }
    h->have_edge = true;
    h->edge_age = 0;
    h->stalled = false;
    if (!timing_ok) {
        /* Do not add an ambiguously sampled bit to the statistical stream. */
        h->window_used = 0;
        h->matches = 0;
        h->have_bit = false;
        h->run = 0;
        return;
    }
    uint8_t bit = levels & 1u;
    h->samples = health_add(h->samples, 1);
    repetition(h, bit);
    proportion(h, bit);
}

static void policy(Health *h, uint8_t levels, bool previously_qualified)
{
    bool alarm = (levels & 4u) != 0;
    bool release = (levels & 8u) != 0;
    bool unsafe = release && (!previously_qualified || !h->qualified || alarm || h->faults);
    /* Export only ALARM and RELEASE, never BIT or CLOCK. */
    h->observed = (levels >> 2) & 3u;
    if (unsafe) {
        h->unsafe_ms = health_add(h->unsafe_ms, 1);
        if (!h->unsafe) {
            h->unsafe_episodes = health_add(h->unsafe_episodes, 1);
            event(h, 3, h->observed);
        }
    }
    h->unsafe = unsafe;
    if (h->faults && !alarm)
        h->unannounced_ms = health_add(h->unannounced_ms, 1);
}

void health_tick(Health *h, uint32_t now, uint8_t levels)
{
    uint32_t elapsed = now - h->time;
    if (!elapsed)
        return;
    h->time = now;
    levels &= 15u;
    bool previously_qualified = h->qualified;
    if (elapsed != 1u) {
        h->gaps = health_add(h->gaps, 1);
        h->missed = health_add(h->missed, elapsed - 1u);
        event(h, 4, elapsed - 1u);
        invalidate(h);
        h->initialized = false;
        previously_qualified = false;
    }
    if (!h->initialized) {
        /* A high clock at attachment is not evidence of a rising edge. */
        h->previous = levels;
        h->initialized = true;
        h->bit_age = 0;
        policy(h, levels, false);
        return;
    }
    h->edge_age = health_add(h->edge_age, 1);
    if ((levels ^ h->previous) & 1u)
        h->bit_age = 0;
    else
        h->bit_age = health_add(h->bit_age, 1);
    if (h->have_edge && h->edge_age > MAX_PERIOD_MS) {
        /* Clock continuity failed, but BIT stability was continuously observed. */
        uint32_t bit_age = h->bit_age;
        invalidate(h);
        h->bit_age = bit_age;
        h->stalled = true;
        h->stalls = health_add(h->stalls, 1);
        previously_qualified = false;
        event(h, 5, h->stalls);
    }
    if (!(h->previous & 2u) && (levels & 2u))
        rising_edge(h, levels);
    policy(h, levels, previously_qualified);
    h->previous = levels;
}
