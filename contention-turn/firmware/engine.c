/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* One speculative copy owns all increments, including simultaneous edges. */
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
    s->turn = 0;
    s->collision_active = 0;
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
/* Qualify each channel independently: request chatter must not hide grants.
 * A changed candidate needs its initial sample plus three confirmations. */
static void qualify(Ledger *s, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t mask = (uint8_t)(1u << i);
        if (!s->started || (raw & mask) != (s->candidates & mask)) {
            s->candidates = (s->candidates & (uint8_t)~mask) | (raw & mask);
            s->runs[i] = 0;
        } else if (s->runs[i] < CT_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == CT_QUALIFY) {
            s->valid |= mask;
            s->levels = (s->levels & ~(uint32_t)mask) | (raw & mask);
        }
    }
}
/* Overlap can be observed before any admission baseline. */
static void overlap(Ledger *s)
{
    bool both = (s->valid & 12u) == 12u && (s->levels & 12u) == 12u;
    if (both) {
        if (!s->collision_active)
            add(s, &s->overlaps);
        add(s, &s->overlap_ticks);
        s->faults |= CT_COLLISION;
        s->turn = 0;
    }
    s->collision_active = both;
}
static void admit(Ledger *s, unsigned side, uint32_t previous)
{
    uint32_t request = 1u << side;
    uint32_t peer_request = side == 0u ? 2u : 1u;
    uint32_t peer_grant = side == 0u ? 8u : 4u;
    add(s, &s->starts);
    if (side == 0u)
        add(s, &s->side_a);
    else
        add(s, &s->side_b);
    /* A direct handover has no observed neutral ordering. Do not advance
     * a turn from either an overlapping admission or an orphan grant. */
    if ((previous | s->levels) & peer_grant) {
        add(s, &s->collided);
        s->faults |= CT_COLLISION;
        s->turn = 0;
        return;
    }
    if (!(previous & request) || !(s->levels & request)) {
        add(s, &s->orphans);
        s->faults |= CT_ORPHAN;
        s->turn = 0;
        return;
    }
    /* Contention requires the competing demand in BOTH adjacent samples.
     * A simultaneous arrival or withdrawal cannot prove selection order. */
    bool contention = (previous & s->levels & peer_request) != 0;
    if (contention)
        add(s, &s->contended);
    if (!s->turn) {
        /* The first clean admission establishes history, not an assumed
         * hardware reset seed. It is never classified as an unfair turn. */
        add(s, &s->seeds);
    }
    if (contention && s->turn && s->turn != side + 1u) {
        add(s, &s->unfair);
        s->faults |= CT_UNFAIR;
    } else {
        add(s, &s->accepted);
    }
    /* Track actual service, including a wrong selection. This is a local
     * round-robin rule, not an imagined ideal schedule that drifts forever. */
    s->turn = side == 0u ? 2u : 1u;
}
static void observe(Ledger *s, uint32_t previous)
{
    overlap(s);
    if (s->valid != 15u)
        return;
    add(s, &s->observed);
    if (!s->ready) {
        if (!(s->levels & 12u)) {
            s->ready = 1;
            add(s, &s->baselines);
        }
        return;
    }
    uint32_t rising = s->levels & ~previous & 12u;
    if (rising & 4u)
        admit(s, 0u, previous);
    if (rising & 8u)
        admit(s, 1u, previous);
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
        s->faults |= CT_GAP;
        forget(s);
        s->started = false;
    }
    uint32_t previous = s->levels;
    s->now = now;
    qualify(s, raw);
    s->started = true;
    observe(s, previous);
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
        ledger_halt(s, CT_EXHAUSTED);
    } else {
        *s = next;
    }
}
/* Necessary bounds for telemetry; they do not authenticate diagnostic GPIOs. */
static bool valid_flags(const Ledger *s)
{
    if (s->levels > 15u || s->valid > 15u || s->faults > 63u)
        return false;
    if (s->ready > 1u || s->halted > 1u || s->collision_active > 1u)
        return false;
    if (s->turn > 2u || (s->turn && (!s->ready || !s->seeds)))
        return false;
    if (!!s->halted != !!(s->faults & (CT_EXHAUSTED | CT_WATCHDOG)))
        return false;
    if (s->halted && (s->ready || s->valid || s->collision_active || s->turn))
        return false;
    if (s->ready && (s->valid != 15u || !s->baselines))
        return false;
    if (!s->halted) {
        bool both = (s->valid & 12u) == 12u && (s->levels & 12u) == 12u;
        if (!!s->collision_active != both || (both && s->turn))
            return false;
    }
    if (!!s->orphans != !!(s->faults & CT_ORPHAN))
        return false;
    if (!!s->unfair != !!(s->faults & CT_UNFAIR))
        return false;
    if (!!(s->collided || s->overlaps) != !!(s->faults & CT_COLLISION))
        return false;
    if (!!s->gaps != !!(s->faults & CT_GAP))
        return false;
    return true;
}
static bool valid_counts(const Ledger *s)
{
    uint64_t classified = (uint64_t)s->accepted + s->orphans + s->unfair + s->collided;
    uint64_t clean = (uint64_t)s->accepted + s->unfair;
    if (classified != s->starts || (uint64_t)s->side_a + s->side_b != s->starts)
        return false;
    if (s->contended > clean || s->unfair > s->contended)
        return false;
    if (s->seeds > s->accepted || (!!clean != !!s->seeds))
        return false;
    if ((uint64_t)s->seeds > 1ull + s->gaps + s->orphans + s->collided)
        return false;
    if (s->overlaps > s->overlap_ticks || (!!s->overlaps != !!s->overlap_ticks))
        return false;
    if (s->collision_active && !s->overlaps)
        return false;
    if (s->baselines > s->observed || (uint64_t)s->baselines > (uint64_t)s->gaps + 1u)
        return false;
    if (s->starts && !s->baselines)
        return false;
    if ((uint64_t)s->starts > 2ull * s->observed)
        return false;
    if (s->missed < s->gaps || (!s->gaps && s->missed))
        return false;
    return true;
}
bool ledger_valid(const Ledger *s)
{
    if (!s)
        return false;
    if (!valid_flags(s))
        return false;
    if (!valid_counts(s))
        return false;
    return true;
}
