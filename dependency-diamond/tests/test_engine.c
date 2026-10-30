/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
static uint32_t clock_ms;
static void hold(Ledger *s, unsigned raw, unsigned n)
{
    while (n--) {
        ledger_sample(s, ++clock_ms, (uint8_t)raw);
        assert(ledger_valid(s));
    }
}
static void baseline(Ledger *s)
{
    clock_ms=0; ledger_init(s); hold(s,0,20);
    assert(s->phase==DD_TRACK);
}
static void event(Ledger *s, unsigned tag, unsigned edge)
{
    hold(s,tag,12); hold(s,tag|edge,5); hold(s,tag,5);
}
static void root(Ledger *s)
{
    event(s,0,4); event(s,0,8);
}
/* Enumerate all 8! event orders against an independent prerequisite oracle.
 * Codes 0..3 are starts, 4..7 completions. Stop at the first illegal event. */
static unsigned permutations;
static void permutation(unsigned *order, unsigned used, unsigned depth)
{
    if(depth<8) {
        for(unsigned e=0;e<8;e++) if(!(used&(1u<<e))) {
            order[depth]=e; permutation(order,used|(1u<<e),depth+1);
        }
        return;
    }
    Ledger s; baseline(&s);
    unsigned running=0, finished=0;
    for(unsigned i=0;i<8;i++) {
        unsigned e=order[i],tag=e%4,bit=1u<<tag,reason=0;
        if(e<4) {
            if((running|finished)&bit) reason=DD_DUPLICATE;
            else if((tag==1 || tag==2) && !(finished&1)) reason=DD_DEPENDENCY;
            else if(tag==3 && (finished&6)!=6) reason=DD_DEPENDENCY;
            else running|=bit;
        } else {
            if(!(running&bit)) reason=DD_ORPHAN;
            else { running&=~bit; finished|=bit; }
        }
        event(&s,tag,e<4?4:8);
        assert(s.faults==reason);
        if(reason) break;
        if(finished==15) { finished=0; assert(s.rounds==1); }
        assert(s.active==running && s.done==finished);
    }
    permutations++;
}
int main(void)
{
    unsigned order[8]; permutation(order,0,0); assert(permutations==40320);
    Ledger s; baseline(&s); root(&s);
    event(&s,2,4); event(&s,1,4); assert(s.active==6);
    event(&s,1,8); event(&s,2,8); event(&s,3,4); event(&s,3,8);
    assert(s.rounds==1 && s.launched==4 && s.completed==4);
    root(&s); assert(s.done==1 && s.rounds==1);
    baseline(&s); event(&s,0,4); event(&s,0,4); assert(s.faults==DD_DUPLICATE);
    baseline(&s); root(&s); event(&s,0,4); assert(s.faults==DD_DUPLICATE);
    baseline(&s); root(&s); event(&s,1,4); event(&s,1,8);
    event(&s,3,4); assert(s.faults==DD_DEPENDENCY);
    baseline(&s); hold(&s,12,5); assert(s.rejected==2 && s.faults==DD_INTERFACE);
    /* Opposite raw strobe appears only at the qualifying endpoint. */
    baseline(&s); hold(&s,4,3); hold(&s,12,1); assert(s.faults==DD_INTERFACE);
    baseline(&s); hold(&s,1,3); hold(&s,5,4); assert(s.faults==DD_INTERFACE);
    baseline(&s); hold(&s,4,30); assert(s.launched==1 && !s.halted);
    /* Tag chatter cannot postpone independent strobe qualification. */
    baseline(&s); for(unsigned i=0;i<4;i++) hold(&s,4|(i&1),1);
    assert(s.faults==DD_INTERFACE);
    baseline(&s); event(&s,0,4); ledger_sample(&s,clock_ms+2,0);
    assert(s.faults==DD_GAP && s.active==1 && s.missed==1);
    Ledger retained=s; hold(&s,15,10); assert(s.active==retained.active && s.attempts==retained.attempts);
    ledger_init(&s); clock_ms=UINT32_MAX-10; hold(&s,0,20); assert(!s.halted);
    baseline(&s); s.rounds=(UINT32_MAX-7u)/8u;
    s.launched=4*s.rounds; s.completed=4*s.rounds;
    s.attempts=s.launched+s.completed; s.observed=s.attempts/2u;
    root(&s); event(&s,1,4); event(&s,2,4); event(&s,1,8); event(&s,2,8); event(&s,3,4);
    assert(s.attempts==UINT32_MAX && s.active==8 && s.done==7);
    event(&s,3,8); assert(s.faults==DD_EXHAUSTED && s.active==8 && s.done==7);
    assert(ledger_valid(&s));
    char output[768]; assert(wire_status(output,sizeof output,&s));
    assert(!wire_status(output,2,&s)); assert(!wire_status(NULL,768,&s));
    baseline(&s); s.active=8; s.launched=1; s.attempts=1;
    assert(!ledger_valid(&s));
    puts("engine: 40320 event permutations, parallelism, duplicate/orphan/dependency, raw guards, wrap, gap, exhaustion PASS");
}
