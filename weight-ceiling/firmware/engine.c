/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>
/* Every sample is speculative until all counters fit. A fault never retires
 * an accepted resource, and an exhaustion never commits half an operation. */
static void increment(Ledger *s, uint32_t *value)
{
    if (*value == UINT32_MAX)
        s->exhausted = true;
    else
        ++*value;
}
static unsigned population(uint32_t mask)
{
    unsigned count = 0;
    for (unsigned i = 0; i < 4; ++i)
        count += (mask >> i) & 1u;
    return count;
}
static unsigned weight(uint32_t mask)
{
    unsigned sum = 0;
    for (unsigned i = 0; i < 4; ++i)
        if (mask & (1u << i))
            sum += i + 1u;
    return sum;
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
    memset(s->runs, 0, sizeof s->runs);
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail != UINT32_MAX)
        ++s->tx_fail;
}
static void qualify(Ledger *s, uint8_t raw)
{
    /* Tag is atomic as a bundle; unrelated strobe chatter cannot postpone
     * qualification of the other strobe. Setup still guards the endpoint. */
    static const uint8_t masks[3] = {3u, 4u, 8u};
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t mask = masks[i];
        if (!s->started || ((s->candidates ^ raw) & mask)) {
            s->runs[i] = 0;
            s->candidates = (s->candidates & (uint8_t)~mask) | (raw & mask);
        } else if (s->runs[i] < WC_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == WC_QUALIFY) {
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
    if (stable < WC_SETUP)
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
static void allocate(Ledger *s, unsigned tag)
{
    uint32_t bit = 1u << tag;
    if (s->active & bit) {
        reject(s, 1, WC_DUPLICATE);
        return;
    }
    unsigned proposed_load = s->load + tag + 1u;
    if (proposed_load > WC_CEILING) {
        reject(s, 1, WC_BUDGET);
        return;
    }
    if (s->active)
        increment(s, &s->queued);
    s->active |= bit;
    s->load = proposed_load;
    if (s->load > s->peak)
        s->peak = s->load;
    increment(s, &s->launched);
}
static void release(Ledger *s, unsigned tag)
{
    uint32_t bit = 1u << tag;
    if (!(s->active & bit)) {
        reject(s, 1, WC_ORPHAN);
        return;
    }
    s->active &= ~bit;
    s->load -= tag + 1u;
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
    else if (s->stable < WC_SETUP)
        ++s->stable;
    if (s->phase == WC_WAIT) {
        if (!((raw | s->levels) & 12u))
            s->phase = WC_TRACK;
        return;
    }
    uint32_t rising = s->levels & ~previous & 12u;
    unsigned edges = !!(rising & 4u) + !!(rising & 8u);
    if (!edges)
        return;
    for (unsigned i = 0; i < edges; ++i)
        increment(s, &s->attempts);
    if (edges == 2u || !interface_ok(s, previous, raw, stable, rising)) {
        reject(s, edges, WC_INTERFACE);
        return;
    }
    unsigned tag = s->levels & 3u;
    if (rising == 4u)
        allocate(s, tag);
    else
        release(s, tag);
}
static void tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    s->now = now;
    if (s->started && delta != 1u) {
        s->gaps = 1;
        s->missed = delta - 1u;
        ledger_halt(s, WC_GAP);
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
        ledger_halt(s, WC_EXHAUSTED);
    } else {
        *s = next;
    }
}
static bool occupancy_valid(const Ledger *s)
{
    unsigned depth = population(s->active);
    if (s->load != weight(s->active) || s->load > WC_CEILING)
        return false;
    if (s->peak > WC_CEILING || s->peak < s->load)
        return false;
    if (!!s->peak != !!s->launched)
        return false;
    /* Each admission can add at most four credits; peak five needs overlap. */
    if ((uint64_t)s->peak > 4ull * s->launched)
        return false;
    if (s->peak == WC_CEILING && !s->queued)
        return false;
    if (s->queued && s->peak < 3u)
        return false;
    if ((uint64_t)s->completed + depth != s->launched)
        return false;
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
    if (s->phase > WC_TRACK || s->active > 15u)
        return false;
    if (s->stable > WC_SETUP || s->halted > 1u)
        return false;
    if (!!s->halted != !!s->faults)
        return false;
    if (s->faults && (s->faults & (s->faults - 1u)))
        return false;
    if (s->halted && s->valid)
        return false;
    if (!s->halted && s->phase && s->valid != 15u)
        return false;
    if (!occupancy_valid(s))
        return false;
    if ((uint64_t)s->launched + s->completed + s->rejected != s->attempts)
        return false;
    if (!s->phase && s->attempts)
        return false;
    if ((uint64_t)s->attempts > 2ull * s->observed)
        return false;
    if (s->rejected > 2u || (!!s->rejected != !!(s->faults & 15u)))
        return false;
    if (s->rejected == 2u && s->faults != WC_INTERFACE)
        return false;
    if (s->gaps > 1u || !!s->gaps != !!(s->faults & WC_GAP))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    return true;
}
