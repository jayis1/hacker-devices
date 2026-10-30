/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* Every acquisition tick is speculative. No half-counted paired edge can
 * escape when any cumulative counter reaches its representable boundary. */
static void add(Ledger *s, uint32_t *value)
{
    if (*value == UINT32_MAX)
        s->exhausted = true;
    else
        ++*value;
}
static void forget(Ledger *s)
{
    s->valid = 0;
    s->ready = 0;
    s->active = 0;
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
    s->halted = 1;
    s->faults |= reason;
}
void ledger_tx_failure(Ledger *s)
{
    if (s->tx_fail < UINT32_MAX)
        ++s->tx_fail;
}
/* Independent qualification avoids letting a chattering other client hide
 * an established rank violation. Four equal samples establish a level. */
static void qualify(Ledger *s, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t mask = (uint8_t)(1u << i);
        if (!s->started || (raw & mask) != (s->candidates & mask)) {
            s->candidates = (s->candidates & (uint8_t)~mask) | (raw & mask);
            s->runs[i] = 0;
        } else if (s->runs[i] < LR_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == LR_QUALIFY) {
            s->valid |= mask;
            s->levels = (s->levels & ~(uint32_t)mask) | (raw & mask);
        }
    }
}
/* Same-rank ownership is a separate defect, not part of rank ordering.
 * It can be seen even before a client's empty acquisition baseline. */
static void collisions(Ledger *s)
{
    for (unsigned rank = 0; rank < 2; ++rank) {
        uint32_t pair = 5u << rank;
        uint32_t bit = 1u << rank;
        bool both = (s->valid & pair) == pair && (s->levels & pair) == pair;
        if (both) {
            if (!(s->active & bit))
                add(s, &s->overlaps[rank]);
            add(s, &s->overlap_ticks[rank]);
            s->faults |= LR_COLLISION;
            s->active |= bit;
        } else {
            s->active &= ~bit;
        }
    }
}
static void acquisition(Ledger *s, unsigned side, uint32_t previous)
{
    Client *c = &s->client[side];
    unsigned shift = 2u * side;
    uint32_t pair = 3u << shift;
    uint32_t bit = 1u << side;
    if ((s->valid & pair) != pair)
        return;
    add(s, &c->observed);
    uint32_t current = (s->levels >> shift) & 3u;
    uint32_t before = (previous >> shift) & 3u;
    if (!(s->ready & bit)) {
        if (!current) {
            add(s, &c->baselines);
            s->ready |= bit;
        }
        return;
    }
    uint32_t rising = current & ~before;
    if (!rising)
        return;
    if (rising == 3u) {
        /* Simultaneous acquisition cannot establish a rank order. Both
         * starts are counted, neither is called ascending or descending. */
        add(s, &c->starts);
        add(s, &c->starts);
        add(s, &c->ambiguous);
        add(s, &c->ambiguous);
        s->faults |= LR_AMBIGUOUS;
    } else {
        add(s, &c->starts);
        if ((rising & 1u) && ((before | current) & 2u)) {
            /* A high release simultaneous with a low acquisition cannot
             * prove that release occurred first: retain a rank finding. */
            add(s, &c->descending);
            s->faults |= LR_DESCENDING;
        } else if ((rising & 2u) && (before & current & 1u)) {
            add(s, &c->ascending);
        } else {
            /* Acquiring only HIGH from empty is legal: skipping a rank
             * is not descending order. Releases have no LIFO constraint. */
            add(s, &c->solo);
        }
    }
}
static void tick(Ledger *s, uint32_t now, uint8_t raw)
{
    uint32_t delta = now - s->now;
    if (s->started && delta != 1u) {
        add(s, &s->gaps);
        if (UINT32_MAX - s->missed < delta - 1u)
            s->exhausted = true;
        else
            s->missed += delta - 1u;
        s->faults |= LR_GAP;
        forget(s);
        s->started = false;
    }
    uint32_t previous = s->levels;
    s->now = now;
    qualify(s, raw);
    s->started = true;
    collisions(s);
    acquisition(s, 0, previous);
    acquisition(s, 1, previous);
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
        ledger_halt(s, LR_EXHAUSTED);
    } else {
        *s = next;
    }
}
/* Validate necessary evidence invariants before emitting a status. This is
 * not authentication and cannot reconstruct unseen diagnostic waveforms. */
static bool valid_client(const Ledger *s, unsigned side)
{
    const Client *c = &s->client[side];
    uint64_t sum = (uint64_t)c->solo + c->ascending + c->descending + c->ambiguous;
    if (sum != c->starts)
        return false;
    if (c->ambiguous & 1u)
        return false;
    if (c->baselines > c->observed)
        return false;
    if ((uint64_t)c->baselines > (uint64_t)s->gaps + 1u)
        return false;
    if ((uint64_t)c->starts > 2ull * c->observed)
        return false;
    if (c->starts && !c->baselines)
        return false;
    if (s->ready & (1u << side)) {
        uint32_t pair = 3u << (2u * side);
        if ((s->valid & pair) != pair || !c->baselines)
            return false;
    }
    return true;
}
static bool valid_collision(const Ledger *s, unsigned rank)
{
    uint32_t pair = 5u << rank;
    bool active = (s->active & (1u << rank)) != 0;
    bool both = (s->valid & pair) == pair && (s->levels & pair) == pair;
    if (active != both)
        return false;
    if (active && !s->overlaps[rank])
        return false;
    if (s->overlaps[rank] > s->overlap_ticks[rank])
        return false;
    if (!!s->overlaps[rank] != !!s->overlap_ticks[rank])
        return false;
    return true;
}
bool ledger_valid(const Ledger *s)
{
    if (!s)
        return false;
    if (s->levels > 15u || s->valid > 15u || s->faults > 63u)
        return false;
    if (s->ready > 3u || s->active > 3u || s->halted > 1u)
        return false;
    if (!!s->halted != !!(s->faults & (LR_EXHAUSTED | LR_WATCHDOG)))
        return false;
    if (s->halted && (s->ready || s->valid || s->active))
        return false;
    if (!!(s->client[0].descending || s->client[1].descending) != !!(s->faults & LR_DESCENDING))
        return false;
    if (!!(s->client[0].ambiguous || s->client[1].ambiguous) != !!(s->faults & LR_AMBIGUOUS))
        return false;
    if (!!(s->overlaps[0] || s->overlaps[1]) != !!(s->faults & LR_COLLISION))
        return false;
    if (!!s->gaps != !!(s->faults & LR_GAP))
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    for (unsigned side = 0; side < 2; ++side) {
        if (!valid_client(s, side))
            return false;
        if (!valid_collision(s, side))
            return false;
    }
    return true;
}
