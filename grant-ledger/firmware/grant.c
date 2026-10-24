/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "grant.h"
#include <string.h>

uint32_t gl_sat_add(uint32_t a, uint32_t b)
{
    if (UINT32_MAX - a < b)
        return UINT32_MAX;
    return a + b;
}

static void increment(uint32_t *value)
{
    *value = gl_sat_add(*value, 1);
}

static void event(struct grant *g, uint32_t kind, uint32_t value)
{
    /* Sequence advances even if storage is exhausted: loss is observable. */
    uint32_t sequence = g->sequence++;
    if (g->used == GL_QUEUE) {
        increment(&g->dropped);
        return;
    }
    unsigned tail = (g->head + g->used) % GL_QUEUE;
    g->queue[tail].sequence = sequence;
    g->queue[tail].time = g->time;
    g->queue[tail].kind = kind;
    g->queue[tail].value = value;
    ++g->used;
}

bool grant_pop(struct grant *g, struct gl_event *out)
{
    if (!g || !out || !g->used)
        return false;
    *out = g->queue[g->head];
    g->head = (g->head + 1u) % GL_QUEUE;
    --g->used;
    return true;
}

static void state(struct grant *g, enum gl_state next)
{
    if (g->state == next)
        return;
    g->state = (uint8_t)next;
    g->age = 0;
    event(g, GL_STATE, next);
}

static void fault(struct grant *g, enum gl_fault reason)
{
    increment(&g->faults[reason]);
    event(g, GL_FAULT, reason);
}

void grant_init(struct grant *g, uint32_t time)
{
    memset(g, 0, sizeof *g);
    g->time = time;
    /* Three observed all-low samples are required before any credit exists. */
    g->synced = false;
}

void grant_tx_fail(struct grant *g)
{
    increment(&g->tx_fail);
}

static unsigned edge_count(uint8_t edges)
{
    unsigned count = 0;
    for (unsigned bit = 0; bit < 3; ++bit) {
        if (edges & (1u << bit))
            ++count;
    }
    return count;
}

static void transaction(struct grant *g, uint8_t rising)
{
    /* Revocation dominates all simultaneous activity, including consumption. */
    if (g->levels & GL_REVOKE) {
        if (rising & GL_REVOKE)
            increment(&g->revoked);
        if (rising & GL_USE)
            fault(g, GL_REVOKED_USE);
        state(g, GL_IDLE);
        return;
    }
    /* A sampled tie cannot establish causal ordering. Never award credit. */
    if (edge_count(rising) > 1u) {
        fault(g, GL_AMBIGUOUS);
        state(g, GL_IDLE);
        return;
    }
    if (rising & GL_REQUEST) {
        increment(&g->requests);
        if (g->state != GL_IDLE) {
            fault(g, GL_OVERLAP);
            state(g, GL_IDLE);
        } else {
            state(g, GL_PENDING);
        }
    }
    if (rising & GL_GRANT) {
        increment(&g->grants);
        if (g->state != GL_PENDING) {
            fault(g, GL_ORPHAN);
            state(g, GL_IDLE);
        } else {
            state(g, GL_CREDIT);
        }
    }
    if (rising & GL_USE) {
        if (g->state == GL_CREDIT) {
            increment(&g->accepted);
            event(g, GL_ACCEPT, g->accepted);
        } else {
            fault(g, GL_NO_CREDIT);
        }
        /* Consume once, regardless of later deassertion of the use line. */
        state(g, GL_IDLE);
    }
}

static void gap(struct grant *g, uint32_t elapsed, uint8_t raw)
{
    increment(&g->gaps);
    g->missed = gl_sat_add(g->missed, elapsed - 1u);
    g->synced = false;
    g->stable = 1;
    g->candidate = raw;
    state(g, GL_IDLE);
    event(g, GL_GAP, elapsed - 1u);
    /* Preserve the last qualified levels until a new vector is qualified. */
}

static bool qualify(struct grant *g, uint8_t raw)
{
    if (g->candidate != raw) {
        g->candidate = raw;
        g->stable = 1;
        return false;
    }
    if (g->stable < GL_QUALIFY)
        ++g->stable;
    return g->stable == GL_QUALIFY;
}

void grant_sample(struct grant *g, uint32_t time, uint8_t raw)
{
    uint32_t elapsed = time - g->time;
    if (!elapsed)
        return;
    g->time = time;
    raw &= 15u;
    if (elapsed != 1u) {
        gap(g, elapsed, raw);
        return;
    }
    /* Age is accumulated and saturated, not subtracted from an old epoch. */
    if (g->state != GL_IDLE) {
        g->age = gl_sat_add(g->age, elapsed);
        if (g->age >= GL_WINDOW_MS) {
            fault(g, GL_EXPIRED);
            state(g, GL_IDLE);
        }
    }
    if (!qualify(g, raw))
        return;
    uint8_t old = g->levels;
    g->levels = raw;
    if (old != raw)
        event(g, GL_LEVEL, raw);
    if (!g->synced) {
        if (raw == 0) {
            g->synced = true;
            event(g, GL_SYNC, 1);
        }
        return;
    }
    transaction(g, (uint8_t)(raw & (uint8_t)~old));
}
