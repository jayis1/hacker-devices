/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "budget.h"
#include <limits.h>
#include <string.h>

uint32_t budget_add(uint32_t a, uint32_t b)
{
    /* Lifetime counters never wrap into an apparently clean session. */
    if (UINT32_MAX - a < b)
        return UINT32_MAX;
    return a + b;
}

void budget_init(Budget *b)
{
    memset(b, 0, sizeof *b);
}

bool budget_valid(const Budget *b)
{
    /* Cheap snapshot invariants; never scan the ring in the acquisition loop.
     * Corrupted internal state is not emitted as plausible telemetry. */
    if (!b)
        return false;
    if (b->fill > DB_WINDOW || b->cursor >= DB_WINDOW)
        return false;
    if (b->occupied > b->fill || b->unauthorized > b->occupied)
        return false;
    if (b->peak > DB_WINDOW || b->occupied > b->peak)
        return false;
    if (b->total_denied > b->total_active)
        return false;
    if (b->occupied > b->total_active)
        return false;
    if (b->unauthorized > b->total_denied)
        return false;
    if (b->levels > 15u || b->faults > 127u)
        return false;
    if (b->over != (b->occupied > DB_LIMIT))
        return false;
    if (!b->over && b->over_age != 0)
        return false;
    if (b->over && (!(b->faults & DB_EXCESS) || !b->excess_episodes))
        return false;
    if (b->gaps && !(b->faults & DB_GAP))
        return false;
    if (b->tx_fail && !(b->faults & DB_TX))
        return false;
    if (b->total_denied && !(b->faults & (DB_NO_CONSENT | DB_INHIBITED)))
        return false;
    if ((b->faults & DB_WATCHDOG) && b->fill)
        return false;
    return true;
}

static void invalidate_window(Budget *b)
{
    /* Clear only prospective measurements, never historical findings. */
    memset(b->slots, 0, sizeof b->slots);
    b->fill = 0;
    b->cursor = 0;
    b->occupied = 0;
    b->unauthorized = 0;
    b->over = false;
    b->over_age = 0;
}

void budget_tx_failure(Budget *b)
{
    b->faults |= DB_TX;
    b->tx_fail = budget_add(b->tx_fail, 1);
}

static uint8_t classify(Budget *b, uint8_t levels)
{
    bool active = (levels & 1u) != 0;
    bool consent = (levels & 2u) != 0;
    bool inhibit = (levels & 4u) != 0;
    bool denied = active && (!consent || inhibit);
    if (active) {
        b->total_active = budget_add(b->total_active, 1);
        if (!consent)
            b->faults |= DB_NO_CONSENT;
        if (inhibit)
            b->faults |= DB_INHIBITED;
    }
    if (denied)
        b->total_denied = budget_add(b->total_denied, 1);
    return (uint8_t)((active ? 1u : 0u) | (denied ? 2u : 0u));
}

static void insert_slot(Budget *b, uint8_t slot)
{
    uint8_t old = b->slots[b->cursor];
    b->occupied -= old & 1u;
    b->unauthorized -= (old >> 1) & 1u;
    b->slots[b->cursor] = slot;
    b->occupied += slot & 1u;
    b->unauthorized += (slot >> 1) & 1u;
    ++b->cursor;
    if (b->cursor == DB_WINDOW)
        b->cursor = 0;
    if (b->fill < DB_WINDOW)
        ++b->fill;
    if (b->occupied > b->peak)
        b->peak = b->occupied;
}

static void check_policy(Budget *b, uint8_t levels)
{
    bool over = b->occupied > DB_LIMIT;
    if (over) {
        b->faults |= DB_EXCESS;
        if (!b->over) {
            b->excess_episodes = budget_add(b->excess_episodes, 1);
            b->over_age = 0;
        } else {
            b->over_age = budget_add(b->over_age, 1);
        }
        /* Alarm is a level contract while over budget, not a single pulse.
         * Initial assertion has a 20 sampled-ms grace interval. */
        if (b->over_age >= DB_RESPONSE && !(levels & 8u))
            b->faults |= DB_NO_ALARM;
    } else {
        b->over_age = 0;
    }
    b->over = over;
}

void budget_sample(Budget *b, uint32_t now, uint8_t levels)
{
    levels &= 15u;
    if (!b->started) {
        b->started = true;
        b->last = now;
        b->levels = levels;
        return;
    }
    uint32_t elapsed = now - b->last;
    if (elapsed == 0)
        return;
    b->last = now;
    b->levels = levels;
    if (elapsed != 1u) {
        b->gaps = budget_add(b->gaps, 1);
        b->missed = budget_add(b->missed, elapsed - 1u);
        b->faults |= DB_GAP;
        invalidate_window(b);
        return;
    }
    insert_slot(b, classify(b, levels));
    check_policy(b, levels);
}
