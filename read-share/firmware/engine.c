/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* Tags 0..2 are distinct non-reentrant readers of one resource. Tag 3 is
 * its exclusive writer. No FIFO, rank, or writer-preference rule applies. */
static void increment(Ledger *s, uint32_t *value)
{
    if (*value == UINT32_MAX)
        s->exhausted = true;
    else
        ++*value;
}
static unsigned population(uint32_t mask)
{
    unsigned result = 0;
    for (unsigned i = 0; i < 4; ++i)
        result += (mask >> i) & 1u;
    return result;
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
    /* Quiet strobes cannot establish that all target owners released. */
    memset(s->runs, 0, sizeof s->runs);
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail != UINT32_MAX)
        ++s->tx_fail;
}
static void qualify(Ledger *s, uint8_t raw)
{
    /* Tags qualify as a bundle. Strobes qualify independently so data-bus
     * chatter cannot indefinitely conceal a sustained ACQUIRE or RELEASE. */
    static const uint8_t masks[3] = {3u, 4u, 8u};
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
static void reject(Ledger *s, unsigned edges, uint32_t reason)
{
    for (unsigned i = 0; i < edges; ++i)
        increment(s, &s->rejected);
    ledger_halt(s, reason);
}
static bool interface_ok(const Ledger *s, uint32_t previous,
                         uint8_t raw, uint32_t stable, uint32_t edge)
{
    /* Setup must exist before the edge, not be earned on that same sample.
     * Opposite raw high is ambiguous even if it has not yet qualified. */
    if (stable < RS_SETUP)
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
    return true;
}
static void launch(Ledger *s, unsigned tag)
{
    uint32_t bit = 1u << tag;
    /* Same-client reentrancy is not part of this interface contract. */
    if (s->active & bit) {
        reject(s, 1, RS_DUPLICATE);
        return;
    }
    if ((tag == 3u && s->active) || (tag != 3u && (s->active & 8u))) {
        reject(s, 1, RS_CONFLICT);
        return;
    }
    if (tag == 3u) {
        increment(s, &s->writers);
    } else {
        increment(s, &s->readers);
        if (s->active)
            increment(s, &s->shared);
    }
    s->active |= bit;
    s->seen |= bit;
    increment(s, &s->launched);
}
static void complete(Ledger *s, unsigned tag)
{
    uint32_t bit = 1u << tag;
    if (!(s->active & bit)) {
        reject(s, 1, RS_ORPHAN);
        return;
    }
    s->active &= ~bit;
    increment(s, &s->completed);
    if (!s->active) {
        /* A cohort closes on its last release, independent of identity. */
        increment(s, &s->rounds);
        s->seen = 0;
    }
}
static void policy(Ledger *s, uint32_t previous, uint8_t raw)
{
    if (s->valid != 15u)
        return;
    increment(s, &s->observed);
    uint32_t stable = s->stable;
    if (((previous ^ s->levels) & 3u) || ((raw ^ s->levels) & 3u))
        s->stable = 0;
    else if (s->stable < RS_SETUP)
        ++s->stable;
    if (s->phase == RS_WAIT) {
        /* Operator establishes an unowned resource before observer startup.
         * Electrical quiet alone is insufficient to prove that baseline. */
        if (!((raw | s->levels) & 12u))
            s->phase = RS_TRACK;
        return;
    }
    uint32_t rising = s->levels & ~previous & 12u;
    unsigned edges = !!(rising & 4u) + !!(rising & 8u);
    if (!edges)
        return;
    for (unsigned i = 0; i < edges; ++i)
        increment(s, &s->attempts);
    if (edges == 2u || !interface_ok(s, previous, raw, stable, rising)) {
        reject(s, edges, RS_INTERFACE);
        return;
    }
    unsigned tag = s->levels & 3u;
    if (rising == 4u)
        launch(s, tag);
    else
        complete(s, tag);
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
    /* All accounting changes are transactional, including cohort closure. */
    Ledger next = *s;
    tick(&next, now, raw & 15u);
    if (next.exhausted) {
        s->now = now;
        ledger_halt(s, RS_EXHAUSTED);
    } else {
        *s = next;
    }
}
static bool ownership_valid(const Ledger *s)
{
    if ((s->active & 8u) && (s->active & 7u))
        return false;
    if ((s->seen & s->active) != s->active)
        return false;
    if (!s->active && s->seen)
        return false;
    if ((s->seen & 8u) && s->seen != 8u)
        return false;
    if (s->active == 8u && s->seen != 8u)
        return false;
    if ((uint64_t)s->completed + population(s->active) != s->launched)
        return false;
    if ((uint64_t)s->readers + s->writers != s->launched)
        return false;
    if (s->shared > s->readers)
        return false;
    /* Each completed or open cohort has exactly one nonshared admission.
     * This remains true when a reader releases and later rejoins peers. */
    if ((uint64_t)s->rounds + !!s->active + s->shared != s->launched)
        return false;
    if (s->rounds > s->completed)
        return false;
    if (population(s->active & 7u) > s->readers)
        return false;
    if ((s->active & 8u) && !s->writers)
        return false;
    if (population(s->seen & 7u) > s->readers)
        return false;
    if (s->seen && population(s->seen) - 1u > s->shared)
        return false;
    return true;
}
/* Wide arithmetic protects evidence conservation checks from wrap. These
 * are necessary constraints, not authentication or a physical trace proof. */
bool ledger_valid(const Ledger *s)
{
    if (!s)
        return false;
    if (s->levels > 15u || s->valid > 15u || s->faults > 127u)
        return false;
    if (s->phase > RS_TRACK || s->active > 15u || s->seen > 15u)
        return false;
    if (s->stable > RS_SETUP || s->halted > 1u)
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
    if ((uint64_t)s->launched + s->completed + s->rejected != s->attempts)
        return false;
    if (!s->phase && s->attempts)
        return false;
    if ((uint64_t)s->attempts > 2ull * s->observed)
        return false;
    if (s->rejected > 2u || (!!s->rejected != !!(s->faults & 15u)))
        return false;
    if (s->rejected == 2u && s->faults != RS_INTERFACE)
        return false;
    if (s->gaps > 1u || !!s->gaps != !!(s->faults & RS_GAP))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    return true;
}
