/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
static void hold(Ledger *s,unsigned raw,unsigned ticks)
{
    while(ticks--) {
        ledger_sample(s,s->now+1,raw);
        assert(ledger_valid(s));
        char out[768]; assert(wire_status(out,sizeof out,s));
    }
}
static Ledger baseline(void)
{
    Ledger s; ledger_init(&s); hold(&s,0,5);
    assert(s.ready && !s.turn); return s;
}
static void admit(Ledger *s,unsigned requests,unsigned side)
{
    hold(s,requests,5); hold(s,requests|(4u<<side),5);
    hold(s,requests,5);
}
int main(void)
{
    Ledger s=baseline();
    admit(&s,3,0); assert(s.accepted==1 && s.turn==2 && s.seeds==1);
    admit(&s,3,1); assert(s.accepted==2 && s.turn==1);
    admit(&s,3,1); assert(s.unfair==1 && s.turn==1);
    admit(&s,3,0); assert(s.accepted==3 && s.turn==2);
    assert(s.contended==4 && s.starts==4);
    s=baseline();
    for(unsigned i=0;i<20;++i) admit(&s,1,0);
    assert(s.accepted==20 && !s.unfair && !s.contended && s.seeds==1);
    /* Demand during a held grant never creates a fresh admission. */
    hold(&s,5,5); unsigned before=s.starts; hold(&s,7,40);
    assert(s.starts==before && !s.unfair);
    /* No prior request is orphaned and invalidates the preference. */
    s=baseline(); admit(&s,3,0); hold(&s,0,5); hold(&s,8,5);
    assert(s.orphans==1 && !s.turn); hold(&s,0,5);
    admit(&s,3,0); assert(s.seeds==2 && !s.unfair);
    /* Simultaneous grants each collide and invalidate the preference. */
    s=baseline(); admit(&s,3,0); hold(&s,15,5);
    assert(s.collided==2 && !s.turn && s.overlaps==1);
    hold(&s,3,5); admit(&s,3,0); assert(s.seeds==2);
    /* Gaps clear correlation but not historical evidence. */
    s=baseline(); admit(&s,3,0);
    ledger_sample(&s,s.now+10,3); assert(!s.ready && !s.turn);
    hold(&s,3,5); admit(&s,3,0);
    assert(s.seeds==2 && s.baselines==2 && !s.unfair && s.gaps==1);
    /* Terminal exhaustion rolls back all speculative accounting. */
    s=baseline(); s.observed=UINT_MAX;
    ledger_sample(&s,s.now+1,0);
    assert(s.halted && s.faults==CT_EXHAUSTED && ledger_valid(&s));
    /* Ordinary wrap and duplicate tick do not manufacture a gap. */
    ledger_init(&s); s.now=UINT_MAX-10; hold(&s,0,20);
    assert(!s.gaps && s.ready); unsigned observed=s.observed;
    ledger_sample(&s,s.now,15); assert(s.observed==observed);
    /* Startup high grants are not admissions, but overlap is visible. */
    ledger_init(&s); hold(&s,15,8);
    assert(!s.ready && !s.starts && s.overlaps==1);
    hold(&s,0,5); assert(s.ready && !s.turn);
    puts("engine: alternation, demand skipping, seed recovery, gap, overflow, wrap passed");
}
