/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <string.h>
#include <limits.h>

/* A sample either commits its whole ledger update or none of it. */
static void add(Slots *s, uint32_t *counter, uint32_t amount)
{
    if (UINT32_MAX - *counter < amount) {
        s->exhausted = true;
        return;
    }
    *counter += amount;
}
static void finding(Slots *s, uint32_t reason, uint32_t *counter)
{
    s->faults |= reason;
    add(s, counter, 1);
}
static unsigned population(uint32_t mask)
{
    unsigned count = 0;
    for (unsigned bit = 0; bit < SA_CAPACITY; ++bit) {
        if (mask & (1u << bit))
            ++count;
    }
    return count;
}
static void abandon(Slots *s)
{
    /* Conservation bounds this addition for every committed valid state. */
    add(s, &s->abandoned, s->pending);
    s->pending = 0;
    s->occupied = 0;
    s->ready = 0;
    s->lost = true;
}
void slots_init(Slots *s)
{
    memset(s, 0, sizeof *s);
}
void slots_halt(Slots *s, uint32_t reason)
{
    if (s->halted)
        return;
    abandon(s);
    s->halted = 1;
    s->faults |= reason;
}
void slots_tx_failure(Slots *s)
{
    if (s->tx_fail < UINT32_MAX)
        ++s->tx_fail;
}
static void qualify(Slots *s, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        unsigned mask = 1u << i;
        bool high = (raw & mask) != 0;
        bool candidate = (s->candidates & mask) != 0;
        if (!s->started || high != candidate) {
            if (high)
                s->candidates |= mask;
            else
                s->candidates &= ~mask;
            s->runs[i] = 0;
        } else if (s->runs[i] < SA_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == SA_QUALIFY) {
            s->valid |= mask;
            if (high)
                s->levels |= mask;
            else
                s->levels &= ~mask;
        }
    }
}
static void gap(Slots *s, uint32_t delta)
{
    finding(s, SA_GAP, &s->gaps);
    add(s, &s->missed, delta - 1u);
    abandon(s);
    s->valid = 0;
    s->setup = 0;
    s->baseline = 0;
    s->started = false;
    memset(s->runs, 0, sizeof s->runs);
}
static void allocate(Slots *s, unsigned tag)
{
    unsigned bit = 1u << tag;
    if (s->occupied & bit) {
        /* Attempt counted already: abandon it and every prior live slot. */
        finding(s, SA_DUPLICATE, &s->duplicate);
        add(s, &s->abandoned, 1);
        abandon(s);
        return;
    }
    if (s->seen & bit)
        add(s, &s->reuse, 1);
    s->seen |= bit;
    s->occupied |= bit;
    ++s->pending;
    if (s->pending > s->peak)
        s->peak = s->pending;
}
static void release(Slots *s, unsigned tag)
{
    unsigned bit = 1u << tag;
    if (!(s->occupied & bit)) {
        finding(s, SA_ORPHAN, &s->orphan);
        abandon(s);
        return;
    }
    /* Membership, not order: any outstanding slot may retire first. */
    s->occupied &= ~bit;
    --s->pending;
    add(s, &s->retired, 1);
}
static void operations(Slots *s, uint8_t raw, uint32_t previous)
{
    uint32_t rising = s->levels & ~previous;
    bool alloc = (rising & 4u) != 0;
    bool free_slot = (rising & 8u) != 0;
    if (!alloc && !free_slot)
        return;
    if (alloc)
        add(s, &s->allocations, 1);
    if (free_slot)
        add(s, &s->releases, 1);
    /* A single shared tag bus cannot order overlapping operations. */
    if ((alloc && (s->levels & 8u)) || (free_slot && (s->levels & 4u))) {
        finding(s, SA_COLLISION, &s->collision);
        if (alloc)
            add(s, &s->abandoned, 1);
        abandon(s);
        return;
    }
    unsigned tag = s->levels & 3u;
    if (s->setup < SA_SETUP || (raw & 3u) != tag) {
        finding(s, SA_UNSTABLE, &s->unstable);
        if (alloc)
            add(s, &s->abandoned, 1);
        abandon(s);
        return;
    }
    if (alloc)
        allocate(s, tag);
    else
        release(s, tag);
}
static void sample_tick(Slots *s, uint32_t now, uint8_t raw)
{
    uint32_t previous = s->levels;
    bool acquired = s->valid == 15;
    uint32_t delta = now - s->now;
    s->now = now;
    if (s->started && delta != 1u)
        gap(s, delta);
    qualify(s, raw);
    s->started = true;
    if (s->valid == 15 && (raw & 3u) == (s->levels & 3u) &&
        (previous & 3u) == (s->levels & 3u)) {
        if (s->setup < SA_SETUP)
            ++s->setup;
    } else {
        s->setup = 0;
    }
    if (!s->ready) {
        /* Only operator-controlled startup may assume all resources free. */
        if (!s->lost && s->valid == 15 && !(raw & 12u) && !(s->levels & 12u)) {
            if (s->baseline < SA_BASELINE)
                ++s->baseline;
            if (s->baseline == SA_BASELINE)
                s->ready = 1;
        } else {
            s->baseline = 0;
        }
        return;
    }
    if (acquired && s->valid == 15)
        operations(s, raw, previous);
}
void slots_sample(Slots *s, uint32_t now, uint8_t raw)
{
    if (s->started && now == s->now)
        return;
    if (s->halted) {
        s->now = now;
        return;
    }
    Slots next = *s;
    sample_tick(&next, now, raw & 15u);
    if (next.exhausted) {
        s->now = now;
        slots_halt(s, SA_EXHAUSTED);
    } else {
        *s = next;
    }
}
static bool state_valid(const Slots *s)
{
    if (s->levels > 15 || s->valid > 15 || s->ready > 1 || s->halted > 1)
        return false;
    if (s->occupied > 15 || s->seen > 15)
        return false;
    if (s->pending != population(s->occupied))
        return false;
    if (s->pending > s->peak || s->peak > SA_CAPACITY)
        return false;
    if ((s->occupied & s->seen) != s->occupied)
        return false;
    if (s->faults > 255 || (s->faults & 4u))
        return false;
    if (s->ready && (s->valid != 15 || s->halted || s->lost || s->faults))
        return false;
    if (!s->ready && s->pending)
        return false;
    return true;
}
static bool ledger_valid(const Slots *s)
{
    if ((uint64_t)s->retired + s->abandoned + s->pending != s->allocations)
        return false;
    if ((uint64_t)s->retired + s->orphan > s->releases)
        return false;
    if (s->duplicate > s->abandoned || s->peak > s->allocations)
        return false;
    /* Every accepted reuse requires a preceding retirement of that slot. */
    if (s->reuse > s->retired)
        return false;
    uint64_t accepted = (uint64_t)s->retired + s->pending;
    if (s->reuse > accepted)
        return false;
    /* Abandonment may hide the last accepted occupancy, not arbitrary births. */
    if (accepted > (uint64_t)s->reuse + SA_CAPACITY)
        return false;
    if ((uint64_t)s->reuse + population(s->seen) > s->allocations)
        return false;
    if (s->peak > population(s->seen))
        return false;
    if ((uint64_t)s->collision + s->unstable + s->duplicate + s->orphan > 1)
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    return true;
}
static bool findings_valid(const Slots *s)
{
    if (!!s->halted != !!(s->faults & (SA_EXHAUSTED | SA_WATCHDOG)))
        return false;
    if (!!s->duplicate != !!(s->faults & SA_DUPLICATE))
        return false;
    if (!!s->orphan != !!(s->faults & SA_ORPHAN))
        return false;
    if (!!s->collision != !!(s->faults & SA_COLLISION))
        return false;
    if (!!s->unstable != !!(s->faults & SA_UNSTABLE))
        return false;
    if (!!s->gaps != !!(s->faults & SA_GAP))
        return false;
    return true;
}
bool slots_valid(const Slots *s)
{
    if (!s)
        return false;
    return state_valid(s) && ledger_valid(s) && findings_valid(s);
}
