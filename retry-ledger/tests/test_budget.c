/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "budget.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static void hold(Budget *b, uint8_t v, unsigned n)
{
    while (n--) budget_tick(b, ++now, v);
}
static void ready(Budget *b)
{
    now = 0;
    budget_init(b, now);
    hold(b, 0, 30);
    assert(b->phase == TRACKING);
}
static void fail(Budget *b)
{
    hold(b, INPUT_FAIL, 6);
    hold(b, 0, 6);
}
int main(void)
{
    Budget b;
    ready(&b);
    fail(&b); fail(&b);
    assert(b.strikes == 2 && b.failures == 2);
    hold(&b, INPUT_SUCCESS, 6);
    assert(b.strikes == 0 && b.successes == 1);
    hold(&b, 0, 6);
    fail(&b); fail(&b); fail(&b);
    assert(b.phase == LOCKOUT && b.cycles == 1);
    hold(&b, INPUT_LOCK, 30);
    assert(!(b.faults & FAULT_NO_LOCK));
    hold(&b, INPUT_LOCK | INPUT_ACCESS, 10);
    assert(b.faults & FAULT_ACCESS);
    hold(&b, INPUT_LOCK | INPUT_SUCCESS, 10);
    assert(b.faults & FAULT_LOCKED_SUCCESS);
    assert(b.strikes == 3);
    hold(&b, 0, 10);
    assert(b.faults & FAULT_EARLY_RELEASE);
    hold(&b, 0, LOCKOUT_MS + 30);
    assert(b.phase == TRACKING && b.strikes == 0);
    ready(&b);
    hold(&b, INPUT_FAIL | INPUT_SUCCESS, 6);
    assert(b.strikes == 1 && b.successes == 0 && (b.faults & FAULT_CONFLICT));
    hold(&b, INPUT_FAIL | INPUT_SUCCESS, 100);
    assert(b.failures == 1);
    ready(&b);
    fail(&b); fail(&b); fail(&b);
    hold(&b, 0, 30);
    assert(b.faults & FAULT_NO_LOCK);
    ready(&b);
    fail(&b); fail(&b);
    hold(&b, INPUT_FAIL, 4);
    assert(b.phase == LOCKOUT && b.lock_age == 0);
    hold(&b, 0, 16);
    hold(&b, INPUT_LOCK, 4);
    assert(b.lock_age == 20 && !(b.faults & FAULT_NO_LOCK));
    hold(&b, INPUT_LOCK, 4979);
    assert(b.lock_age == 4999 && b.phase == LOCKOUT);
    hold(&b, INPUT_LOCK, 1);
    assert(b.phase == RECOVERY && b.lock_age == 5000);
    hold(&b, INPUT_LOCK, 100);
    assert(b.phase == RECOVERY);
    hold(&b, 0, 30);
    assert(b.phase == TRACKING);
    uint32_t old = b.failures;
    now += 20;
    budget_tick(&b, now, INPUT_ACCESS);
    assert(b.phase == UNKNOWN && b.gaps == 1 && b.missed == 19);
    assert(b.failures == old && (b.faults & FAULT_GAP));
    hold(&b, INPUT_ACCESS, 40);
    assert(b.phase == UNKNOWN);
    hold(&b, 0, 30);
    assert(b.phase == TRACKING && b.segments == 3);
    ready(&b);
    hold(&b, INPUT_FAIL, 1);
    hold(&b, 0, 10);
    assert(b.failures == 0);
    /* Chatter on SUCCESS cannot hide a sustained FAIL. */
    for (unsigned i=0;i<20;++i)
        hold(&b, INPUT_FAIL | ((i & 1u) ? INPUT_SUCCESS : 0), 1);
    assert(b.failures == 1);
    ready(&b);
    fail(&b); fail(&b); fail(&b);
    hold(&b, INPUT_LOCK, 10);
    for (unsigned i=0;i<20;++i)
        hold(&b, INPUT_LOCK | INPUT_ACCESS | ((i & 1u) ? INPUT_FAIL : 0), 1);
    assert(b.faults & FAULT_ACCESS);
    now = UINT32_MAX - 5;
    budget_init(&b, now);
    hold(&b, 0, 30);
    assert(b.phase == TRACKING && b.gaps == 0);
    b.failures = UINT32_MAX;
    fail(&b);
    assert(b.failures == UINT32_MAX);
    for (unsigned i=0;i<40;++i) fail(&b);
    assert(b.dropped > 0);
    Event e;
    unsigned count = 0;
    while (budget_pop(&b, &e)) ++count;
    assert(count == EVENT_CAPACITY);
    char out[384];
    assert(wire_crc("123456789",9) == 0x29b1);
    assert(wire_status(out,sizeof out,&b,0) > 0);
    assert(strncmp(out,"RL1,S,",6)==0);
    assert(wire_status(out,3,&b,0)==0);
    assert(wire_status(NULL,100,&b,0)==0);
    assert(wire_policy(out,sizeof out)>0);
    puts("budget state machine, gaps, wrap, saturation, queue, CRC PASS");
}
