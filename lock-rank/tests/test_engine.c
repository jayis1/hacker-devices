/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Ledger s;
static uint32_t now;
static void hold(unsigned raw, unsigned count)
{
    for (unsigned i=0;i<count;i++) {
        ledger_sample(&s,++now,(uint8_t)raw);
        assert(ledger_valid(&s));
    }
}
static void fresh(void)
{
    ledger_init(&s); now=0; hold(0,4);
    assert(s.ready==3);
}
int main(void)
{
    char out[768];
    fresh(); hold(1,4); hold(3,4);
    assert(s.client[0].solo==1 && s.client[0].ascending==1);
    hold(2,4); hold(3,4);
    assert(s.client[0].descending==1);
    hold(0,4); hold(3,4);
    assert(s.client[0].ambiguous==2);
    fresh(); hold(2,4); hold(1,4);
    assert(s.client[0].descending==1); /* direct exchange is not proof of release first */
    fresh(); hold(8,4); hold(12,4);
    assert(s.client[1].descending==1 && s.client[0].starts==0);
    fresh(); hold(5,4);
    assert(s.overlaps[0]==1 && s.active==1);
    hold(0,4); assert(!s.active);
    fresh(); hold(10,4); assert(s.overlaps[1]==1);
    /* Chatter on client B must not suppress client A's rank checks. */
    fresh(); hold(2,4);
    for (unsigned i=0;i<8;i++) hold(3 | ((i&1)?4:0),1);
    assert(s.client[0].descending==1);
    fresh(); hold(2,4);
    ledger_sample(&s,now+=5,3); assert(s.gaps==1 && !s.ready);
    hold(3,4); assert(s.client[0].descending==0 && !(s.ready&1));
    hold(0,4); hold(2,4); hold(3,4);
    assert(s.client[0].descending==1);
    assert(wire_status(out,sizeof out,&s)>0);
    assert(!wire_status(out,1,&s));
    Ledger bad=s; bad.client[0].starts++;
    assert(!ledger_valid(&bad)); assert(!wire_status(out,sizeof out,&bad));
    fresh(); s.now=UINT32_MAX-1; now=s.now; hold(0,3);
    assert(!s.gaps);
    fresh(); s.client[0].starts=UINT32_MAX; s.client[0].solo=UINT32_MAX;
    s.client[0].observed=UINT32_MAX; /* internally consistent boundary */
    hold(1,4); assert(s.halted && (s.faults&LR_EXHAUSTED));
    assert(s.client[0].starts==UINT32_MAX);
    assert(wire_status(out,sizeof out,&s)>0);
    fresh(); ledger_halt(&s,LR_WATCHDOG); assert(ledger_valid(&s));
    /* Exhaustive steady-state transitions with an independent rank oracle. */
    for (unsigned before=0;before<16;before++) {
        for (unsigned after=0;after<16;after++) {
            fresh(); hold(before,4); Ledger previous=s; hold(after,4);
            for(unsigned side=0;side<2;side++) {
                unsigned p=(before>>(2*side))&3, n=(after>>(2*side))&3;
                unsigned rise=n&~p;
                unsigned count=(rise&1)+((rise>>1)&1);
                assert(s.client[side].starts-previous.client[side].starts==count);
                unsigned ambiguous=rise==3?2:0;
                unsigned down=rise!=3 && (rise&1) && ((p|n)&2);
                unsigned up=rise!=3 && (rise&2) && (p&n&1);
                assert(s.client[side].ambiguous-previous.client[side].ambiguous==ambiguous);
                assert(s.client[side].descending-previous.client[side].descending==down);
                assert(s.client[side].ascending-previous.client[side].ascending==up);
            }
        }
    }
    puts("engine: rank, ambiguity, collision, chatter, gap, wrap, overflow, 256 transitions PASS");
}
