/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* Evaluate on a copy: an exhausted sample must never partly clear a set. */
static void add(Ledger *s, uint32_t *counter)
{
    if (*counter == UINT32_MAX)
        s->exhausted = true;
    else
        ++*counter;
}
static unsigned population(uint32_t v)
{
    unsigned count = 0;
    for (unsigned i = 0; i < 4; ++i)
        count += (v >> i) & 1u;
    return count;
}
void ledger_init(Ledger *s)
{
    memset(s, 0, sizeof *s);
}
void ledger_halt(Ledger *s, uint32_t reason)
{
    if (s->halted)
        return;
    s->halted = 1;
    s->faults |= reason;
    s->valid = 0;
    /* Freeze phase and membership: halt does not manufacture session end. */
    memset(s->runs, 0, sizeof s->runs);
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail != UINT32_MAX)
        ++s->tx_fail;
}
static void qualify(Ledger *s, uint8_t raw)
{
    /* The two-bit identity is one bundle. Bitwise qualification could create
     * a tag that was never stable on the physical bus. Strobes are separate
     * so tag chatter cannot hide an ACCEPT edge. */
    const uint8_t masks[] = {3u, 4u, 8u};
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t mask = masks[i];
        if (!s->started || ((s->candidates ^ raw) & mask)) {
            s->runs[i] = 0;
            s->candidates = (s->candidates & (uint8_t)~mask) | (raw & mask);
        } else if (s->runs[i] < RS_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == RS_QUALIFY) {
            s->levels = (s->levels & ~(uint32_t)mask) | (raw & mask);
            s->valid |= mask;
        }
    }
}
static void invalid_admission(Ledger *s)
{
    add(s, &s->invalid);
    ledger_halt(s, RS_INVALID);
}
static void admission(Ledger *s, uint32_t previous, uint8_t raw,
                      uint32_t prior_stable)
{
    add(s, &s->admissions);
    /* SESSION must already have been high; a tied rising edge cannot
     * retroactively authorize acceptance. A raw withdrawal fails closed. */
    if (s->phase != RS_SESSION || !(previous & 8u) ||
        !(s->levels & raw & 8u) || prior_stable < RS_SETUP ||
        ((previous ^ s->levels) & 3u) || ((s->levels ^ raw) & 3u)) {
        invalid_admission(s);
        return;
    }
    uint32_t bit = 1u << (s->levels & 3u);
    if (s->seen & bit) {
        add(s, &s->duplicate);
        s->faults |= RS_DUPLICATE;
    } else {
        add(s, &s->fresh);
        s->seen |= bit;
    }
    /* Duplicates leave correlation intact: the known identity is still seen.
     * Finishing an operation or lowering ACCEPT never permits its reuse. */
}
static void policy(Ledger *s, uint32_t previous, uint8_t raw)
{
    if (s->valid != 15u)
        return;
    add(s, &s->observed);
    uint32_t prior_stable = s->stable;
    if (((previous ^ s->levels) & 3u) || ((raw ^ s->levels) & 3u))
        s->stable = 0;
    else if (s->stable < RS_SETUP)
        ++s->stable;
    bool quiet = !((raw | s->levels) & 12u);
    bool accept = (s->levels & ~previous & 4u) != 0;
    bool opening = (s->levels & ~previous & 8u) != 0;
    bool closing = (previous & ~s->levels & 8u) != 0;
    if (s->phase == RS_WAIT) {
        /* No claim about an already-running session at initial attachment. */
        if (quiet)
            s->phase = RS_IDLE;
        return;
    }
    /* Classify before session transitions: tied accept/reset is invalid. */
    if (accept) {
        admission(s, previous, raw, prior_stable);
        if (s->halted)
            return;
    }
    if (opening) {
        if (s->phase != RS_IDLE || ((previous | s->levels | raw) & 4u)) {
            ledger_halt(s, RS_INVALID);
            return;
        }
        s->phase = RS_SESSION;
        s->seen = 0;
        add(s, &s->sessions);
    }
    if (closing && s->phase == RS_SESSION) {
        /* Session withdrawal during a held ACCEPT loses the boundary even
         * without a new admission. Keep old evidence and stop, not reset. */
        if ((previous | s->levels | raw) & 4u) {
            ledger_halt(s, RS_INVALID);
            return;
        }
        s->phase = RS_IDLE;
        add(s, &s->ended);
        /* Keep the completed session's set visible until the next opening. */
    }
}
static void tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    s->now = now;
    if (s->started && delta != 1u) {
        s->gaps = 1;
        s->missed = delta - 1u;
        ledger_halt(s, RS_GAP);
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
        ledger_halt(s, RS_EXHAUSTED);
    } else {
        *s = next;
    }
}
/* Public necessary invariants are shared with the companion, but neither
 * these nor CRC can authenticate a source or prove physical acceptance. */
bool ledger_valid(const Ledger *s)
{
    if (!s)
        return false;
    if (s->levels > 15u || s->valid > 15u || s->faults > 31u)
        return false;
    if (s->phase > RS_SESSION || s->seen > 15u || s->stable > RS_SETUP)
        return false;
    if (s->halted > 1u || !!s->halted != !!(s->faults & 30u))
        return false;
    if (s->halted && s->valid)
        return false;
    if (!s->halted && s->phase && s->valid != 15u)
        return false;
    if ((uint64_t)s->ended + (s->phase == RS_SESSION) != s->sessions)
        return false;
    if ((uint64_t)s->fresh + s->duplicate + s->invalid != s->admissions)
        return false;
    if (s->fresh > 4ull * s->sessions || population(s->seen) > s->fresh)
        return false;
    if (!s->sessions && s->seen)
        return false;
    if (s->phase == RS_WAIT && (s->sessions || s->admissions))
        return false;
    if (s->admissions > s->observed || s->sessions > s->observed)
        return false;
    if (!!s->duplicate != !!(s->faults & RS_DUPLICATE))
        return false;
    if (s->duplicate && !s->fresh)
        return false;
    if (s->invalid > 1u || (s->invalid && !(s->faults & RS_INVALID)))
        return false;
    if (s->gaps > 1u || !!s->gaps != !!(s->faults & RS_GAP))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    if (!s->halted && s->phase == RS_SESSION && !(s->levels & 8u))
        return false;
    if (!s->halted && s->phase == RS_IDLE && (s->levels & 12u))
        return false;
    return true;
}
