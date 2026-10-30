/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* All mutations in a sample happen on a speculative copy. Counter exhaustion
 * commits neither a half-enqueue nor a half-dequeue. The old queue survives. */
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
    /* Quiet strobes do not prove an empty target queue after evidence loss. */
    memset(s->runs, 0, sizeof s->runs);
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail != UINT32_MAX)
        ++s->tx_fail;
}
static void qualify(Ledger *s, uint8_t raw)
{
    /* Tag bits are one bundle; ENQUEUE and DEQUEUE qualify independently. */
    static const uint8_t masks[3] = {3u, 4u, 8u};
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t mask = masks[i];
        if (!s->started || ((s->candidates ^ raw) & mask)) {
            s->runs[i] = 0;
            s->candidates = (s->candidates & (uint8_t)~mask) | (raw & mask);
        } else if (s->runs[i] < QO_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == QO_QUALIFY) {
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
    /* Require setup before the endpoint, not earned on that same sample. */
    if (stable < QO_SETUP)
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
static void enqueue(Ledger *s, unsigned tag)
{
    uint32_t bit = 1u << tag;
    unsigned depth = population(s->active);
    /* With four tags a full queue necessarily makes every enqueue duplicate.
     * Reject before the shift, so no packed entry can exceed the byte. */
    if (s->active & bit) {
        reject(s, 1, QO_DUPLICATE);
        return;
    }
    if (depth)
        increment(s, &s->queued);
    s->order |= tag << (2u * depth);
    s->active |= bit;
    if (depth + 1u > s->peak)
        s->peak = depth + 1u;
    increment(s, &s->launched);
}
static void dequeue(Ledger *s, unsigned tag)
{
    uint32_t bit = 1u << tag;
    if (!(s->active & bit)) {
        reject(s, 1, QO_ORPHAN);
        return;
    }
    if ((s->order & 3u) != tag) {
        reject(s, 1, QO_OVERTAKE);
        return;
    }
    s->order >>= 2;
    s->active &= ~bit;
    increment(s, &s->completed);
    if (!s->active)
        increment(s, &s->rounds);
}
static void policy(Ledger *s, uint32_t previous, uint8_t raw)
{
    if (s->valid != 15u)
        return;
    increment(s, &s->observed);
    uint32_t stable = s->stable;
    if (((previous ^ s->levels) & 3u) || ((raw ^ s->levels) & 3u))
        s->stable = 0;
    else if (s->stable < QO_SETUP)
        ++s->stable;
    if (s->phase == QO_WAIT) {
        /* Operator establishes empty queue before starting this instrument. */
        if (!((raw | s->levels) & 12u))
            s->phase = QO_TRACK;
        return;
    }
    uint32_t rising = s->levels & ~previous & 12u;
    unsigned edges = !!(rising & 4u) + !!(rising & 8u);
    if (!edges)
        return;
    for (unsigned i = 0; i < edges; ++i)
        increment(s, &s->attempts);
    if (edges == 2u || !interface_ok(s, previous, raw, stable, rising)) {
        reject(s, edges, QO_INTERFACE);
        return;
    }
    unsigned tag = s->levels & 3u;
    if (rising == 4u)
        enqueue(s, tag);
    else
        dequeue(s, tag);
}
static void tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    s->now = now;
    if (s->started && delta != 1u) {
        s->gaps = 1;
        s->missed = delta - 1u;
        ledger_halt(s, QO_GAP);
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
        ledger_halt(s, QO_EXHAUSTED);
    } else {
        *s = next;
    }
}
static bool queue_valid(const Ledger *s)
{
    unsigned depth = population(s->active);
    uint32_t bits = s->order, mask = 0;
    for (unsigned i = 0; i < depth; ++i) {
        unsigned tag = bits & 3u;
        uint32_t bit = 1u << tag;
        if (mask & bit)
            return false;
        mask |= bit;
        bits >>= 2;
    }
    if (bits || mask != s->active)
        return false;
    if (s->peak > 4u || s->peak < depth || s->peak > s->launched)
        return false;
    if (!!s->peak != !!s->launched)
        return false;
    if (s->peak && s->peak - 1u > s->queued)
        return false;
    if (s->queued && s->peak < 2u)
        return false;
    if ((uint64_t)s->completed + depth != s->launched)
        return false;
    /* Every nonempty episode has exactly one enqueue into an empty queue. */
    if ((uint64_t)s->rounds + !!s->active + s->queued != s->launched)
        return false;
    if (s->rounds > s->completed)
        return false;
    return true;
}
bool ledger_valid(const Ledger *s)
{
    if (!s)
        return false;
    if (s->levels > 15u || s->valid > 15u || s->faults > 127u)
        return false;
    if (s->phase > QO_TRACK || s->active > 15u || s->order > 255u)
        return false;
    if (s->stable > QO_SETUP || s->halted > 1u)
        return false;
    if (!!s->halted != !!s->faults)
        return false;
    if (s->faults && (s->faults & (s->faults - 1u)))
        return false;
    if (s->halted && s->valid)
        return false;
    if (!s->halted && s->phase && s->valid != 15u)
        return false;
    if (!queue_valid(s))
        return false;
    if ((uint64_t)s->launched + s->completed + s->rejected != s->attempts)
        return false;
    if (!s->phase && s->attempts)
        return false;
    if ((uint64_t)s->attempts > 2ull * s->observed)
        return false;
    if (s->rejected > 2u || (!!s->rejected != !!(s->faults & 15u)))
        return false;
    if (s->rejected == 2u && s->faults != QO_INTERFACE)
        return false;
    if (s->gaps > 1u || !!s->gaps != !!(s->faults & QO_GAP))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    return true;
}
