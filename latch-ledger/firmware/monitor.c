/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "monitor.h"
#include <limits.h>
#include <string.h>

/* Saturation prevents counter wrap from resembling a new engagement. */
static uint32_t add_saturated(uint32_t a, uint32_t b)
{
    if (UINT32_MAX - a < b)
        return UINT32_MAX;
    return a + b;
}

static uint32_t maximum(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}

static void emit(monitor_t *m, uint32_t now, event_kind_t kind,
                 uint8_t channel, uint8_t value, uint32_t duration)
{
    m->sequence++;
    if (m->count == EVENT_CAPACITY) {
        m->dropped = add_saturated(m->dropped, 1);
        return;
    }
    uint8_t index = (uint8_t)((m->head + m->count) % EVENT_CAPACITY);
    event_t *e = &m->events[index];
    e->sequence = m->sequence;
    e->time_ms = now;
    e->kind = (uint8_t)kind;
    e->channel = channel;
    e->value = value;
    e->duration_ms = duration;
    m->count++;
}

void monitor_init(monitor_t *m, uint32_t now, uint8_t levels)
{
    memset(m, 0, sizeof *m);
    m->last_sample = now;
    for (uint8_t i = 0; i < CHANNELS; ++i) {
        channel_t *c = &m->channel[i];
        c->raw = (levels >> i) & 1u;
        c->stable = c->raw;
        c->raw_since = now;
        c->stable_since = now;
        c->shortest_raw_ms = UINT32_MAX;
    }
    for (uint8_t i = 0; i < PAIRS; ++i) {
        pair_t *p = &m->pair[i];
        p->active = m->channel[2 * i].stable != m->channel[2 * i + 1].stable;
        p->since = now;
    }
}

static void sample_channel(monitor_t *m, uint8_t index,
                           uint32_t now, uint8_t level)
{
    channel_t *c = &m->channel[index];
    if (level != c->raw) {
        uint32_t width = now - c->raw_since;
        /* Boot-to-first-edge is truncated, not a measured complete pulse. */
        if (c->seen_edge) {
            if (width < c->shortest_raw_ms)
                c->shortest_raw_ms = width;
            if (width < DEBOUNCE_MS)
                c->short_pulses = add_saturated(c->short_pulses, 1);
        }
        c->seen_edge = true;
        c->raw_edges = add_saturated(c->raw_edges, 1);
        c->raw = level;
        c->raw_since = now;
        emit(m, now, EVENT_RAW, index, level, width);
    }
    if (c->raw != c->stable && now - c->raw_since >= DEBOUNCE_MS) {
        uint32_t dwell = now - c->stable_since;
        if (c->stable)
            c->longest_open_ms = maximum(c->longest_open_ms, dwell);
        c->stable = c->raw;
        c->stable_since = now;
        c->changes = add_saturated(c->changes, 1);
        emit(m, now, EVENT_STABLE, index, c->stable, dwell);
    }
}

static void sample_pair(monitor_t *m, uint8_t index, uint32_t now)
{
    pair_t *p = &m->pair[index];
    uint8_t different = m->channel[2 * index].stable !=
                        m->channel[2 * index + 1].stable;
    if (different == p->active)
        return;
    uint32_t duration = now - p->since;
    if (different)
        p->mismatches = add_saturated(p->mismatches, 1);
    else
        p->longest_ms = maximum(p->longest_ms, duration);
    p->active = different;
    p->since = now;
    emit(m, now, EVENT_PAIR, index, different, duration);
}

void monitor_sample(monitor_t *m, uint32_t now, uint8_t levels)
{
    uint32_t elapsed = now - m->last_sample;
    if (!elapsed)
        return;
    if (elapsed > 1) {
        m->missed_samples = add_saturated(m->missed_samples, elapsed - 1);
        /* Never interpret an unobserved interval as continuous stability. */
        for (uint8_t i = 0; i < CHANNELS; ++i) {
            m->channel[i].raw_since = now;
            m->channel[i].stable_since = now;
            m->channel[i].seen_edge = false;
        }
        for (uint8_t i = 0; i < PAIRS; ++i)
            m->pair[i].since = now;
    }
    m->last_sample = now;
    for (uint8_t i = 0; i < CHANNELS; ++i)
        sample_channel(m, i, now, (levels >> i) & 1u);
    for (uint8_t i = 0; i < PAIRS; ++i)
        sample_pair(m, i, now);
}

bool monitor_pop(monitor_t *m, event_t *event)
{
    if (!m->count)
        return false;
    *event = m->events[m->head];
    m->head = (uint8_t)((m->head + 1) % EVENT_CAPACITY);
    m->count--;
    return true;
}

void monitor_tx_failure(monitor_t *m)
{
    m->tx_failures = add_saturated(m->tx_failures, 1);
}

uint32_t monitor_open_ms(const channel_t *c, uint32_t now)
{
    if (!c->stable)
        return c->longest_open_ms;
    return maximum(c->longest_open_ms, now - c->stable_since);
}

uint32_t monitor_pair_ms(const pair_t *p, uint32_t now)
{
    if (!p->active)
        return p->longest_ms;
    return maximum(p->longest_ms, now - p->since);
}
