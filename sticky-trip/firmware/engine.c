/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* A sample is a transaction: even diagnostic exhaustion cannot publish half
   a transition. The caller commits only after every addition succeeds. */
static void add(Ledger *s, uint32_t *v)
{
    if (*v == UINT32_MAX)
        s->exhausted = true;
    else
        ++*v;
}
static void forget(Ledger *s)
{
    s->known = 0;
    s->expected = 0;
    s->valid = 0;
    s->missing = 0;
    s->spurious = 0;
    s->tracked = false;
    memset(s->runs, 0, sizeof s->runs);
    memset(s->streak, 0, sizeof s->streak);
}
void ledger_init(Ledger *s)
{
    memset(s, 0, sizeof *s);
}
void ledger_halt(Ledger *s, uint32_t reason)
{
    if (s->halted)
        return;
    forget(s);
    s->faults |= reason;
    s->halted = 1;
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail < UINT32_MAX)
        ++s->tx_fail;
}
/* Each channel qualifies independently. Once qualified, the old level is
   retained until the replacement candidate has three observed intervals. */
static void qualify(Ledger *s, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t mask = (uint8_t)(1u << i);
        bool high = (raw & mask) != 0;
        if (!s->started || high != ((s->candidates & mask) != 0)) {
            if (high)
                s->candidates |= mask;
            else
                s->candidates &= (uint8_t)~mask;
            s->runs[i] = 0;
        } else if (s->runs[i] < ST_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == ST_QUALIFY) {
            s->valid |= mask;
            if (high)
                s->levels |= mask;
            else
                s->levels &= ~(uint32_t)mask;
        }
    }
}
static void episode(Ledger *s, unsigned index, bool active)
{
    if (!active) {
        s->streak[index] = 0;
        return;
    }
    add(s, &s->totals[index]);
    if (!s->streak[index])
        add(s, &s->episodes[index]);
    add(s, &s->streak[index]);
    if (s->streak[index] > s->maximum[index])
        s->maximum[index] = s->streak[index];
    s->faults |= 1u << index;
}
static void observe(Ledger *s)
{
    if (s->valid != 15u)
        return;
    bool trip = (s->levels & 1u) != 0;
    bool clear = (s->levels & 2u) != 0;
    bool auth = (s->levels & 4u) != 0;
    bool latch = (s->levels & 8u) != 0;
    bool edge = s->tracked && clear && !(s->previous & 2u);
    /* Observing an already-high clear never establishes a clear edge. Auth
       must have been qualified in the preceding vector as well as now. */
    bool accepted = edge && auth && (s->previous & 4u) && !trip;
    if (edge && !accepted) {
        add(s, &s->rejected);
        s->faults |= ST_REJECTED;
    }
    if (trip) {
        if (!s->known || !s->expected)
            add(s, &s->sets);
        s->expected = 1;
        s->known = 1;
    } else if (accepted) {
        add(s, &s->clears);
        s->expected = 0;
        s->known = 1;
    }
    s->tracked = true;
    s->previous = (uint8_t)s->levels;
    if (!s->known)
        return;
    add(s, &s->coverage);
    s->missing = s->expected && !latch;
    s->spurious = !s->expected && latch;
    episode(s, 0, s->missing != 0);
    episode(s, 1, s->spurious != 0);
}
static void sample_tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    if (s->started && delta != 1u) {
        add(s, &s->gaps);
        if (UINT32_MAX - s->missed < delta - 1u)
            s->exhausted = true;
        else
            s->missed += delta - 1u;
        s->faults |= ST_GAP;
        forget(s);
        s->started = false;
    }
    s->now = now;
    qualify(s, raw);
    s->started = true;
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
        ledger_halt(s, ST_EXHAUSTED);
    } else {
        *s = next;
    }
}
/* Necessary realizability checks, not authentication or full trace proof. */
static bool episode_valid(const Ledger *s, unsigned i, bool active)
{
    uint32_t n = s->episodes[i];
    uint32_t total = s->totals[i];
    uint32_t current = s->streak[i];
    uint32_t maximum = s->maximum[i];
    if (!!n != !!total || !!n != !!maximum || !!current != active)
        return false;
    if (n > total || current > maximum || maximum > total)
        return false;
    if (n && (uint64_t)maximum + n - 1u > total)
        return false;
    if ((uint64_t)n * maximum < total)
        return false;
    if (n == 1u && (maximum != total || (active && current != total)))
        return false;
    if (!!total != !!(s->faults & (1u << i)))
        return false;
    return true;
}
bool ledger_valid(const Ledger *s)
{
    if (!s || s->levels > 15u || s->valid > 15u || s->faults > 63u)
        return false;
    if (s->known > 1u || s->expected > 1u || s->halted > 1u)
        return false;
    if (!!s->halted != !!(s->faults & (ST_EXHAUSTED | ST_WATCHDOG)))
        return false;
    if (s->halted && (s->valid || s->known))
        return false;
    if (s->known && (s->valid != 15u || !s->coverage))
        return false;
    if (!s->known && s->expected)
        return false;
    if (s->known && (s->levels & 1u) && !s->expected)
        return false;
    if (s->expected && !s->sets)
        return false;
    if (s->known && !s->expected && !s->clears)
        return false;
    if (s->coverage && !s->sets && !s->clears)
        return false;
    if ((uint64_t)s->sets + s->clears > s->coverage)
        return false;
    if ((uint64_t)s->totals[0] + s->totals[1] > s->coverage)
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    if (!!s->gaps != !!(s->faults & ST_GAP))
        return false;
    if (!!s->rejected != !!(s->faults & ST_REJECTED))
        return false;
    bool missing = s->known && s->expected && !(s->levels & 8u);
    bool spurious = s->known && !s->expected && (s->levels & 8u);
    if (s->missing != missing || s->spurious != spurious)
        return false;
    return episode_valid(s, 0, missing) && episode_valid(s, 1, spurious);
}
