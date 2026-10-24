/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "audit.h"
#include <limits.h>
#include <string.h>

/* Positive sequence: 00, 01, 11, 10, 00. Diagonals are NOT motion. */
static const int8_t movement[16] = {
     0,  1, -1,  0,
    -1,  0,  0,  1,
     1,  0,  0, -1,
     0, -1,  1,  0
};

static void increment(uint32_t *value)
{
    if (*value != UINT32_MAX)
        ++*value;
}

static void accumulate(uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount)
        *value = UINT32_MAX;
    else
        *value += amount;
}

static void event(audit_t *a, unsigned index, event_kind_t kind,
                  uint8_t before, uint8_t after, uint32_t now)
{
    /* Sequence wraps intentionally; event loss never silently overwrites data. */
    uint32_t sequence = a->sequence++;
    if (a->count == EVENT_CAPACITY) {
        increment(&a->dropped);
        return;
    }
    unsigned tail = (a->head + a->count) % EVENT_CAPACITY;
    audit_event_t *e = &a->events[tail];
    e->time = now;
    e->sequence = sequence;
    e->axis = (uint8_t)index;
    e->kind = kind;
    e->before = before;
    e->after = after;
    ++a->count;
}

void audit_init(audit_t *a, uint32_t now, uint8_t levels)
{
    memset(a, 0, sizeof *a);
    a->last_sample = now;
    for (unsigned i = 0; i < AXES; ++i) {
        a->axis[i].state = (levels >> (2 * i)) & 3u;
        a->axis[i].min_interval = UINT32_MAX;
    }
}

static unsigned interval_bin(uint32_t duration)
{
    /* 0..1, 2..3, 4..7, ..., 64..127, 128+ milliseconds. */
    unsigned bin = 0;
    while (duration > 1 && bin < HISTOGRAM_BINS - 1) {
        duration >>= 1;
        ++bin;
    }
    return bin;
}

static void edge_timing(axis_t *s, uint32_t now)
{
    if (s->have_edge) {
        uint32_t interval = now - s->last_edge;
        if (interval < s->min_interval)
            s->min_interval = interval;
        if (interval > s->max_interval)
            s->max_interval = interval;
        increment(&s->histogram[interval_bin(interval)]);
    }
    s->last_edge = now;
    s->have_edge = true;
}

static void position(audit_t *a, unsigned index, int8_t delta, uint32_t now)
{
    axis_t *s = &a->axis[index];
    bool overflow = (delta > 0 && s->position == INT32_MAX) ||
                    (delta < 0 && s->position == INT32_MIN);
    if (overflow) {
        if (!s->saturated)
            event(a, index, EVENT_SATURATION, s->state, s->state, now);
        s->saturated = true;
        return;
    }
    s->position += delta;
}

static void cycle(axis_t *s, int8_t delta)
{
    /* A cycle is four contiguous legal transitions in the same direction.
     * An illegal transition, observation gap, or reversal breaks the run. */
    if ((s->partial_cycle > 0 && delta < 0) ||
        (s->partial_cycle < 0 && delta > 0))
        s->partial_cycle = 0;
    s->partial_cycle += delta;
    if (s->partial_cycle == 4) {
        increment(&s->cycles_forward);
        s->partial_cycle = 0;
    } else if (s->partial_cycle == -4) {
        increment(&s->cycles_reverse);
        s->partial_cycle = 0;
    }
}

static void transition(audit_t *a, unsigned index, uint8_t next, uint32_t now)
{
    axis_t *s = &a->axis[index];
    uint8_t before = s->state;
    uint8_t changed = before ^ next;
    if (!changed)
        return;
    increment(&s->edges);
    if (changed & 1u)
        increment(&s->phase_a);
    if (changed & 2u)
        increment(&s->phase_b);
    edge_timing(s, now);
    s->state = next;
    if (changed == 3u) {
        increment(&s->illegal);
        s->direction = 0;
        s->partial_cycle = 0;
        event(a, index, EVENT_ILLEGAL, before, next, now);
        return;
    }
    int8_t delta = movement[before * 4u + next];
    if (delta > 0)
        increment(&s->forward);
    else
        increment(&s->reverse);
    if (s->direction && s->direction != delta) {
        increment(&s->reversals);
        event(a, index, EVENT_REVERSAL, before, next, now);
    }
    s->direction = delta;
    position(a, index, delta, now);
    cycle(s, delta);
}

static void resynchronize(audit_t *a, uint8_t levels, uint32_t now)
{
    for (unsigned i = 0; i < AXES; ++i) {
        axis_t *s = &a->axis[i];
        uint8_t next = (levels >> (2 * i)) & 3u;
        event(a, i, EVENT_GAP, s->state, next, now);
        s->state = next;
        s->direction = 0;
        s->partial_cycle = 0;
        s->have_edge = false;
    }
}

void audit_sample(audit_t *a, uint32_t now, uint8_t levels)
{
    uint32_t elapsed = now - a->last_sample;
    if (!elapsed)
        return;
    a->last_sample = now;
    increment(&a->samples);
    if (elapsed != 1u) {
        increment(&a->gaps);
        accumulate(&a->missed, elapsed - 1u);
        resynchronize(a, levels, now);
        return;
    }
    for (unsigned i = 0; i < AXES; ++i) {
        axis_t *s = &a->axis[i];
        /* Dwell is observed intervals, never extrapolated over missing time. */
        increment(&s->dwell[s->state]);
        transition(a, i, (levels >> (2 * i)) & 3u, now);
    }
}

bool audit_pop(audit_t *a, audit_event_t *out)
{
    if (!out || !a->count)
        return false;
    *out = a->events[a->head];
    a->head = (a->head + 1u) % EVENT_CAPACITY;
    --a->count;
    return true;
}

void audit_tx_failure(audit_t *a)
{
    increment(&a->tx_failed);
}

uint32_t audit_age(const axis_t *s, uint32_t now)
{
    return s->have_edge ? now - s->last_edge : UINT32_MAX;
}
