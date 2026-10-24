/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "quorum.h"
#include <string.h>
#include <stddef.h>

/* Duration and evidence counters saturate. Only timestamps/sequences wrap. */
static uint32_t add(uint32_t a, uint32_t b)
{
    return UINT32_MAX - a < b ? UINT32_MAX : a + b;
}

static void emit(struct quorum *q, enum qw_kind kind, uint32_t value)
{
    struct qw_event event = {q->sequence++, q->time, kind, value};
    if (q->count == QW_QUEUE) {
        q->dropped = add(q->dropped, 1);
        return;
    }
    unsigned index = (q->head + q->count) % QW_QUEUE;
    q->queue[index] = event;
    ++q->count;
}

bool qw_pop(struct quorum *q, struct qw_event *event)
{
    if (!q || !event || !q->count)
        return false;
    *event = q->queue[q->head];
    q->head = (q->head + 1u) % QW_QUEUE;
    --q->count;
    return true;
}

void qw_init(struct quorum *q, uint32_t now)
{
    memset(q, 0, sizeof *q);
    q->time = now;
}

void qw_tx_fail(struct quorum *q)
{
    q->tx_fail = add(q->tx_fail, 1);
}

/* A gap destroys interval continuity, not previously collected evidence. */
static void invalidate(struct quorum *q)
{
    q->valid = false;
    q->consecutive = 0;
    q->mismatch_ms = 0;
    q->mismatch = 0;
    q->alarms = 0;
    for (unsigned i = 0; i < 3; ++i)
        q->dissent_ms[i] = 0;
}

static void advance_mismatch(struct quorum *q)
{
    if (!q->mismatch)
        return;
    q->mismatch_ms = add(q->mismatch_ms, 1);
    if (q->mismatch_ms > q->max_mismatch)
        q->max_mismatch = q->mismatch_ms;
    uint8_t bit = q->mismatch == 1 ? 1u : 2u;
    if (q->mismatch_ms < QW_SETTLE_MS || (q->alarms & bit))
        return;
    q->alarms |= bit;
    if (q->mismatch == 1) {
        q->unsafe = add(q->unsafe, 1);
        emit(q, QW_UNSAFE, q->unsafe);
    } else {
        q->denied = add(q->denied, 1);
        emit(q, QW_DENIED, q->denied);
    }
}

static void advance_dissent(struct quorum *q)
{
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t input_bit = (uint8_t)(1u << i);
        uint8_t alarm_bit = (uint8_t)(4u << i);
        if (!(q->dissent & input_bit))
            continue;
        q->dissent_ms[i] = add(q->dissent_ms[i], 1);
        if (q->dissent_ms[i] > q->max_dissent[i])
            q->max_dissent[i] = q->dissent_ms[i];
        if (q->dissent_ms[i] < QW_DISSENT_MS || (q->alarms & alarm_bit))
            continue;
        q->alarms |= alarm_bit;
        q->dissent_count[i] = add(q->dissent_count[i], 1);
        emit(q, (enum qw_kind)(QW_DISSENT_A + i), q->dissent_count[i]);
    }
}

static void accept(struct quorum *q, uint8_t levels)
{
    bool first = !q->valid;
    uint8_t votes = levels & 7u;
    unsigned ones = (votes & 1u) + ((votes >> 1) & 1u) + ((votes >> 2) & 1u);
    uint8_t majority = ones >= 2u;
    uint8_t dissent = (uint8_t)((majority ? ~votes : votes) & 7u);
    uint8_t output = (levels >> 3) & 1u;
    uint8_t mismatch = output == majority ? 0u : (output ? 1u : 2u);

    /* Preserve an output mismatch across vote changes if its direction holds.
     * Track each minority identity separately; rotating dissent is not one
     * continuously faulty voter. These semantics distinguish this instrument
     * from a two-line comparator or a generic edge counter. */
    if (first || mismatch != q->mismatch) {
        q->mismatch_ms = 0;
        q->alarms &= (uint8_t)~3u;
    }
    for (unsigned i = 0; i < 3; ++i) {
        if (first || ((dissent ^ q->dissent) & (1u << i))) {
            q->dissent_ms[i] = 0;
            q->alarms &= (uint8_t)~(4u << i);
        }
    }
    if (first || levels != q->levels)
        emit(q, QW_LEVELS, levels);
    q->levels = levels;
    q->majority = majority;
    q->dissent = dissent;
    q->mismatch = mismatch;
    q->valid = true;
    if (first)
        emit(q, QW_READY, 1);
}

void qw_sample(struct quorum *q, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - q->time;
    if (!delta)
        return;
    q->time = now;
    raw &= 15u;
    if (delta != 1u) {
        q->gaps = add(q->gaps, 1);
        q->missed = add(q->missed, delta - 1u);
        invalidate(q);
        emit(q, QW_GAP, delta - 1u);
        /* The first post-gap read is intentionally not a qualifying sample. */
        return;
    }
    if (q->valid) {
        /* Finish the old interval before accepting the next vector. This
         * makes a recovery exactly on the threshold count the old episode. */
        advance_mismatch(q);
        advance_dissent(q);
    }
    if (!q->consecutive || raw != q->candidate) {
        q->candidate = raw;
        q->consecutive = 1;
    } else if (q->consecutive < QW_QUALIFY) {
        ++q->consecutive;
    }
    if (q->consecutive == QW_QUALIFY && (!q->valid || q->levels != raw))
        accept(q, raw);
}
