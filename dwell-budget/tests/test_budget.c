/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "budget.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
static Budget b;
static uint32_t t;
static void steps(unsigned n, unsigned bits)
{
    while (n--) budget_sample(&b, ++t, bits);
}
static void fresh(void)
{
    budget_init(&b);
    t = 0;
    budget_sample(&b, t, 0);
}
int main(void)
{
    fresh();
    steps(DB_WINDOW, 0);
    assert(b.fill == DB_WINDOW && b.occupied == 0 && b.faults == 0);
    steps(DB_LIMIT, 3);
    assert(b.occupied == DB_LIMIT && !b.over);
    steps(1, 3);
    assert(b.over && (b.faults & DB_EXCESS) && b.excess_episodes == 1);
    steps(DB_WINDOW, 0);
    assert(!b.over && b.occupied == 0 && b.faults == (DB_EXCESS | DB_NO_ALARM));
    fresh();
    steps(1, 1);
    assert(b.unauthorized == 1 && (b.faults & DB_NO_CONSENT));
    steps(1, 7);
    assert(b.unauthorized == 2 && (b.faults & DB_INHIBITED));
    steps(1, 3);
    assert(b.unauthorized == 2 && b.total_active == 3);
    budget_sample(&b, t += 10, 3);
    assert(b.fill == 0 && b.gaps == 1 && b.missed == 9);
    assert(b.total_active == 3 && (b.faults & DB_GAP));
    steps(DB_WINDOW, 0);
    assert(b.fill == DB_WINDOW && b.unauthorized == 0);
    fresh();
    steps(DB_LIMIT + DB_RESPONSE + 1, 3);
    assert(b.faults & DB_NO_ALARM);
    fresh();
    steps(DB_LIMIT, 3);
    steps(DB_RESPONSE + 1, 11);
    assert(!(b.faults & DB_NO_ALARM));
    fresh();
    budget_sample(&b, t, 15); /* duplicate timestamp cannot add a sample */
    assert(b.fill == 0);
    b.last = UINT32_MAX;
    t = UINT32_MAX;
    steps(1, 3);
    assert(b.fill == 1 && b.gaps == 0);
    b.total_active = UINT32_MAX;
    steps(1, 3);
    assert(b.total_active == UINT32_MAX);
    fresh();
    /* Compare the rolling implementation to a simple independent model. */
    unsigned char ring[DB_WINDOW] = {0};
    unsigned pos=0, sum=0, bad=0, rng=7;
    for(unsigned i=0; i<50000; ++i) {
        rng = rng*1664525u + 1013904223u;
        unsigned bits = (rng >> 24) & 15;
        sum -= ring[pos] & 1;
        bad -= (ring[pos] >> 1) & 1;
        unsigned active = bits & 1;
        unsigned denied = active && (!(bits & 2) || (bits & 4));
        ring[pos] = active | (denied << 1);
        sum += active;
        bad += denied;
        pos = (pos + 1) % DB_WINDOW;
        steps(1,bits);
        assert(b.occupied == sum && b.unauthorized == bad);
    }
    puts("PASS rolling oracle, boundaries, gaps, wrap, saturation, authorization, alarm");
}
