/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "receipt.h"
#include <string.h>

uint32_t receipt_add(uint32_t a, uint32_t b)
{
    return b > UINT32_MAX - a ? UINT32_MAX : a + b;
}

void receipt_init(Receipt *r, uint32_t now)
{
    memset(r, 0, sizeof *r);
    r->time = now;
}

static Pending *entry(Receipt *r, unsigned offset)
{
    return &r->pending[(r->head + offset) % CAPACITY];
}

uint32_t receipt_waiting(const Receipt *r)
{
    uint32_t result = 0;
    for (unsigned i = 0; i < r->count; ++i) {
        const Pending *p = &r->pending[(r->head + i) % CAPACITY];
        if (!p->appended)
            ++result;
    }
    return result;
}

static void invalidate(Receipt *r, uint32_t fault)
{
    /* Ambiguous correlation never invents a receipt. Preserve cumulative facts. */
    r->faults |= fault;
    r->abandoned = receipt_add(r->abandoned, r->count);
    r->head = 0;
    r->count = 0;
    r->ready = false;
    r->baseline = 0;
}

static void qualify(Receipt *r, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t mask = (uint8_t)(1u << i);
        if ((raw ^ r->candidate) & mask) {
            r->candidate ^= mask;
            r->stable[i] = 0;
        } else if (r->stable[i] < QUALIFY_MS) {
            ++r->stable[i];
        }
        if (r->stable[i] == QUALIFY_MS) {
            r->known |= mask;
            r->levels = (r->levels & (uint8_t)~mask) | (raw & mask);
        }
    }
}

static bool age_pending(Receipt *r)
{
    uint32_t fault = 0;
    for (unsigned i = 0; i < r->count; ++i) {
        Pending *p = entry(r, i);
        p->age = receipt_add(p->age, 1);
        if (p->appended) {
            p->append_age = receipt_add(p->append_age, 1);
            if (p->append_age > DURABLE_MS)
                fault |= F_DURABLE_LATE;
        } else if (p->age > APPEND_MS) {
            fault |= F_APPEND_LATE;
        }
    }
    if (fault) {
        invalidate(r, fault);
        return false;
    }
    return true;
}

static bool take_receipt(Receipt *r)
{
    /* Process before this tick's append/action: ordering must be evidenced. */
    if (!r->count || !entry(r, 0)->appended) {
        invalidate(r, F_ORPHAN_DURABLE);
        return false;
    }
    Pending *p = entry(r, 0);
    if (p->append_age > r->max_durable)
        r->max_durable = p->append_age;
    r->head = (uint8_t)((r->head + 1u) % CAPACITY);
    --r->count;
    r->receipts = receipt_add(r->receipts, 1);
    return true;
}

static bool take_action(Receipt *r)
{
    if (r->count == CAPACITY) {
        /* Include the unqueueable action in the loss ledger. */
        r->abandoned = receipt_add(r->abandoned, 1);
        invalidate(r, F_OVERFLOW);
        return false;
    }
    Pending *p = entry(r, r->count);
    p->age = 0;
    p->append_age = 0;
    p->appended = false;
    ++r->count;
    return true;
}

static bool take_append(Receipt *r)
{
    for (unsigned i = 0; i < r->count; ++i) {
        Pending *p = entry(r, i);
        if (p->appended)
            continue;
        p->appended = true;
        p->append_age = 0;
        if (p->age > r->max_append)
            r->max_append = p->age;
        r->appended = receipt_add(r->appended, 1);
        return true;
    }
    invalidate(r, F_ORPHAN_APPEND);
    return false;
}

void receipt_tick(Receipt *r, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - r->time;
    r->time = now;
    raw &= 15u;
    if (delta != 1u) {
        r->gaps = receipt_add(r->gaps, 1);
        r->missed = receipt_add(r->missed, delta ? delta - 1u : 0u);
        invalidate(r, F_GAP);
        r->known = 0;
        r->candidate = raw;
        memset(r->stable, 0, sizeof r->stable);
        return;
    }
    uint8_t before = r->levels;
    qualify(r, raw);
    if (!r->ready) {
        /* All four wires must be low, including BLOCKED, to start a segment. */
        if (r->known == 15u && !raw && !r->levels) {
            r->baseline = receipt_add(r->baseline, 1);
            if (r->baseline >= BASELINE_MS) {
                r->ready = true;
                r->segments = receipt_add(r->segments, 1);
            }
        } else {
            r->baseline = 0;
            if (raw & (ACTION | APPEND | DURABLE))
                r->faults |= F_STARTUP;
        }
        return;
    }
    uint8_t rising = (uint8_t)(r->levels & (uint8_t)~before);
    /* Count evidence before any fault can invalidate correlation this tick. */
    if (rising & ACTION) {
        r->actions = receipt_add(r->actions, 1);
        if ((raw | r->levels) & BLOCKED)
            r->faults |= F_BLOCKED_ACTION;
    }
    if (!age_pending(r) || ((rising & DURABLE) && !take_receipt(r))) {
        if (rising & ACTION)
            r->abandoned = receipt_add(r->abandoned, 1);
        return;
    }
    if ((rising & ACTION) && !take_action(r))
        return;
    if (rising & APPEND)
        (void)take_append(r);
}
