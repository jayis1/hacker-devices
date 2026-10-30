/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* Every sample is transactional. Overflow must not retire half an operation. */
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
    /* Retain readiness and prefix. A fault is not an implicit batch abort. */
    memset(s->runs, 0, sizeof s->runs);
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail != UINT32_MAX)
        ++s->tx_fail;
}
static void qualify(Ledger *s, uint8_t raw)
{
    /* Ordinal bits form one bundle; independent qualification would invent
     * values during binary transitions. Strobes qualify independently. */
    static const uint8_t masks[3] = {3u, 4u, 8u};
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t mask = masks[i];
        if (!s->started || ((s->candidates ^ raw) & mask)) {
            s->runs[i] = 0;
            s->candidates = (s->candidates & (uint8_t)~mask) | (raw & mask);
        } else if (s->runs[i] < PR_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == PR_QUALIFY) {
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
    /* The tag must be stable BEFORE this operation, not become stable on it.
     * Both raw and qualified opposite strobes must be low at the endpoint. */
    if (stable < PR_SETUP)
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
static void prepare(Ledger *s, unsigned tag)
{
    uint32_t bit = 1u << tag;
    /* Keep ready bits even after retirement: a late duplicate must not be
     * mistaken for a fresh completion of a reused slot in the same batch. */
    if (s->ready & bit) {
        reject(s, 1, PR_DUPLICATE);
        return;
    }
    s->ready |= bit;
    increment(s, &s->prepared);
}
static void retire(Ledger *s, unsigned tag)
{
    /* Ordering has precedence over readiness for the primary finding. */
    if (tag != s->next) {
        reject(s, 1, PR_ORDER);
        return;
    }
    if (!(s->ready & (1u << tag))) {
        reject(s, 1, PR_UNREADY);
        return;
    }
    increment(s, &s->retired);
    ++s->next;
    if (s->next == 4u) {
        increment(s, &s->rounds);
        s->next = 0;
        s->ready = 0;
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
    else if (s->stable < PR_SETUP)
        ++s->stable;
    if (s->phase == PR_WAIT) {
        /* Electrical quiet does not prove a target batch boundary. The
         * operator must establish round zero before powering this observer. */
        if (!((raw | s->levels) & 12u))
            s->phase = PR_TRACK;
        return;
    }
    uint32_t rising = s->levels & ~previous & 12u;
    unsigned edges = !!(rising & 4u) + !!(rising & 8u);
    if (!edges)
        return;
    for (unsigned i = 0; i < edges; ++i)
        increment(s, &s->attempts);
    if (edges == 2u || !interface_ok(s, previous, raw, stable, rising)) {
        reject(s, edges, PR_INTERFACE);
        return;
    }
    unsigned tag = s->levels & 3u;
    if (rising == 4u)
        prepare(s, tag);
    else
        retire(s, tag);
}
static void tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    s->now = now;
    if (s->started && delta != 1u) {
        s->gaps = 1;
        s->missed = delta - 1u;
        ledger_halt(s, PR_GAP);
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
        ledger_halt(s, PR_EXHAUSTED);
    } else {
        *s = next;
    }
}
/* These are necessary evidence constraints, not authentication. They use
 * wide arithmetic so the checker itself cannot wrap into accepting a lie. */
bool ledger_valid(const Ledger *s)
{
    if (!s)
        return false;
    if (s->levels > 15u || s->valid > 15u || s->faults > 127u)
        return false;
    if (s->phase > PR_TRACK || s->ready > 15u || s->next > 3u)
        return false;
    if (s->stable > PR_SETUP || s->halted > 1u)
        return false;
    if (!!s->halted != !!s->faults)
        return false;
    if (s->faults && (s->faults & (s->faults - 1u)))
        return false;
    if (s->halted && s->valid)
        return false;
    if (!s->halted && s->phase && s->valid != 15u)
        return false;
    uint32_t prefix = (1u << s->next) - 1u;
    if ((s->ready & prefix) != prefix)
        return false;
    if (4ull * s->rounds + population(s->ready) != s->prepared)
        return false;
    if (4ull * s->rounds + s->next != s->retired)
        return false;
    if ((uint64_t)s->prepared + s->retired + s->rejected != s->attempts)
        return false;
    if (!s->phase && s->attempts)
        return false;
    if ((uint64_t)s->attempts > 2ull * s->observed)
        return false;
    if (s->rejected > 2u || (!!s->rejected != !!(s->faults & 15u)))
        return false;
    if (s->rejected == 2u && s->faults != PR_INTERFACE)
        return false;
    if (s->gaps > 1u || !!s->gaps != !!(s->faults & PR_GAP))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    return true;
}
