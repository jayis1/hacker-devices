/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* All sample updates are speculative. No counter may silently wrap and no
   partial histogram/episode transaction is published on exhaustion. */
static void add(Ledger *s, uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount) {
        s->exhausted = true;
        return;
    }
    *value += amount;
}

static void break_acquisition(Ledger *s)
{
    s->valid = 0;
    s->excess = 0;
    s->denied = 0;
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
    break_acquisition(s);
    s->faults |= reason;
    s->halted = 1;
}

void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail != UINT32_MAX)
        ++s->tx_fail;
}

/* Independent qualification is essential: unrelated noisy permission bits
   must not hide an already-observable containment failure on another lane.
   Retain old qualified values while a different candidate settles. */
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
        } else if (s->runs[i] < MC_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == MC_QUALIFY) {
            s->valid |= mask;
            if (high)
                s->levels |= mask;
            else
                s->levels &= ~(uint32_t)mask;
        }
    }
}

static bool pair_valid(const Ledger *s, unsigned lane)
{
    unsigned mask = (1u << lane) | (4u << lane);
    return (s->valid & mask) == mask;
}

static unsigned pair_state(const Ledger *s, unsigned lane)
{
    unsigned allowed = (s->levels >> lane) & 1u;
    unsigned effective = (s->levels >> (lane + 2u)) & 1u;
    return allowed | (effective << 1u);
}

static void episode(Ledger *s, unsigned index, bool active)
{
    if (!active) {
        s->streak[index] = 0;
        return;
    }
    if (!s->streak[index])
        add(s, &s->episodes[index], 1);
    add(s, &s->streak[index], 1);
    if (s->streak[index] > s->maximum[index])
        s->maximum[index] = s->streak[index];
}

static void observe(Ledger *s)
{
    s->excess = 0;
    s->denied = 0;
    for (unsigned lane = 0; lane < 2; ++lane) {
        if (!pair_valid(s, lane)) {
            s->streak[lane] = 0;
            s->streak[lane + 2u] = 0;
            continue;
        }
        unsigned state = pair_state(s, lane);
        add(s, &s->bins[4u * lane + state], 1);
        if (state == 2u) {
            s->excess |= 1u << lane;
            s->faults |= 1u << lane;
        }
        if (state == 1u)
            s->denied |= 1u << lane;
        episode(s, lane, state == 2u);
        episode(s, lane + 2u, state == 1u);
    }
}

static void sample_tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    bool continuous = s->started && delta == 1u;
    s->now = now;
    if (s->started && !continuous) {
        add(s, &s->gaps, 1);
        add(s, &s->missed, delta - 1u);
        s->faults |= MC_GAP;
        break_acquisition(s);
        s->started = false;
    }
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
        ledger_halt(s, MC_EXHAUSTED);
    } else {
        *s = next;
    }
}

/* Necessary realizability bounds shared with the host. They reject broken
   snapshots, not a malicious source able to invent a coherent history. */
static bool episode_valid(const Ledger *s, unsigned index, uint32_t total,
                          bool active)
{
    uint32_t count = s->episodes[index];
    uint32_t current = s->streak[index];
    uint32_t maximum = s->maximum[index];
    if (!!count != !!total || !!count != !!maximum)
        return false;
    if (count > total || current > maximum || maximum > total)
        return false;
    if (!!current != active)
        return false;
    /* Every other episode requires at least one sample of its own. */
    if (count && (uint64_t)maximum + count - 1u > total)
        return false;
    if (count == 1u && maximum != total)
        return false;
    if (count == 1u && active && current != total)
        return false;
    return true;
}

bool ledger_valid(const Ledger *s)
{
    if (!s)
        return false;
    if (s->levels > 15u || s->valid > 15u || s->faults > 31u)
        return false;
    if (s->halted > 1u || s->excess > 3u || s->denied > 3u)
        return false;
    if (!!s->halted != !!(s->faults & (MC_EXHAUSTED | MC_WATCHDOG)))
        return false;
    if (s->halted && (s->valid || s->excess || s->denied))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    if (!!s->gaps != !!(s->faults & MC_GAP))
        return false;
    unsigned expected_excess = 0;
    unsigned expected_denied = 0;
    for (unsigned lane = 0; lane < 2; ++lane) {
        bool valid = pair_valid(s, lane);
        unsigned state = pair_state(s, lane);
        bool excess = valid && state == 2u;
        bool denied = valid && state == 1u;
        if (excess)
            expected_excess |= 1u << lane;
        if (denied)
            expected_denied |= 1u << lane;
        if (valid && !s->bins[4u * lane + state])
            return false;
        if (!episode_valid(s, lane, s->bins[4u * lane + 2u], excess))
            return false;
        if (!episode_valid(s, lane + 2u, s->bins[4u * lane + 1u], denied))
            return false;
        if (!!s->episodes[lane] != !!(s->faults & (1u << lane)))
            return false;
    }
    if (s->excess != expected_excess || s->denied != expected_denied)
        return false;
    return true;
}
