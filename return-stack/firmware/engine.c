/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>
/* Each observation commits atomically: exhaustion never commits half a use. */
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
    for (unsigned i = 0; i < UQ_SLOTS; ++i)
        count += (mask >> i) & 1u;
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
    memset(s->runs, 0, sizeof s->runs);
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail != UINT32_MAX)
        ++s->tx_fail;
}
static void qualify(Ledger *s, uint8_t raw)
{
    /* Atomic tag bundle, independent strobes: chatter cannot hide its peer. */
    static const uint8_t masks[3] = {3u, 4u, 8u};
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t mask = masks[i];
        if (!s->started || ((s->candidates ^ raw) & mask)) {
            s->runs[i] = 0;
            s->candidates = (s->candidates & (uint8_t)~mask) | (raw & mask);
        } else if (s->runs[i] < UQ_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == UQ_QUALIFY) {
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
    if (stable < UQ_SETUP)
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
bool ledger_eligible(const Ledger *s, unsigned tag)
{
    /* Greatest return ordinal is the free-list head. Virgin tags ascend. */
    if (!s || tag >= UQ_SLOTS || (s->active & (1u << tag)))
        return false;
    for (unsigned candidate = 0; candidate < UQ_SLOTS; ++candidate) {
        if (s->active & (1u << candidate))
            continue;
        if (s->returned[candidate] > s->returned[tag] ||
            (s->returned[candidate] == s->returned[tag] && candidate < tag))
            return false;
    }
    return true;
}
static void allocate(Ledger *s, unsigned tag)
{
    uint32_t bit = 1u << tag;
    if (s->active & bit) {
        reject(s, 1, UQ_DUPLICATE);
        return;
    }
    if (!ledger_eligible(s, tag)) {
        reject(s, 1, UQ_RETURN);
        return;
    }
    if (s->active)
        increment(s, &s->queued);
    s->active |= bit;
    ++s->depth;
    if (s->depth > s->peak)
        s->peak = s->depth;
    increment(s, &s->uses[tag]);
    increment(s, &s->launched);
    s->stamp[tag] = s->launched;
}
static void release(Ledger *s, unsigned tag)
{
    uint32_t bit = 1u << tag;
    if (!(s->active & bit)) {
        reject(s, 1, UQ_ORPHAN);
        return;
    }
    s->active &= ~bit;
    --s->depth;
    increment(s, &s->completed);
    s->returned[tag] = s->completed;
    if (!s->active)
        increment(s, &s->rounds);
    /* Cumulative use is intentionally not refunded. */
}
static void policy(Ledger *s, uint32_t previous, uint8_t raw)
{
    if (s->valid != 15u)
        return;
    increment(s, &s->observed);
    uint32_t stable = s->stable;
    if (((previous ^ s->levels) & 3u) || ((raw ^ s->levels) & 3u))
        s->stable = 0;
    else if (s->stable < UQ_SETUP)
        ++s->stable;
    if (s->phase == UQ_WAIT) {
        if (!((raw | s->levels) & 12u))
            s->phase = UQ_TRACK;
        return;
    }
    uint32_t rising = s->levels & ~previous & 12u;
    unsigned edges = !!(rising & 4u) + !!(rising & 8u);
    if (!edges)
        return;
    for (unsigned i = 0; i < edges; ++i)
        increment(s, &s->attempts);
    if (edges == 2u || !interface_ok(s, previous, raw, stable, rising)) {
        reject(s, edges, UQ_INTERFACE);
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
        ledger_halt(s, UQ_GAP);
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
        ledger_halt(s, UQ_EXHAUSTED);
    } else {
        *s = next;
    }
}
static bool occupancy_valid(const Ledger *s)
{
    if (s->depth != population(s->active))
        return false;
    /* Selection constrains admission, never simultaneous occupancy. */
    if (s->peak > UQ_SLOTS || s->peak < s->depth)
        return false;
    if (!!s->peak != !!s->launched || s->peak > s->launched)
        return false;
    if (s->queued && s->peak < 2u)
        return false;
    if (s->peak > 1u && s->queued < s->peak - 1u)
        return false;
    uint64_t sum = 0;
    for (unsigned i = 0; i < UQ_SLOTS; ++i) {
        sum += s->uses[i];
        if (!!s->uses[i] != !!s->stamp[i] || s->stamp[i] > s->launched)
            return false;
        if (s->uses[i] > s->stamp[i])
            return false;
        for (unsigned j = 0; j < i; ++j)
            if (s->stamp[i] && s->stamp[i] == s->stamp[j])
                return false;
        if ((s->active & (1u << i)) && !s->uses[i])
            return false;
    }
    uint32_t latest_return = 0;
    for (unsigned i = 0; i < UQ_SLOTS; ++i) {
        uint32_t busy = !!(s->active & (1u << i));
        if (s->uses[i] < busy)
            return false;
        uint32_t releases = s->uses[i] - busy;
        if (!!releases != !!s->returned[i] || releases > s->returned[i] ||
            s->returned[i] > s->completed)
            return false;
        if (s->returned[i] > latest_return)
            latest_return = s->returned[i];
        for (unsigned j = 0; j < i; ++j)
            if (s->returned[i] && s->returned[i] == s->returned[j])
                return false;
    }
    if (latest_return != s->completed)
        return false;
    /* The latest admission retains its stamp even after immediate release. */
    if (s->launched) {
        bool found = false;
        for (unsigned i = 0; i < UQ_SLOTS; ++i)
            found |= s->stamp[i] == s->launched;
        if (!found)
            return false;
    }
    if (sum != s->launched)
        return false;
    if ((uint64_t)s->completed + s->depth != s->launched)
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
    if (s->phase > UQ_TRACK || s->active > 15u)
        return false;
    if (s->stable > UQ_SETUP || s->halted > 1u)
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
    /* A finding requires a free identity other than the free-list head. */
    unsigned tag = s->levels & 3u;
    if (s->faults == UQ_RETURN &&
        ((s->active & (1u << tag)) || ledger_eligible(s, tag)))
        return false;
    if ((uint64_t)s->launched + s->completed + s->rejected != s->attempts)
        return false;
    if (!s->phase && s->attempts)
        return false;
    if ((uint64_t)s->attempts > 2ull * s->observed)
        return false;
    if (s->rejected > 2u || (!!s->rejected != !!(s->faults & 15u)))
        return false;
    if (s->rejected == 2u && s->faults != UQ_INTERFACE)
        return false;
    if (s->gaps > 1u || !!s->gaps != !!(s->faults & UQ_GAP))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    return true;
}
