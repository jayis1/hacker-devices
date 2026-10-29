/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
static void hold(Ledger *s,unsigned raw,unsigned n)
{
    while(n--) {
        ledger_sample(s,s->now+1,raw);
        assert(ledger_valid(s));
    }
}
static Ledger start(void)
{
    Ledger s; ledger_init(&s); hold(&s,0,4); return s;
}
static void seed_a(Ledger *s)
{
    hold(s,1,4); hold(s,5,4); hold(s,1,4);
    assert(s->turn==2);
}
int main(void)
{
    /* Enumerate every request mask, selected side, and last owner.
     * The independent oracle uses the last actual served side, not turn. */
    for(unsigned last=0;last<2;++last)
    for(unsigned requests=0;requests<4;++requests)
    for(unsigned side=0;side<2;++side) {
        Ledger s=start();
        hold(&s,3,4); hold(&s,3|(4u<<last),4); hold(&s,3,4);
        hold(&s,requests,4); hold(&s,requests|(4u<<side),4);
        bool orphan=!(requests&(1u<<side));
        bool wrong=!orphan && requests==3 && side==last;
        assert(s.starts==2 && s.orphans==(unsigned)orphan);
        assert(s.unfair==(unsigned)wrong);
        assert(s.accepted==2u-(unsigned)orphan-(unsigned)wrong);
        assert(s.turn==(orphan?0:2u-side));
    }
    /* Peer demand arriving or withdrawing with admission is not proven
     * continuous contention, but own simultaneous request remains orphan. */
    Ledger s=start(); seed_a(&s); hold(&s,7,4);
    assert(!s.unfair && s.accepted==2);
    s=start(); seed_a(&s); hold(&s,3,4); hold(&s,5,4);
    assert(!s.unfair && s.accepted==2);
    s=start(); hold(&s,5,4); assert(s.orphans==1 && !s.seeds);
    /* Direct handover is collided even without a both-high observation. */
    s=start(); hold(&s,3,4); hold(&s,7,4); hold(&s,11,4);
    assert(s.collided==1 && !s.overlaps && !s.turn);
    /* Mirrored selection edges and reseeding after ambiguous handover. */
    s=start(); hold(&s,2,4); hold(&s,10,4); hold(&s,2,4);
    hold(&s,11,4); assert(!s.unfair && s.accepted==2);
    hold(&s,3,4); hold(&s,10,4); assert(!s.unfair && s.accepted==3);
    hold(&s,7,4); assert(s.collided==1 && !s.turn);
    hold(&s,3,4); hold(&s,11,4);
    assert(s.turn==1 && s.seeds==2 && !s.unfair);
    /* An unrelated unqualified request cannot hide grant overlap. */
    ledger_init(&s);
    for(unsigned i=0;i<25;++i) hold(&s,12|(i&1),1);
    assert(s.overlaps==1 && !s.ready);
    hold(&s,0,4); hold(&s,12,4);
    assert(s.overlaps==2);
    /* Duplicate calls and rejected glitches do not advance the token. */
    s=start(); seed_a(&s); unsigned starts=s.starts;
    for(unsigned i=0;i<10;++i) { hold(&s,5,1); hold(&s,1,1); }
    assert(s.starts==starts && s.turn==2);
    /* Fail a speculative seed update after other counters incremented. */
    s=start(); s.starts=UINT_MAX; s.accepted=UINT_MAX;
    s.side_a=UINT_MAX; s.observed=UINT_MAX/2+1;
    s.seeds=1; s.turn=2;
    assert(ledger_valid(&s));
    hold(&s,3,4); hold(&s,11,4);
    assert(s.halted && s.starts==UINT_MAX && s.accepted==UINT_MAX);
    assert(!s.turn && s.faults==CT_EXHAUSTED);
    /* Two colliding admissions overflow together: neither may leak through. */
    s=start(); s.starts=UINT_MAX-1; s.accepted=UINT_MAX-1;
    s.side_a=UINT_MAX-1; s.observed=UINT_MAX/2+1;
    s.seeds=1; s.turn=2; hold(&s,3,4); hold(&s,15,4);
    assert(s.halted && s.starts==UINT_MAX-1 && !s.collided);
    assert(!s.overlaps && s.faults==CT_EXHAUSTED);
    /* Overlap occupancy overflow also rolls back entry and collision counts. */
    s=start(); s.overlaps=1; s.overlap_ticks=UINT_MAX; s.faults=CT_COLLISION;
    hold(&s,15,4);
    assert(s.halted && !s.starts && s.overlaps==1 && s.overlap_ticks==UINT_MAX);
    /* Necessary bounds are applied by the production serializer too. */
    char out[768];
    s=start(); seed_a(&s);
    Ledger bad=s; bad.turn=3; assert(!wire_status(out,sizeof out,&bad));
    bad=s; bad.contended=2; assert(!wire_status(out,sizeof out,&bad));
    bad=s; bad.seeds=0; assert(!wire_status(out,sizeof out,&bad));
    assert(!wire_status(out,1,&s)); assert(!wire_status(NULL,768,&s));
    /* Deterministic request-aware oracle with uncontrolled side choices. */
    s=start(); unsigned random=0x9251u,last=2,accepted=0,unfair=0,seeds=0,orphans=0;
    for(unsigned i=0;i<10000;++i) {
        random=random*1664525u+1013904223u;
        unsigned req=(random>>16)&3,side=(random>>23)&1;
        hold(&s,req,4); hold(&s,req|(4u<<side),4); hold(&s,req,4);
        if(!(req&(1u<<side))) { ++orphans; last=2; }
        else {
            if(last==2) ++seeds;
            if(req==3 && last==side) ++unfair; else ++accepted;
            last=side;
        }
        assert(s.starts==i+1 && s.accepted==accepted && s.unfair==unfair);
        assert(s.seeds==seeds && s.orphans==orphans);
        assert(s.turn==(last==2?0:2-last));
    }
    puts("edges: exhaustive masks, simultaneous demand, handover, glitches, 10000-admission oracle passed");
}
