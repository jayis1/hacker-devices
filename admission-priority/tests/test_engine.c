/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
static Ledger s;
static uint32_t now;
static void hold(unsigned raw, unsigned ticks)
{
    while (ticks--) {
        ledger_sample(&s, ++now, raw);
        if (!ledger_valid(&s))
            fprintf(stderr,"invalid at %u: raw=%u starts=%u observed=%u flags=%u\n",now,raw,s.starts,s.observed,s.faults);
        assert(ledger_valid(&s));
    }
}
static void fresh(void)
{
    ledger_init(&s); now=0; hold(0,8);
}
int main(void)
{
    fresh();
    hold(3,8); hold(7,8); /* Both demand; high priority wins. */
    assert(s.accepted==1 && s.starts==1 && !s.faults);
    hold(3,8); hold(11,8); /* Both demand; low priority wrongly wins. */
    assert(s.inversions==1 && s.starts==2);
    fresh();
    hold(2,8); hold(10,8); hold(11,8); /* No preemption required. */
    assert(s.accepted==1 && !s.inversions && !s.orphans);
    hold(3,8); hold(7,8);
    assert(s.accepted==2 && s.high==1 && s.low==1);
    fresh(); hold(5,8); /* Simultaneous demand and grant is not prior setup. */
    assert(s.orphans==1 && !s.accepted);
    fresh(); hold(3,8); hold(15,8);
    assert(s.starts==2 && s.collided==2 && s.overlaps==1);
    hold(15,8); assert(s.starts==2 && s.overlaps==1);
    hold(7,8); hold(15,8); assert(s.overlaps==2 && s.collided==3);
    fresh(); hold(1,8); hold(5,8); hold(9,8); /* Direct handover: previous owner. */
    assert(s.collided==1 && s.starts==2);
    fresh(); hold(5,8); ledger_sample(&s,now+10,5); now+=10;
    hold(5,8); assert(!s.ready && s.starts==1 && s.gaps==1);
    hold(1,8); assert(s.ready); hold(5,8); assert(s.accepted==1);
    fresh();
    for(unsigned i=0;i<100;++i) hold((i&1)?3:1,1);
    hold(5,8); assert(s.accepted==1); /* Unrelated request chatter. */
    fresh(); uint32_t saved=s.observed; ledger_sample(&s,now,15);
    assert(s.observed==saved);
    fresh(); s.now=UINT32_MAX; now=UINT32_MAX; hold(0,1); assert(!s.gaps);
    fresh(); s.observed=UINT32_MAX; hold(0,1);
    assert(s.halted && (s.faults&AP_EXHAUSTED));
    fresh(); s.starts=UINT32_MAX; s.high=UINT32_MAX;
    s.accepted=UINT32_MAX; s.observed=UINT32_MAX/2u+1u;
    hold(1,8); hold(5,8);
    assert(s.halted && s.starts==UINT32_MAX && s.accepted==UINT32_MAX);
    char out[768]; assert(wire_status(out,sizeof out,&s));
    assert(!wire_status(out,4,&s)); s.accepted--;
    assert(!ledger_valid(&s) && !wire_status(out,sizeof out,&s));
    /* Exhaustively classify single admissions after every stable request mask. */
    for(unsigned req=0;req<4;++req) for(unsigned grants=1;grants<4;++grants) {
        fresh(); hold(req,8); hold(req|(grants<<2),8);
        unsigned n=(grants==3)?2:1;
        assert(s.starts==n);
        if(grants==3) assert(s.collided==2);
        else if(!(req&grants)) assert(s.orphans==1);
        else if(grants==2 && (req&1)) assert(s.inversions==1);
        else assert(s.accepted==1);
    }
    /* Independent event oracle, generous plateau qualification. */
    fresh(); unsigned expected[4]={0};
    for(unsigned i=0;i<10000;++i) {
        unsigned req=(i*17u+i/7u)&3u;
        unsigned grant=(i%3u)+1u;
        hold(req,8); hold(req|(grant<<2),8); hold(req,8);
        unsigned cls=grant==3?3:(!(req&grant)?1:(grant==2&&(req&1)?2:0));
        expected[cls]+=grant==3?2:1;
    }
    assert(s.accepted==expected[0] && s.orphans==expected[1]);
    assert(s.inversions==expected[2] && s.collided==expected[3]);
    puts("engine: admission truth table, nonpreemption, gaps, wrap, exhaustion, 10000-cycle oracle passed");
}
