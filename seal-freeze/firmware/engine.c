/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>
/* Counter updates commit atomically as a whole sample. */
static void add(Ledger *s, uint32_t *v)
{
    if (*v == UINT32_MAX)
        s->exhausted = true;
    else
        ++*v;
}
static void end_contract(Ledger *s)
{
    s->state = SF_UNKNOWN;
    s->captured = 0;
    s->mismatch = 0;
    s->unsealed = 0;
    memset(s->streak, 0, sizeof s->streak);
}
static void forget(Ledger *s)
{
    end_contract(s);
    s->valid = 0;
    s->reset_seen = false;
    s->tracked = false;
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
    forget(s);
    s->faults |= reason;
    s->halted = 1;
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail < UINT32_MAX)
        ++s->tx_fail;
}
/* Independent qualification prevents unrelated chatter hiding a violation.
   These are accepted levels, not proof of continuous electrical stability. */
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
        } else if (s->runs[i] < SF_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == SF_QUALIFY) {
            s->valid |= mask;
            if (high)
                s->levels |= mask;
            else
                s->levels &= ~(uint32_t)mask;
        }
    }
}
static void episode(Ledger *s, unsigned i, bool active)
{
    if (!active) {
        s->streak[i] = 0;
        return;
    }
    add(s, &s->totals[i]);
    if (!s->streak[i])
        add(s, &s->episodes[i]);
    add(s, &s->streak[i]);
    if (s->streak[i] > s->maximum[i])
        s->maximum[i] = s->streak[i];
    s->faults |= 1u << i;
}
static void observe(Ledger *s)
{
    if (s->valid != 15u)
        return;
    bool reset = (s->levels & 8u) != 0;
    bool seal = (s->levels & 4u) != 0;
    uint32_t value = s->levels & 3u;
    bool rise = s->tracked && seal && !(s->previous & 4u);
    if (reset) {
        end_contract(s);
        if (!s->tracked || !(s->previous & 8u))
            add(s, &s->resets);
        /* Require a reset observation with seal low. Holding reset and seal
           together cannot establish an eligible configuration interval. */
        if (!seal && !s->reset_seen) {
            s->reset_seen = true;
        }
    } else if (s->reset_seen) {
        s->reset_seen = false;
        if (!seal) {
            s->state = SF_OPEN;
        } else {
            add(s, &s->rejected);
            s->faults |= SF_REJECTED;
        }
    } else if (s->state == SF_OPEN && rise) {
        /* Do not assign a new value on the very observation that seals it. */
        if ((s->previous & 3u) != value) {
            add(s, &s->rejected);
            s->faults |= SF_REJECTED;
            end_contract(s);
        } else {
            s->captured = value;
            s->state = SF_SEALED;
            add(s, &s->seals);
        }
    }
    s->tracked = true;
    s->previous = (uint8_t)s->levels;
    if (s->state != SF_SEALED)
        return;
    add(s, &s->coverage);
    s->mismatch = value != s->captured;
    s->unsealed = !seal;
    /* Both findings can coexist; never sum them as elapsed wall time. */
    episode(s, 0, s->mismatch != 0);
    episode(s, 1, s->unsealed != 0);
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
        s->faults |= SF_GAP;
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
        ledger_halt(s, SF_EXHAUSTED);
    } else {
        *s = next;
    }
}
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
    return total <= s->coverage;
}
/* Necessary consistency checks; these cannot authenticate a trace. */
bool ledger_valid(const Ledger *s)
{
    if (!s || s->levels > 15u || s->valid > 15u || s->faults > 63u)
        return false;
    if (s->state > SF_SEALED || s->captured > 3u || s->halted > 1u)
        return false;
    if (!!s->halted != !!(s->faults & (SF_EXHAUSTED | SF_WATCHDOG)))
        return false;
    if (s->halted && (s->valid || s->state))
        return false;
    if (s->state && (s->valid != 15u || (s->levels & 8u) || !s->resets))
        return false;
    if (s->state != SF_SEALED && s->captured)
        return false;
    if (s->state == SF_SEALED && (!s->seals || !s->coverage))
        return false;
    if (s->seals > s->resets || s->seals > s->coverage)
        return false;
    if (s->coverage && !s->seals)
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    if (!!s->gaps != !!(s->faults & SF_GAP))
        return false;
    if (!!s->rejected != !!(s->faults & SF_REJECTED))
        return false;
    bool mismatch = s->state == SF_SEALED && (s->levels & 3u) != s->captured;
    bool unsealed = s->state == SF_SEALED && !(s->levels & 4u);
    if (s->mismatch != mismatch || s->unsealed != unsealed)
        return false;
    return episode_valid(s, 0, mismatch) && episode_valid(s, 1, unsealed);
}
