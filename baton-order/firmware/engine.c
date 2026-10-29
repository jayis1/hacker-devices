/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>
/* Every sample commits atomically; overflow cannot split matrix/aggregate. */
static void add(Ledger *s, uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount) {
        s->exhausted = true;
        return;
    }
    *value += amount;
}
static void lose_phase(Ledger *s)
{
    s->last = BO_NONE;
    s->ready = 0;
    s->idle = 0;
}
static void break_acquisition(Ledger *s)
{
    lose_phase(s);
    s->valid = 0;
    s->collision_active = 0;
    memset(s->runs, 0, sizeof s->runs);
}
void ledger_init(Ledger *s)
{
    memset(s, 0, sizeof *s);
    s->last = BO_NONE;
}
void ledger_halt(Ledger *s, uint32_t reason)
{
    if (s->halted)
        return;
    break_acquisition(s);
    s->faults |= reason;
    s->halted = 1;
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail != UINT32_MAX)
        ++s->tx_fail;
}
/* Qualification is independent, so an unrelated input cannot hide two
   known high owners. A whole-vector requirement is used only for order. */
static void qualify(Ledger *s, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t mask = (uint8_t)(1u << i);
        bool high = (raw & mask) != 0;
        bool candidate = (s->candidates & mask) != 0;
        if (!s->started || high != candidate) {
            if (high)
                s->candidates |= mask;
            else
                s->candidates &= (uint8_t)~mask;
            s->runs[i] = 0;
        } else if (s->runs[i] < BO_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == BO_QUALIFY) {
            s->valid |= mask;
            if (high)
                s->levels |= mask;
            else
                s->levels &= ~(uint32_t)mask;
        }
    }
}
static unsigned population(unsigned mask)
{
    unsigned count = 0;
    for (unsigned i = 0; i < 4; ++i)
        count += (mask >> i) & 1u;
    return count;
}
static unsigned owner(unsigned mask)
{
    for (unsigned i = 0; i < 4; ++i) {
        if (mask & (1u << i))
            return i;
    }
    return BO_NONE;
}
static void admission(Ledger *s, unsigned next)
{
    if (s->last == BO_NONE) {
        /* First singleton after a witnessed zero baseline is an anchor,
           not evidence of an earlier transfer or a completed rotation. */
        add(s, &s->anchors, 1);
    } else {
        add(s, &s->matrix[4u * s->last + next], 1);
        unsigned distance = (next + 4u - s->last) % 4u;
        switch (distance) {
        case 0:
            add(s, &s->repeat, 1);
            s->faults |= BO_REPEAT;
            break;
        case 1:
            add(s, &s->forward, 1);
            break;
        case 2:
            add(s, &s->skip, 1);
            s->faults |= BO_SKIP;
            break;
        case 3:
            add(s, &s->reverse, 1);
            s->faults |= BO_REVERSE;
            break;
        }
    }
    s->last = next;
    s->idle = 0;
}
static void observe(Ledger *s)
{
    unsigned highs = population(s->levels & s->valid);
    if (highs > 1) {
        if (!s->collision_active)
            add(s, &s->collisions, 1);
        s->collision_active = 1;
        s->faults |= BO_COLLISION;
        lose_phase(s);
        return;
    }
    /* Episode closure uses the same independent predicate as entry. */
    s->collision_active = 0;
    if (s->valid != 15)
        return;
    if (!s->levels) {
        /* Zero establishes an observable baseline, not a known next owner.
           Once anchored, any duration of idle preserves the expected turn. */
        s->ready = 1;
        s->idle = 1;
        return;
    }
    if (!s->ready)
        return;
    unsigned next = owner(s->levels);
    if (s->last == next && !s->idle)
        return;
    admission(s, next);
}
static void sample_tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    bool continuous = s->started && delta == 1;
    s->now = now;
    if (s->started && !continuous) {
        add(s, &s->gaps, 1);
        add(s, &s->missed, delta - 1u);
        s->faults |= BO_GAP;
        break_acquisition(s);
        s->started = false;
    }
    qualify(s, raw);
    s->started = true;
    if (s->valid == 15)
        add(s, &s->observed, 1);
    observe(s);
}
void ledger_sample(Ledger *s, uint32_t now, uint8_t raw)
{
    if (s->started && now == s->now)
        return;
    if (s->halted) {
        s->now = now;
        return;
    }
    Ledger next = *s;
    sample_tick(&next, now, raw & 15u);
    if (next.exhausted) {
        s->now = now;
        ledger_halt(s, BO_EXHAUSTED);
    } else {
        *s = next;
    }
}
/* Serializer and host tests use necessary cross-field bounds. These do not
   establish authenticity or reconstruct all possible sample histories. */
bool ledger_valid(const Ledger *s)
{
    if (!s || s->levels > 15 || s->valid > 15 || s->faults > 127)
        return false;
    if (s->halted > 1 || s->ready > 1 || s->idle > 1 || s->collision_active > 1)
        return false;
    if (s->last > BO_NONE)
        return false;
    if (!!s->halted != !!(s->faults & (BO_EXHAUSTED | BO_WATCHDOG)))
        return false;
    if (s->halted && (s->valid || s->ready || s->idle || s->collision_active))
        return false;
    if (!s->ready && (s->last != BO_NONE || s->idle))
        return false;
    if (s->ready && (s->valid != 15 || population(s->levels) > 1))
        return false;
    if (s->ready && (!!s->idle != !s->levels))
        return false;
    if (s->ready && !s->idle && owner(s->levels) != s->last)
        return false;
    if (s->last != BO_NONE && !s->anchors)
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    if (!!s->gaps != !!(s->faults & BO_GAP))
        return false;
    if (!!s->repeat != !!(s->faults & BO_REPEAT))
        return false;
    if (!!s->skip != !!(s->faults & BO_SKIP))
        return false;
    if (!!s->reverse != !!(s->faults & BO_REVERSE))
        return false;
    if (!!s->collisions != !!(s->faults & BO_COLLISION))
        return false;
    bool colliding = !s->halted && population(s->levels & s->valid) > 1;
    if (!!s->collision_active != colliding)
        return false;
    uint64_t sums[4] = {0};
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned j = 0; j < 4; ++j)
            sums[(j + 4u - i) % 4u] += s->matrix[4u * i + j];
    }
    if (sums[0] != s->repeat || sums[1] != s->forward ||
        sums[2] != s->skip || sums[3] != s->reverse)
        return false;
    if (!s->anchors && (sums[0] || sums[1] || sums[2] || sums[3]))
        return false;
    return true;
}
