/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

void witness_init(Witness *w)
{
    memset(w, 0, sizeof *w);
}
void witness_halt(Witness *w, uint32_t reason)
{
    w->faults |= reason;
    w->halted = true;
}
static bool increment(Witness *w, uint32_t *counter)
{
    if (*counter == UINT32_MAX) {
        witness_halt(w, PE_EXHAUSTED);
        return false;
    }
    ++*counter;
    return true;
}
void witness_tx_failure(Witness *w)
{
    w->faults |= PE_TX;
    (void)increment(w, &w->tx_fail);
}
static void qualify(Witness *w, uint8_t raw)
{
    /* Per-input qualification: one noisy lane cannot blind another. */
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw ^ w->candidate) & bit) {
            w->candidate ^= bit;
            w->age[i] = 0;
        } else if (w->age[i] < PE_QUALIFY) {
            ++w->age[i];
        }
        if (w->age[i] == PE_QUALIFY) {
            w->valid |= bit;
            w->levels = (uint8_t)((w->levels & ~bit) | (raw & bit));
        }
    }
}
static void advance(Channel *c)
{
    /* Censor long durations rather than retaining unbounded clocks. */
    uint32_t ceiling = c->phase == 1u ? PE_RECOVERY : PE_MAX + 1u;
    if (c->phase && c->age < ceiling)
        ++c->age;
}
static void observe(Witness *w, unsigned index)
{
    Channel *c = &w->channel[index];
    bool high = (w->levels & (1u << index)) != 0;
    if (c->phase == 0u) {
        /* A high seen at attachment has no known leading edge. */
        if (!high) {
            c->phase = 1u;
            c->age = 0;
        }
        return;
    }
    if (c->phase == 1u) {
        if (!high)
            return;
        if (!increment(w, &c->starts))
            return;
        if (c->age < PE_RECOVERY) {
            if (!increment(w, &c->rapid))
                return;
            w->faults |= PE_RAPID;
        }
        c->phase = 2u;
        c->age = 0;
        return;
    }
    /* The preceding high interval counts even on a falling endpoint. */
    if (c->age > c->peak)
        c->peak = c->age;

    if (high)
        return;
    if (!increment(w, &c->completed))
        return;
    c->last = c->age;
    if (c->age < PE_MIN) {
        if (!increment(w, &c->shorts))
            return;
        w->faults |= PE_SHORT;
    }
    c->phase = 1u;
    c->age = 0;
}
static void sample_contiguous(Witness *w, uint8_t raw)
{
    for (unsigned i = 0; i < 4; ++i) {
        Channel *c = &w->channel[i];
        bool crossing = c->phase == 2u && c->age == PE_MAX;
        advance(c);
        if (crossing) {
            if (!increment(w, &c->longs))
                return;
            w->faults |= PE_LONG;
        }
    }
    qualify(w, raw);
    for (unsigned i = 0; i < 4 && !w->halted; ++i) {
        if (w->valid & (1u << i))
            observe(w, i);
    }
}
static void gap(Witness *w, uint32_t elapsed, uint8_t raw)
{
    if (!increment(w, &w->gaps))
        return;
    if (UINT32_MAX - w->missed < elapsed - 1u) {
        witness_halt(w, PE_EXHAUSTED);
        return;
    }
    w->missed += elapsed - 1u;
    w->faults |= PE_GAP;
    for (unsigned i = 0; i < 4; ++i) {
        Channel *c = &w->channel[i];
        if (c->phase == 2u && !increment(w, &c->aborted))
            return;
        c->phase = 0;
        c->age = 0;
    }
    w->valid = 0;
    w->candidate = raw;
    memset(w->age, 0, sizeof w->age);
}
void witness_sample(Witness *w, uint32_t now, uint8_t raw)
{
    raw &= 15u;
    if (!w->started) {
        w->started = true;
        w->now = now;
        w->candidate = raw;
        return;
    }
    uint32_t elapsed = now - w->now;
    if (!elapsed)
        return;
    w->now = now;
    if (w->halted)
        return;
    /* Transactional accounting preserves conservation on exhaustion:
     * do not retain half of a multi-counter or multi-channel update. */
    Witness next = *w;
    if (elapsed == 1u)
        sample_contiguous(&next, raw);
    else
        gap(&next, elapsed, raw);
    if (next.halted)
        witness_halt(w, PE_EXHAUSTED);
    else
        *w = next;
}
static bool channel_consistent(const Channel *c)
{
    if (c->phase > 2u || c->age > PE_MAX + 1u)
        return false;
    if ((!c->phase && c->age) || (c->phase == 1u && c->age > PE_RECOVERY))
        return false;
    /* Widen before addition: impossible snapshots must not wrap. */
    uint64_t accounted = (uint64_t)c->completed + c->aborted + (c->phase == 2u);
    if (accounted != c->starts)
        return false;
    if (c->shorts > c->completed || c->longs > c->starts || c->rapid > c->starts)
        return false;
    if ((uint64_t)c->shorts + c->longs > c->starts)
        return false;
    if (c->last > c->peak || c->peak > PE_MAX + 1u)
        return false;
    if (!c->completed && c->last)
        return false;
    if (!c->starts && c->peak)
        return false;
    if ((c->longs != 0u) != (c->peak == PE_MAX + 1u))
        return false;
    if (c->phase == 2u && c->age > c->peak)
        return false;
    return true;
}
bool witness_consistent(const Witness *w)
{
    if (!w || w->levels > 15u || w->valid > 15u || w->faults > 127u)
        return false;
    if (w->halted != ((w->faults & (PE_EXHAUSTED | PE_WATCHDOG)) != 0))
        return false;
    if (w->missed < w->gaps)
        return false;
    if ((w->gaps != 0u) != ((w->faults & PE_GAP) != 0))
        return false;
    if ((w->tx_fail != 0u) != ((w->faults & PE_TX) != 0))
        return false;
    uint32_t flags = 0;
    for (unsigned i = 0; i < 4; ++i) {
        const Channel *c = &w->channel[i];
        if (!channel_consistent(c))
            return false;
        if (!(w->valid & (1u << i)) && c->phase)
            return false;
        if (c->phase == 1u && (w->levels & (1u << i)))
            return false;
        if (c->phase == 2u && !(w->levels & (1u << i)))
            return false;
        if (c->shorts)
            flags |= PE_SHORT;
        if (c->longs)
            flags |= PE_LONG;
        if (c->rapid)
            flags |= PE_RAPID;
    }
    return (w->faults & 7u) == flags;
}
