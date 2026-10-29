/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* Transactional updates prevent half-accounted simultaneous admissions. */
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
/* Separate qualification prevents request chatter hiding a grant edge.
 * Previously qualified levels persist during candidate changes. */
static void qualify(Ledger *s, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t mask = (uint8_t)(1u << i);
        if (!s->started || (raw & mask) != (s->candidates & mask)) {
            s->candidates = (s->candidates & (uint8_t)~mask) | (raw & mask);
            s->runs[i] = 0;
        } else if (s->runs[i] < AP_QUALIFY) {
            ++s->runs[i];
        }
        if (s->runs[i] == AP_QUALIFY) {
            s->valid |= mask;
            s->levels = (s->levels & ~(uint32_t)mask) | (raw & mask);
        }
    }
}
/* Overlap is observable without knowing the requests or a prior idle baseline.
 * This diagnostic is separate from admission classification. */
static void overlap(Ledger *s)
{
    bool both = (s->valid & 12u) == 12u && (s->levels & 12u) == 12u;
    if (both) {
        if (!s->collision_active)
            add(s, &s->overlaps);
        add(s, &s->overlap_ticks);
        s->faults |= AP_COLLISION;
    }
    s->collision_active = both;
}
static void admit(Ledger *s, unsigned side, uint32_t previous)
{
    uint32_t request = 1u << side;
    uint32_t peer_grant = side == 0u ? 8u : 4u;
    add(s, &s->starts);
    if (side == 0u)
        add(s, &s->high);
    else
        add(s, &s->low);
    /* Previous peer ownership counts even on a simultaneous handover. This
     * requires an observed neutral sample rather than inventing edge order. */
    if ((previous | s->levels) & peer_grant) {
        add(s, &s->collided);
        s->faults |= AP_COLLISION;
        return;
    }
    /* A request must predate admission and remain asserted at the edge.
     * Held grants are intentionally not rechecked: service is nonpreemptive. */
    if (!(previous & request) || !(s->levels & request)) {
        add(s, &s->orphans);
        s->faults |= AP_ORPHAN;
        return;
    }
    /* Either adjacent observation of high demand vetoes a low admission.
     * Simultaneous withdrawal does not prove the high request was gone first. */
    if (side == 1u && ((previous | s->levels) & 1u)) {
        add(s, &s->inversions);
        s->faults |= AP_INVERSION;
        return;
    }
    add(s, &s->accepted);
}
static void observe(Ledger *s, uint32_t previous)
{
    overlap(s);
    if (s->valid != 15u)
        return;
    add(s, &s->observed);
    if (!s->ready) {
        /* Attachment to an already asserted grant must not invent an edge. */
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
        s->faults |= AP_GAP;
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
        ledger_halt(s, AP_EXHAUSTED);
    } else {
        *s = next;
    }
}
/* These are necessary evidence bounds, not authentication or proof that every
 * accepted sparse record has a physically realizable underlying waveform. */
static bool valid_flags(const Ledger *s)
{
    if (s->levels > 15u || s->valid > 15u || s->faults > 63u)
        return false;
    if (s->ready > 1u || s->halted > 1u || s->collision_active > 1u)
        return false;
    if (!!s->halted != !!(s->faults & (AP_EXHAUSTED | AP_WATCHDOG)))
        return false;
    if (s->halted && (s->ready || s->valid || s->collision_active))
        return false;
    if (s->ready && (s->valid != 15u || !s->baselines))
        return false;
    if (!s->halted) {
        bool both = (s->valid & 12u) == 12u && (s->levels & 12u) == 12u;
        if (!!s->collision_active != both)
            return false;
    }
    if (!!s->orphans != !!(s->faults & AP_ORPHAN))
        return false;
    if (!!s->inversions != !!(s->faults & AP_INVERSION))
        return false;
    if (!!(s->collided || s->overlaps) != !!(s->faults & AP_COLLISION))
        return false;
    if (!!s->gaps != !!(s->faults & AP_GAP))
        return false;
    return true;
}
static bool valid_counts(const Ledger *s)
{
    uint64_t classes = (uint64_t)s->accepted + s->orphans;
    classes += (uint64_t)s->inversions + s->collided;
    if (classes != s->starts)
        return false;
    if ((uint64_t)s->high + s->low != s->starts)
        return false;
    if (s->inversions > s->low)
        return false;
    if (s->overlaps > s->overlap_ticks)
        return false;
    if (!!s->overlaps != !!s->overlap_ticks)
        return false;
    if (s->collision_active && !s->overlaps)
        return false;
    if (s->baselines > s->observed)
        return false;
    if ((uint64_t)s->baselines > (uint64_t)s->gaps + 1u)
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
