/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <string.h>

/* Every sample is a transaction. Exhaustion preserves the preceding ledger. */
static bool add(uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount)
        return false;
    *value += amount;
    return true;
}
void clock_init(Clock *c)
{
    memset(c, 0, sizeof *c);
}
void clock_halt(Clock *c, uint32_t fault)
{
    c->halted = true;
    c->faults |= fault;
}
void clock_tx_failure(Clock *c)
{
    if (c->tx_fail != UINT32_MAX)
        ++c->tx_fail;
}
/* Invalidated intervals are not silently converted into successful ratios. */
static bool abandon(Clock *c)
{
    if (c->anchored && !add(&c->aborted, 1))
        return false;
    c->anchored = false;
    c->refs = 0;
    c->age = 0;
    return true;
}
static uint8_t qualify(Clock *c, uint8_t raw)
{
    uint8_t rises = 0;
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw ^ c->candidate) & bit) {
            c->candidate ^= bit;
            c->stable[i] = 0;
        } else if (c->stable[i] < CR_QUALIFY) {
            ++c->stable[i];
        }
        if (c->stable[i] == CR_QUALIFY) {
            /* A high first observed at attachment is not a rising edge. */
            if ((c->valid & bit) && !(c->levels & bit) && (raw & bit))
                rises |= bit;
            c->levels = (uint8_t)((c->levels & ~bit) | (raw & bit));
            c->valid |= bit;
        }
    }
    return rises;
}
static bool gap(Clock *c, uint32_t delta, uint8_t raw)
{
    if (!add(&c->gaps, 1) || !add(&c->missed, delta - 1u))
        return false;
    c->faults |= CR_GAP;
    if (!abandon(c))
        return false;
    c->valid = 0;
    c->candidate = raw;
    memset(c->stable, 0, sizeof c->stable);
    c->tick_low = false;
    return true;
}
static bool process(Clock *c, uint8_t raw)
{
    uint8_t rise = qualify(c, raw);
    bool enabled = c->valid == 15 && (c->levels & 4u) &&
                   !(c->levels & 8u) && (raw & 4u) && !(raw & 8u);
    if (!enabled) {
        if (!abandon(c))
            return false;
        c->tick_low = false;
        if (rise & 2u) {
            c->faults |= CR_OUTSIDE;
            if (!add(&c->outside, 1))
                return false;
        }
        return true;
    }
    if (!(c->levels & 2u))
        c->tick_low = true;
    /* The age belongs to the interval preceding this qualified transition. */
    if (c->anchored)
        ++c->age;
    if (c->anchored && c->age > CR_DEADLINE) {
        c->faults |= CR_TIMEOUT;
        if (!add(&c->timeouts, 1) || !abandon(c))
            return false;
        /* A late tick cannot immediately anchor a replacement interval. */
        c->tick_low = false;
        return true;
    }
    if ((rise & 3u) == 3u) {
        c->faults |= CR_AMBIGUOUS;
        if (!add(&c->ambiguous, 1) || !abandon(c))
            return false;
        c->tick_low = false;
        return true;
    }
    if (c->anchored && (rise & 1u)) {
        /* Five denotes at least five, never an exact long-running count. */
        if (c->refs < CR_DIVISOR + 1u)
            ++c->refs;
        if (c->refs > CR_DIVISOR)
            c->faults |= CR_EXCESS;
    }
    if (!(rise & 2u) || !c->tick_low)
        return true;
    if (c->anchored) {
        if (!add(&c->intervals, 1))
            return false;
        if (c->refs == CR_DIVISOR) {
            if (!add(&c->good, 1))
                return false;
        } else {
            c->faults |= CR_RATIO;
            if (!add(&c->bad, 1))
                return false;
        }
    }
    if (!add(&c->anchors, 1))
        return false;
    c->anchored = true;
    c->tick_low = false;
    c->refs = 0;
    c->age = 0;
    return true;
}
void clock_sample(Clock *c, uint32_t now, uint8_t raw)
{
    raw &= 15u;
    if (!c->started) {
        c->started = true;
        c->now = now;
        c->candidate = raw;
        return;
    }
    uint32_t delta = now - c->now;
    if (!delta)
        return;
    c->now = now;
    if (c->halted)
        return;
    Clock next = *c;
    bool ok = delta == 1u ? process(&next, raw) : gap(&next, delta, raw);
    if (ok)
        *c = next;
    else
        clock_halt(c, CR_EXHAUSTED);
}
bool clock_valid(const Clock *c)
{
    if (!c || c->levels > 15 || c->candidate > 15 || c->valid > 15 || c->refs > 5 ||
        c->age > CR_DEADLINE || c->faults > 255)
        return false;
    for (unsigned i = 0; i < 4; ++i) {
        if (c->stable[i] > CR_QUALIFY)
            return false;
    }
    if (c->refs == 5 && !(c->faults & CR_EXCESS))
        return false;
    if ((c->faults & CR_RATIO) && !c->bad)
        return false;
    if ((c->faults & CR_AMBIGUOUS) && !c->ambiguous)
        return false;
    if ((c->faults & CR_TIMEOUT) && !c->timeouts)
        return false;
    if ((c->faults & CR_OUTSIDE) && !c->outside)
        return false;
    if ((c->faults & CR_GAP) && !c->gaps)
        return false;
    if (!c->gaps && c->missed)
        return false;
    if ((uint64_t)c->good + c->bad != c->intervals)
        return false;
    if ((uint64_t)c->intervals + c->aborted + (c->anchored ? 1u : 0u) != c->anchors)
        return false;
    if (!c->anchored && (c->age || c->refs))
        return false;
    if (c->anchored && (c->valid != 15 || !(c->levels & 4) || (c->levels & 8)))
        return false;
    if (c->bad && !(c->faults & CR_RATIO))
        return false;
    if (c->ambiguous && !(c->faults & CR_AMBIGUOUS))
        return false;
    if (c->timeouts && !(c->faults & CR_TIMEOUT))
        return false;
    if (c->outside && !(c->faults & CR_OUTSIDE))
        return false;
    if (c->gaps && !(c->faults & CR_GAP))
        return false;
    if (c->missed < c->gaps)
        return false;
    if (c->halted != ((c->faults & (CR_EXHAUSTED | CR_WATCHDOG)) != 0))
        return false;
    return true;
}
