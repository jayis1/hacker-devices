/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* These masks name immediate prerequisites, not numerical rank. Siblings
 * one and two may start and finish in either order or overlap in time. */
static const uint8_t prerequisites[4] = {0u, 1u, 1u, 6u};
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
    /* No auto-recovery: quiet inputs do not establish an empty graph. */
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
     * chatter cannot indefinitely conceal a sustained START or FINISH. */
    static const uint8_t masks[3] = {3u, 4u, 8u};
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t mask = masks[i];
        if (!s->started || ((s->candidates ^ raw) & mask)) {
            s->runs[i] = 0;
            s->candidates = (s->candidates & (uint8_t)~mask) | (raw & mask);
        } else if (s->runs[i] < DD_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == DD_QUALIFY) {
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
    if (stable < DD_SETUP)
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
    /* Completion does not make a job reusable in the same graph round. */
    if ((s->active | s->done) & bit) {
        reject(s, 1, DD_DUPLICATE);
        return;
    }
    uint32_t required = prerequisites[tag];
    if ((s->done & required) != required) {
        reject(s, 1, DD_DEPENDENCY);
        return;
    }
    s->active |= bit;
    increment(s, &s->launched);
}
static void complete(Ledger *s, unsigned tag)
{
    uint32_t bit = 1u << tag;
    if (!(s->active & bit)) {
        reject(s, 1, DD_ORPHAN);
        return;
    }
    s->active &= ~bit;
    s->done |= bit;
    increment(s, &s->completed);
    if (s->done == 15u) {
        /* The sink's completion closes this graph. No cross-round overlap. */
        increment(s, &s->rounds);
        s->done = 0;
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
    else if (s->stable < DD_SETUP)
        ++s->stable;
    if (s->phase == DD_WAIT) {
        /* Operator establishes empty target graph before observer startup.
         * Electrical quiet alone is insufficient to prove that baseline. */
        if (!((raw | s->levels) & 12u))
            s->phase = DD_TRACK;
        return;
    }
    uint32_t rising = s->levels & ~previous & 12u;
    unsigned edges = !!(rising & 4u) + !!(rising & 8u);
    if (!edges)
        return;
    for (unsigned i = 0; i < edges; ++i)
        increment(s, &s->attempts);
    if (edges == 2u || !interface_ok(s, previous, raw, stable, rising)) {
        reject(s, edges, DD_INTERFACE);
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
        ledger_halt(s, DD_GAP);
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
    /* All accounting changes are transactional, including graph closure. */
    Ledger next = *s;
    tick(&next, now, raw & 15u);
    if (next.exhausted) {
        s->now = now;
        ledger_halt(s, DD_EXHAUSTED);
    } else {
        *s = next;
    }
}
static bool graph_valid(const Ledger *s)
{
    if (s->active & s->done)
        return false;
    /* A complete sink cannot persist: it atomically closes the graph. */
    if (s->done & 8u)
        return false;
    uint32_t seen = s->active | s->done;
    for (unsigned tag = 0; tag < 4; ++tag) {
        if (!(seen & (1u << tag)))
            continue;
        if ((s->done & prerequisites[tag]) != prerequisites[tag])
            return false;
    }
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
    if (s->phase > DD_TRACK || s->active > 15u || s->done > 7u)
        return false;
    if (s->stable > DD_SETUP || s->halted > 1u)
        return false;
    if (!!s->halted != !!s->faults)
        return false;
    if (s->faults && (s->faults & (s->faults - 1u)))
        return false;
    if (s->halted && s->valid)
        return false;
    if (!s->halted && s->phase && s->valid != 15u)
        return false;
    if (!graph_valid(s))
        return false;
    if (4ull * s->rounds + population(s->active | s->done) != s->launched)
        return false;
    if (4ull * s->rounds + population(s->done) != s->completed)
        return false;
    if ((uint64_t)s->launched + s->completed + s->rejected != s->attempts)
        return false;
    if (!s->phase && s->attempts)
        return false;
    if ((uint64_t)s->attempts > 2ull * s->observed)
        return false;
    if (s->rejected > 2u || (!!s->rejected != !!(s->faults & 15u)))
        return false;
    if (s->rejected == 2u && s->faults != DD_INTERFACE)
        return false;
    if (s->gaps > 1u || !!s->gaps != !!(s->faults & DD_GAP))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    return true;
}
