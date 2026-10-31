/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
static void hold(Ledger *s,unsigned raw,unsigned n)
{
    while(n--) {
        ledger_sample(s,s->now+1,(uint8_t)raw);
        assert(ledger_valid(s));
    }
}
static void init(Ledger *s)
{
    ledger_init(s); hold(s,0,20);
}
static void op(Ledger *s,unsigned slot,unsigned edge,unsigned slow)
{
    unsigned tag=slot|slow*2;
    hold(s,tag,20); hold(s,tag|edge,4);
}
static void boundary(void)
{
    for(unsigned slot=0;slot<2;slot++) for(unsigned slow=0;slow<2;slow++) {
        Ledger base; init(&base); op(&base,slot,4,0); hold(&base,slot,20);
        op(&base,slot,8,slow);
        unsigned delay=slow?1000:250;
        assert(base.remaining[slot]==delay && base.cooling==(1u<<slot));
        for(unsigned age=delay-1;age<=delay+1;age++) {
            Ledger s=base;
            hold(&s,slot,age-4);
            hold(&s,slot|4,4);
            assert(s.halted==(age<delay));
            assert(s.faults==(age<delay?OB_EARLY:0));
            if(age>=delay) assert(s.active==(1u<<slot) && s.expired==1);
        }
    }
}
static void random_oracle(void)
{
    unsigned rng=17;
    for(unsigned trial=0;trial<200;trial++) {
        Ledger s; init(&s);
        unsigned occupied[2]={0,0}, until[2]={0,0};
        for(unsigned step=0;step<50;step++) {
            rng=rng*1664525u+1013904223u;
            unsigned slot=(rng>>5)&1, release=(rng>>9)&1, slow=(rng>>13)&1;
            hold(&s,0,20+(rng%300));
            unsigned edge=release?8:4;
            unsigned deadline=s.now+24;
            unsigned fault=release?(!occupied[slot]?OB_ORPHAN:0):
                (occupied[slot]?OB_DUPLICATE:(deadline<until[slot]?OB_EARLY:0));
            op(&s,slot,edge,release?slow:0);
            assert(s.faults==fault);
            if(fault) break;
            occupied[slot]=!release;
            if(release) until[slot]=s.now+(slow?1000:250);
            assert(s.active==(occupied[0]|occupied[1]<<1));
        }
    }
}
static void timer_failures(void)
{
    Ledger s; init(&s);
    op(&s,0,4,0); hold(&s,0,20); op(&s,1,4,0); hold(&s,1,20);
    op(&s,0,8,1);
    unsigned returned=s.now;
    hold(&s,1,726); op(&s,1,8,0);
    assert(s.now-returned==750 && s.remaining[0]==250 && s.remaining[1]==250);
    hold(&s,0,249);
    Ledger gap=s;
    ledger_sample(&gap,gap.now+2,0);
    assert(gap.faults==OB_GAP && gap.remaining[0]==1 && gap.remaining[1]==1);
    hold(&gap,0,500); assert(gap.remaining[0]==1 && gap.expired==0);
    Ledger exhausted=s;
    exhausted.observed=UINT_MAX;
    ledger_sample(&exhausted,exhausted.now+1,0);
    assert(exhausted.faults==OB_EXHAUSTED && exhausted.remaining[0]==1);
    assert(exhausted.remaining[1]==1 && exhausted.expired==0);
    assert(ledger_valid(&exhausted));
    hold(&s,0,1); assert(!s.cooling && s.expired==2);
}
int main(void)
{
    boundary(); random_oracle(); timer_failures();
    Ledger s; init(&s); op(&s,0,4,0); hold(&s,0,20); op(&s,0,8,1);
    hold(&s,0,20); op(&s,1,4,0); assert(s.active==2 && s.cooling==1);
    hold(&s,1,20); op(&s,1,8,0); assert(s.cooling==3);
    hold(&s,0,250); assert(s.cooling==1 && s.expired==1);
    hold(&s,0,1000); assert(!s.cooling && s.expired==2);
    init(&s); op(&s,0,4,1); assert(s.faults==OB_INTERFACE);
    init(&s); hold(&s,12,4); assert(s.faults==OB_INTERFACE && s.rejected==2);
    init(&s); op(&s,0,4,0); hold(&s,4,100); assert(s.launched==1);
    ledger_sample(&s,s.now+2,0); assert(s.faults==OB_GAP);
    unsigned attempts=s.attempts; hold(&s,12,100); assert(s.attempts==attempts);
    init(&s); s.now=UINT_MAX-2; hold(&s,0,5); assert(!s.halted && s.now==2);
    init(&s); s.observed=UINT_MAX; ledger_sample(&s,s.now+1,0);
    assert(s.faults==OB_EXHAUSTED && s.observed==UINT_MAX);
    char b[768]; assert(wire_status(b,sizeof b,&s)); assert(!wire_status(b,1,&s));
    init(&s); op(&s,0,4,0); hold(&s,0,20); op(&s,0,8,0);
    s.remaining[0]=0; assert(!ledger_valid(&s));
    puts("engine: independent deadline oracle, both slots/classes, exact boundaries, interleaving, faults, wrap, exhaustion PASS");
}
