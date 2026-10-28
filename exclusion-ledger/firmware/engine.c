/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>
/* A whole acquisition update either commits, or preserves prior totals. */
static void add(Ledger *s, uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount) {
        s->exhausted = true;
        return;
    }
    *value += amount;
}
static void break_segments(Ledger *s)
{
    for (unsigned i = 0; i < EL_PAIRS; ++i) {
        s->pair[i].active = 0;
        s->pair[i].age = 0;
    }
    s->valid = 0;
    memset(s->runs, 0, sizeof s->runs);
}
void ledger_init(Ledger *s)
{
    memset(s, 0, sizeof *s);
}
void ledger_halt(Ledger *s, uint32_t reason)
{
    if (s->halted)
        return;
    break_segments(s);
    s->faults |= reason;
    s->halted = 1;
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail != UINT32_MAX)
        ++s->tx_fail;
}
/* Each channel has an independent candidate and qualification run. Keeping
   last qualified levels while a candidate settles is intentional: the
   measured quantity is qualified overlap, not instantaneous raw overlap. */
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
        } else if (s->runs[i] < EL_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == EL_QUALIFY) {
            s->valid |= mask;
            if (high)
                s->levels |= mask;
            else
                s->levels &= ~(uint32_t)mask;
        }
    }
}
static void sample_pair(Ledger *s, unsigned index, unsigned mask)
{
    Pair *p = &s->pair[index];
    bool on = (s->valid & mask) == mask && (s->levels & mask) == mask;
    if (!on) {
        p->active = 0;
        p->age = 0;
        return;
    }
    s->faults |= 1u << index;
    if (!p->active) {
        add(s, &p->entries, 1);
        p->age = 0;
        p->active = 1;
        return;
    }
    /* Only intervals bracketed by two qualified overlapping observations.
       A closing edge does not contribute a speculative trailing interval. */
    add(s, &p->duration, 1);
    add(s, &p->age, 1);
    if (p->age > p->maximum)
        p->maximum = p->age;
}
static void sample_tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    bool continuous = s->started && delta == 1;
    s->now = now;
    if (s->started && !continuous) {
        add(s, &s->gaps, 1);
        add(s, &s->missed, delta - 1u);
        s->faults |= EL_GAP;
        break_segments(s);
        s->started = false;
    }
    qualify(s, raw);
    s->started = true;
    /* Coverage counts fully qualified vectors, not elapsed wall time.
       Individual pairs remain observable if an unrelated pin chatters. */
    if (s->valid == 15)
        add(s, &s->observed, 1);
    unsigned k = 0;
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned j = i + 1; j < 4; ++j) {
            unsigned mask = (1u << i) | (1u << j);
            sample_pair(s, k++, mask);
        }
    }
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
        /* No partially advanced pair or coverage ledger is published. */
        s->now = now;
        ledger_halt(s, EL_EXHAUSTED);
    } else {
        *s = next;
    }
}
static bool pair_valid(const Ledger *s, unsigned index, unsigned mask)
{
    const Pair *p = &s->pair[index];
    if (p->active > 1 || p->age > p->maximum || p->maximum > p->duration)
        return false;
    if (!p->entries && (p->duration || p->maximum || p->active))
        return false;
    if (!p->active && p->age)
        return false;
    if (p->entries == 1 && p->maximum != p->duration)
        return false;
    if (p->entries && (uint64_t)p->maximum * p->entries < p->duration)
        return false;
    bool on = !s->halted && (s->valid & mask) == mask && (s->levels & mask) == mask;
    if (!!p->active != on)
        return false;
    if (!!p->entries != !!(s->faults & (1u << index)))
        return false;
    return true;
}
bool ledger_valid(const Ledger *s)
{
    if (!s || s->levels > 15 || s->valid > 15 || s->faults > 511)
        return false;
    if (s->halted > 1 || (s->halted && s->valid))
        return false;
    if (!!s->halted != !!(s->faults & (EL_EXHAUSTED | EL_WATCHDOG)))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    if (!!s->gaps != !!(s->faults & EL_GAP))
        return false;
    unsigned k = 0;
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned j = i + 1; j < 4; ++j) {
            if (!pair_valid(s, k++, (1u << i) | (1u << j)))
                return false;
        }
    }
    return true;
}
