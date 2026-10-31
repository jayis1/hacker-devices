/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>
/* Apply each observation transactionally, including all diagnostics. */
static void increment(Ledger *s, uint32_t *value)
{
    if (*value == UINT32_MAX)
        s->exhausted = true;
    else
        ++*value;
}
static unsigned population(uint32_t mask)
{
    return (mask & 1u) + ((mask >> 1) & 1u);
}
void ledger_init(Ledger *s)
{
    /* No inherited cleanliness, including on the first allocation. */
    memset(s, 0, sizeof *s);
}
void ledger_halt(Ledger *s, uint32_t reason)
{
    if (s->halted)
        return;
    s->halted = 1;
    s->faults |= reason;
    s->valid = 0;
    memset(s->runs, 0, sizeof s->runs);
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail != UINT32_MAX)
        ++s->tx_fail;
}
static void qualify(Ledger *s, uint8_t raw)
{
    /* SLOT/SCRUB are bundled data; each operation strobe is independent. */
    static const uint8_t masks[3] = {3u, 4u, 8u};
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t mask = masks[i];
        if (!s->started || ((s->candidates ^ raw) & mask)) {
            s->runs[i] = 0;
            s->candidates = (s->candidates & (uint8_t)~mask) | (raw & mask);
        } else if (s->runs[i] < OB_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == OB_QUALIFY) {
            s->levels = (s->levels & ~(uint32_t)mask) | (raw & mask);
            s->valid |= mask;
        }
    }
}
static void reject(Ledger *s, unsigned edges, uint32_t reason)
{
    for (unsigned i = 0; i < edges; ++i)
        increment(s, &s->rejected);
    ledger_halt(s, reason);
}
static bool interface_ok(const Ledger *s, uint32_t previous,
                         uint8_t raw, uint32_t stable, uint32_t edge)
{
    if (stable < OB_SETUP)
        return false;
    if ((previous ^ s->levels) & 3u)
        return false;
    if ((raw ^ s->levels) & 3u)
        return false;
    if (!(raw & edge))
        return false;
    uint32_t opposite = edge == 4u ? 8u : 4u;
    if ((previous | s->levels | raw) & opposite)
        return false;
    /* SCRUB qualifies RETURN only; ALLOCATE with SCRUB is reserved. */
    if (edge == 4u && (s->levels & 2u))
        return false;
    return true;
}
static void allocate(Ledger *s, unsigned slot)
{
    uint32_t bit = 1u << slot;
    if (s->active & bit) {
        reject(s, 1, OB_DUPLICATE);
        return;
    }
    if (!(s->clean & bit)) {
        reject(s, 1, OB_CREDIT);
        return;
    }
    /* Consumption is immediate; a return never regenerates this credit. */
    s->clean &= ~bit;
    s->active |= bit;
    unsigned depth = population(s->active);
    if (depth > s->peak)
        s->peak = depth;
    increment(s, &s->launched);
}
static void release(Ledger *s, unsigned slot)
{
    uint32_t bit = 1u << slot;
    if (!(s->active & bit)) {
        reject(s, 1, OB_ORPHAN);
        return;
    }
    s->active &= ~bit;
    increment(s, &s->completed);
    if (!s->active)
        increment(s, &s->rounds);
}
static void scrub(Ledger *s, unsigned slot)
{
    uint32_t bit = 1u << slot;
    /* No live scrubbing and no credit stacking on an already-clean slot. */
    if ((s->active | s->clean) & bit) {
        reject(s, 1, OB_CREDIT);
        return;
    }
    s->clean |= bit;
    increment(s, &s->scrubbed);
}
static void policy(Ledger *s, uint32_t previous, uint8_t raw)
{
    if (s->valid != 15u)
        return;
    increment(s, &s->observed);
    uint32_t stable = s->stable;
    if (((previous ^ s->levels) & 3u) || ((raw ^ s->levels) & 3u))
        s->stable = 0;
    else if (s->stable < OB_SETUP)
        ++s->stable;
    if (s->phase == OB_WAIT) {
        if (!((raw | s->levels) & 12u))
            s->phase = OB_TRACK;
        return;
    }
    uint32_t rising = s->levels & ~previous & 12u;
    unsigned edges = !!(rising & 4u) + !!(rising & 8u);
    if (!edges)
        return;
    for (unsigned i = 0; i < edges; ++i)
        increment(s, &s->attempts);
    if (edges == 2u || !interface_ok(s, previous, raw, stable, rising)) {
        reject(s, edges, OB_INTERFACE);
        return;
    }
    unsigned slot = s->levels & 1u;
    if (rising == 4u)
        allocate(s, slot);
    else if (s->levels & 2u)
        scrub(s, slot);
    else
        release(s, slot);
}
static void tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    s->now = now;
    if (s->started && delta != 1u) {
        s->gaps = 1;
        s->missed = delta - 1u;
        ledger_halt(s, OB_GAP);
        return;
    }
    uint32_t previous = s->levels;
    qualify(s, raw);
    s->started = true;
    policy(s, previous, raw);
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
    tick(&next, now, raw & 15u);
    if (next.exhausted) {
        s->now = now;
        ledger_halt(s, OB_EXHAUSTED);
    } else {
        *s = next;
    }
}
static bool ownership_valid(const Ledger *s)
{
    unsigned depth = population(s->active);
    if (s->active & s->clean)
        return false;
    if ((uint64_t)s->launched + population(s->clean) != s->scrubbed)
        return false;
    if ((uint64_t)s->completed + depth != s->launched)
        return false;
    if (s->peak > 2u || s->peak < depth || s->peak > s->launched)
        return false;
    if (!!s->peak != !!s->launched)
        return false;
    if (s->rounds > s->completed)
        return false;
    if (s->rounds + (uint64_t)!!depth > s->launched)
        return false;
    if (s->launched && !depth && !s->rounds)
        return false;
    /* Every closed episode has at least one release, at most all releases.
     * Peak one implies no overlapping lifetimes, hence every return drains. */
    if (s->peak < 2u && s->rounds != s->completed)
        return false;
    return true;
}
bool ledger_valid(const Ledger *s)
{
    if (!s)
        return false;
    if (s->levels > 15u || s->valid > 15u || s->faults > 127u)
        return false;
    if (s->phase > OB_TRACK || s->active > 3u || s->clean > 3u)
        return false;
    if (s->stable > OB_SETUP || s->halted > 1u)
        return false;
    if (!!s->halted != !!s->faults)
        return false;
    if (s->faults && (s->faults & (s->faults - 1u)))
        return false;
    if (s->halted && s->valid)
        return false;
    if (!s->halted && s->phase && s->valid != 15u)
        return false;
    if (!ownership_valid(s))
        return false;
    if ((uint64_t)s->launched + s->completed + s->scrubbed + s->rejected != s->attempts)
        return false;
    if (!s->phase && s->attempts)
        return false;
    if ((uint64_t)s->attempts > 2ull * s->observed)
        return false;
    if (s->rejected > 2u || (!!s->rejected != !!(s->faults & 15u)))
        return false;
    if (s->rejected == 2u && s->faults != OB_INTERFACE)
        return false;
    if (s->gaps > 1u || !!s->gaps != !!(s->faults & OB_GAP))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    return true;
}
