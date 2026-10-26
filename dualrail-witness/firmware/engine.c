/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <limits.h>
#include <string.h>

/* Exhaustion is terminal; never wrap a diagnostic into a clean value. */
void witness_halt(Witness *w, uint32_t reason)
{
    w->faults |= reason;
    w->halted = true;
}
static bool increment(Witness *w, uint32_t *value)
{
    if (*value == UINT32_MAX) {
        witness_halt(w, DW_EXHAUSTED);
        return false;
    }
    ++*value;
    return true;
}
static bool add(Witness *w, uint32_t *value, uint32_t amount)
{
    if (UINT32_MAX - *value < amount) {
        witness_halt(w, DW_EXHAUSTED);
        return false;
    }
    *value += amount;
    return true;
}
void witness_init(Witness *w)
{
    memset(w, 0, sizeof *w);
}
void witness_tx_failure(Witness *w)
{
    w->faults |= DW_TX;
    /* Transport diagnostics may advance after measurement halted. */
    (void)increment(w, &w->tx_fail);
}
static bool invalid_code(uint8_t levels)
{
    uint8_t code = levels & 3u;
    return code == 0u || code == 3u;
}
static bool allowed(uint8_t levels)
{
    return (levels & 3u) == 1u && !(levels & 8u);
}
static void cancel_intervals(Witness *w)
{
    w->invalid_active = false;
    w->unsafe_active = false;
    w->silent_active = false;
    w->silent_reported = false;
    w->invalid_ms = 0;
    w->unsafe_ms = 0;
    w->silent_ms = 0;
}
static void qualify(Witness *w, uint8_t raw)
{
    /* Independent channels prevent unrelated chatter hiding a fault. */
    for (unsigned i = 0; i < 4; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw ^ w->candidate) & bit) {
            w->candidate ^= bit;
            w->age[i] = 0;
        } else if (w->age[i] < DW_QUALIFY) {
            ++w->age[i];
        }
        if (w->age[i] == DW_QUALIFY) {
            w->levels = (uint8_t)((w->levels & ~bit) | (raw & bit));
            w->valid |= bit;
        }
    }
}
static void advance_intervals(Witness *w)
{
    /* Charge the preceding interval before processing endpoint recovery. */
    if (w->invalid_active) {
        if (!increment(w, &w->invalid_ms))
            return;
        if (w->invalid_ms > w->max_invalid)
            w->max_invalid = w->invalid_ms;
    }
    if (w->unsafe_active) {
        if (!increment(w, &w->unsafe_ms))
            return;
        if (w->unsafe_ms > w->max_unsafe)
            w->max_unsafe = w->unsafe_ms;
    }
    if (w->silent_active) {
        if (!increment(w, &w->silent_ms))
            return;
        if (w->silent_ms >= DW_GRACE && !w->silent_reported) {
            if (!increment(w, &w->silent))
                return;
            w->silent_reported = true;
            w->faults |= DW_SILENT;
        }
    }
}
static void classify(Witness *w, bool prior_allowed)
{
    bool bad = invalid_code(w->levels);
    bool enabled = (w->levels & 4u) != 0;
    /* Same-tick permission and enable cannot establish causal precedence. */
    bool unsafe = enabled && (!allowed(w->levels) || !prior_allowed);
    bool silent = bad && !(w->levels & 8u);
    if (bad && !w->invalid_active) {
        if (!increment(w, &w->invalid))
            return;
        w->faults |= DW_INVALID;
    }
    if (unsafe && !w->unsafe_active) {
        if (!increment(w, &w->unsafe))
            return;
        w->faults |= DW_UNSAFE;
    }
    if (!bad)
        w->invalid_ms = 0;
    if (!unsafe)
        w->unsafe_ms = 0;
    if (!silent) {
        w->silent_ms = 0;
        w->silent_reported = false;
    }
    w->invalid_active = bad;
    w->unsafe_active = unsafe;
    w->silent_active = silent;
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
    if (elapsed != 1u) {
        w->faults |= DW_GAP;
        if (!increment(w, &w->gaps))
            return;
        if (!add(w, &w->missed, elapsed - 1u))
            return;
        w->valid = 0;
        w->candidate = raw;
        memset(w->age, 0, sizeof w->age);
        cancel_intervals(w);
        return;
    }
    bool prior_allowed = w->valid == 15u && allowed(w->levels);
    if (w->valid == 15u)
        advance_intervals(w);
    if (w->halted)
        return;
    qualify(w, raw);
    if (w->valid == 15u)
        classify(w, prior_allowed);
}
bool witness_consistent(const Witness *w)
{
    if (!w || w->levels > 15u || w->valid > 15u || w->faults > 127u)
        return false;
    if (w->invalid_ms > w->max_invalid || w->unsafe_ms > w->max_unsafe)
        return false;
    if (w->silent_ms > w->invalid_ms)
        return false;
    /* Episode totals are witnesses for every retained duration. */
    if (w->max_invalid && !w->invalid)
        return false;
    if (w->max_unsafe && !w->unsafe)
        return false;
    if (w->silent && (!w->invalid || w->max_invalid < DW_GRACE))
        return false;
    if (w->silent_ms >= DW_GRACE && !w->silent && !w->halted)
        return false;
    if (w->missed < w->gaps)
        return false;
    if ((w->invalid != 0) != ((w->faults & DW_INVALID) != 0))
        return false;
    if ((w->unsafe != 0) != ((w->faults & DW_UNSAFE) != 0))
        return false;
    if ((w->silent != 0) != ((w->faults & DW_SILENT) != 0))
        return false;
    if ((w->gaps != 0) != ((w->faults & DW_GAP) != 0) && !w->halted)
        return false;
    if ((w->tx_fail != 0) != ((w->faults & DW_TX) != 0) && !w->halted)
        return false;
    if (w->halted != ((w->faults & (DW_EXHAUSTED | DW_WATCHDOG)) != 0))
        return false;
    if (!w->halted && w->valid != 15u &&
        (w->invalid_ms || w->unsafe_ms || w->silent_ms))
        return false;
    if (!w->halted && w->valid == 15u) {
        if (!invalid_code(w->levels) && w->invalid_ms)
            return false;
        if (!(w->levels & 4u) && w->unsafe_ms)
            return false;
        if ((w->levels & 8u) && w->silent_ms)
            return false;
    }
    return true;
}
