/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "budget.h"
#include <limits.h>
#include <string.h>

void budget_init(Budget *s)
{
    memset(s, 0, sizeof *s);
}

/* Minted credits are conserved across invalidation as discarded credits.
 * Terminal faults retain the last accounting state for forensic export. */
bool budget_valid(const Budget *s)
{
    if (!s || s->phase > BB_HALTED || s->levels > 15 || s->qualified > 15)
        return false;
    if (s->tokens > BB_CAPACITY || s->refill >= BB_PERIOD)
        return false;
    if (s->empty_age > BB_GRACE || s->faults > 127)
        return false;
    if ((uint64_t)s->allowed + s->excess + s->denied != s->admissions)
        return false;
    if ((uint64_t)s->allowed + s->discarded + s->tokens != s->minted)
        return false;
    if (s->excess && !(s->faults & BB_EXCESS))
        return false;
    if (s->denied && !(s->faults & BB_DENIED))
        return false;
    if (s->missing && !(s->faults & BB_MISSING))
        return false;
    if (s->gaps && !(s->faults & BB_GAP))
        return false;
    if (s->tx_fail && !(s->faults & BB_TX))
        return false;
    bool terminal = (s->faults & (BB_EXHAUSTED | BB_WATCHDOG)) != 0;
    if (terminal != (s->phase == BB_HALTED))
        return false;
    if (s->phase == BB_UNKNOWN && (s->tokens || s->refill || s->empty_age))
        return false;
    if (s->phase == BB_TRACKING && (s->qualified != 15 || !(s->levels & 4)))
        return false;
    if (s->tokens && s->empty_age)
        return false;
    if (s->tokens == BB_CAPACITY && s->refill)
        return false;
    return true;
}

void budget_halt(Budget *s, uint32_t reason)
{
    s->faults |= reason;
    s->phase = BB_HALTED;
}

static bool reserve(Budget *s, uint32_t value)
{
    if (value != UINT32_MAX)
        return true;
    budget_halt(s, BB_EXHAUSTED);
    return false;
}

void budget_tx_failure(Budget *s)
{
    s->faults |= BB_TX;
    if (reserve(s, s->tx_fail))
        ++s->tx_fail;
}

static void invalidate(Budget *s)
{
    /* This sum is bounded by minted via the conservation invariant. */
    s->discarded += s->tokens;
    s->tokens = 0;
    s->refill = 0;
    s->empty_age = 0;
    s->baseline = 0;
    if (s->phase != BB_HALTED)
        s->phase = BB_UNKNOWN;
}

static void restart_qualification(Budget *s, unsigned raw)
{
    s->qualified = 0;
    for (unsigned i = 0; i < 4; ++i) {
        s->candidate[i] = (raw >> i) & 1u;
        s->stable[i] = 0;
    }
}

/* Qualifiers are independent: chatter on THROTTLE cannot mask ADMIT. */
static void qualify(Budget *s, unsigned raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        unsigned bit = 1u << i;
        unsigned value = (raw >> i) & 1u;
        if (value != s->candidate[i]) {
            s->candidate[i] = value;
            s->stable[i] = 0;
            continue;
        }
        if (s->stable[i] < BB_QUALIFY)
            ++s->stable[i];
        if (s->stable[i] == BB_QUALIFY) {
            s->qualified |= bit;
            s->levels = (s->levels & ~bit) | (value << i);
        }
    }
}

static bool quiet(const Budget *s, unsigned raw)
{
    /* THROTTLE may legitimately be high while an empty bucket recovers. */
    return s->qualified == 15 && (s->levels & 13u) == 4u &&
           (raw & 13u) == 4u;
}

static void baseline(Budget *s, unsigned raw)
{
    if (!quiet(s, raw)) {
        s->baseline = 0;
        return;
    }
    if (s->baseline < BB_BASELINE)
        ++s->baseline;
    if (s->baseline != BB_BASELINE)
        return;
    if (!reserve(s, s->segments))
        return;
    ++s->segments;
    s->phase = BB_TRACKING;
    /* No free startup burst: four seconds of observation earns four credits. */
    s->refill = 0;
}

static void admission(Budget *s)
{
    uint32_t *counter;
    uint32_t fault = 0;
    if (s->phase != BB_TRACKING || (s->levels & 8u)) {
        counter = &s->denied;
        fault = BB_DENIED;
    } else if (!s->tokens) {
        counter = &s->excess;
        fault = BB_EXCESS;
    } else {
        counter = &s->allowed;
    }
    /* Reserve both increments before mutating conservation equations. */
    if (!reserve(s, s->admissions) || !reserve(s, *counter))
        return;
    ++s->admissions;
    ++*counter;
    s->faults |= fault;
    if (!fault)
        --s->tokens;
}

static void throttle(Budget *s)
{
    if (s->tokens) {
        s->empty_age = 0;
        return;
    }
    if (s->empty_age < BB_GRACE)
        ++s->empty_age;
    if (s->empty_age < BB_GRACE || (s->levels & 2u))
        return;
    s->faults |= BB_MISSING;
    if (reserve(s, s->missing))
        ++s->missing;
}

static void replenish(Budget *s, bool was_full)
{
    /* A full interval cannot bank fractional credit for the next burst.
     * The consumption tick itself is not credited as a refill interval. */
    if (was_full || s->tokens == BB_CAPACITY) {
        s->refill = 0;
        return;
    }
    ++s->refill;
    if (s->refill < BB_PERIOD)
        return;
    s->refill = 0;
    if (!reserve(s, s->minted))
        return;
    ++s->minted;
    ++s->tokens;
    s->empty_age = 0;
}

void budget_sample(Budget *s, uint32_t now, unsigned raw)
{
    raw &= 15u;
    if (!s->started) {
        s->started = true;
        s->now = now;
        restart_qualification(s, raw);
        return;
    }
    uint32_t elapsed = now - s->now;
    if (!elapsed)
        return;
    s->now = now;
    if (s->phase == BB_HALTED)
        return;
    if (elapsed != 1u) {
        s->faults |= BB_GAP;
        if (!reserve(s, s->gaps))
            return;
        ++s->gaps;
        invalidate(s);
        restart_qualification(s, raw);
        return;
    }
    unsigned old = s->levels;
    unsigned known = s->qualified;
    qualify(s, raw);
    /* An already-high input on attachment is not a proved rising edge. */
    bool edge = (known & 1u) && !(old & 1u) && (s->levels & 1u);
    if ((s->qualified & 4u) && !(s->levels & 4u))
        invalidate(s);
    bool was_full = s->tokens == BB_CAPACITY;
    if (edge)
        admission(s);
    if (s->phase == BB_HALTED)
        return;
    if (s->phase == BB_UNKNOWN) {
        baseline(s, raw);
        return;
    }
    /* Conservative endpoint: admission and missing-throttle are evaluated
     * before refill on this tick, not retroactively authorized by it. */
    throttle(s);
    if (s->phase == BB_HALTED)
        return;
    replenish(s, was_full);
}
