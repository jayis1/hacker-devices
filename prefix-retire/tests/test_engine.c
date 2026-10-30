/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t clock_ms;
static void hold(Ledger *s, unsigned raw, unsigned count)
{
    while (count--) {
        ledger_sample(s, ++clock_ms, raw);
        assert(ledger_valid(s));
    }
}
static void init(Ledger *s)
{
    clock_ms=0; ledger_init(s); hold(s,0,20);
    assert(s->phase==PR_TRACK);
}
static void pulse(Ledger *s,unsigned tag,unsigned strobe)
{
    hold(s,tag,20); hold(s,tag|strobe,10); hold(s,tag,10);
}
static void permutations(void)
{
    unsigned runs=0;
    for(unsigned a=0;a<4;a++) for(unsigned b=0;b<4;b++)
    for(unsigned c=0;c<4;c++) for(unsigned d=0;d<4;d++) {
        if(a==b||a==c||a==d||b==c||b==d||c==d) continue;
        Ledger s; init(&s);
        unsigned order[]={a,b,c,d};
        for(unsigned i=0;i<4;i++) pulse(&s,order[i],4);
        assert(s.ready==15 && s.prepared==4);
        for(unsigned i=0;i<4;i++) pulse(&s,i,8);
        assert(!s.halted && s.rounds==1 && s.retired==4 && !s.ready && !s.next);
        for(unsigned i=0;i<4;i++) { pulse(&s,i,4); pulse(&s,i,8); }
        assert(s.rounds==2 && s.prepared==8 && s.retired==8);
        runs++;
    }
    assert(runs==24);
}
static void faults(void)
{
    Ledger s;
    init(&s); pulse(&s,2,4); pulse(&s,2,8);
    assert(s.halted && s.faults==PR_ORDER && s.rejected==1 && s.next==0);
    init(&s); pulse(&s,0,8);
    assert(s.faults==PR_UNREADY && !s.retired);
    init(&s); pulse(&s,0,4); pulse(&s,0,8); pulse(&s,0,4);
    assert(s.faults==PR_DUPLICATE && s.ready==1 && s.next==1);
    init(&s); hold(&s,12,10);
    assert(s.faults==PR_INTERFACE && s.attempts==2 && s.rejected==2);
    init(&s); hold(&s,4,10); hold(&s,12,10);
    assert(s.faults==PR_INTERFACE && s.prepared==1 && s.rejected==1);
    init(&s); hold(&s,1|4,4);
    assert(s.faults==PR_INTERFACE && s.rejected==1);
    init(&s); pulse(&s,3,4); ledger_sample(&s,clock_ms+2,3);
    assert(s.faults==PR_GAP && s.missed==1 && s.ready==8);
    Ledger old=s; hold(&s,15,20);
    old.now=s.now; assert(!memcmp(&s,&old,sizeof s));
    init(&s); s.observed=UINT32_MAX; hold(&s,0,1);
    assert(s.faults==PR_EXHAUSTED && s.halted && s.observed==UINT32_MAX);
}
static void startup_and_wrap(void)
{
    Ledger s; clock_ms=0; ledger_init(&s); hold(&s,12,20);
    assert(!s.phase && !s.attempts);
    hold(&s,0,20); assert(s.phase==PR_TRACK);
    s.now=UINT32_MAX-2; clock_ms=s.now; hold(&s,0,10);
    assert(!s.halted);
    uint32_t observed=s.observed;
    ledger_sample(&s,s.now,15); assert(s.observed==observed);
}
static void serialization(void)
{
    Ledger s; init(&s);
    char frame[768];
    size_t n=wire_status(frame,sizeof frame,&s);
    assert(n && !memcmp(frame,"PR1,S,",6));
    assert(!wire_status(frame,n-1,&s));
    assert(wire_status(frame,n,&s)==n);
    assert(!wire_status(NULL,0,&s));
    s.ready=1; assert(!ledger_valid(&s));
    assert(!wire_status(frame,sizeof frame,&s));
}
static void all_retirement_orders(void)
{
    unsigned checked=0;
    for(unsigned a=0;a<4;a++) for(unsigned b=0;b<4;b++)
    for(unsigned c=0;c<4;c++) for(unsigned d=0;d<4;d++) {
        if(a==b||a==c||a==d||b==c||b==d||c==d) continue;
        for(unsigned e=0;e<4;e++) for(unsigned f=0;f<4;f++)
        for(unsigned g=0;g<4;g++) for(unsigned h=0;h<4;h++) {
            if(e==f||e==g||e==h||f==g||f==h||g==h) continue;
            Ledger s; init(&s);
            unsigned ready[]={a,b,c,d}, retire[]={e,f,g,h};
            for(unsigned i=0;i<4;i++) pulse(&s,ready[i],4);
            unsigned prefix=0;
            while(prefix<4 && retire[prefix]==prefix) prefix++;
            for(unsigned i=0;i<4;i++) pulse(&s,retire[i],8);
            assert(s.retired==prefix);
            assert(s.halted==(prefix!=4));
            assert(s.rounds==(prefix==4));
            checked++;
        }
    }
    assert(checked==576);
}
static void transactional_overflow_and_raw_guards(void)
{
    Ledger s; init(&s); hold(&s,3,20);
    s.rounds=536870911u; s.ready=15; s.next=3;
    s.prepared=2147483648u; s.retired=2147483647u;
    s.attempts=UINT32_MAX; s.observed=UINT32_MAX-100;
    assert(ledger_valid(&s));
    hold(&s,11,4);
    assert(s.faults==PR_EXHAUSTED && s.rounds==536870911u);
    assert(s.next==3 && s.ready==15 && s.retired==2147483647u);
    /* Opposite raw strobe need not qualify to make an edge ambiguous. */
    init(&s); hold(&s,4,3); hold(&s,12,1);
    assert(s.faults==PR_INTERFACE && s.attempts==1);
    /* A single raw tag excursion breaks setup even if qualified tag stays. */
    init(&s); hold(&s,4,2); hold(&s,5,1); hold(&s,4,1);
    assert(s.faults==PR_INTERFACE && s.rejected==1);
}
int main(void)
{
    permutations(); faults(); startup_and_wrap(); serialization();
    all_retirement_orders(); transactional_overflow_and_raw_guards();
    puts("engine: all readiness permutations, interleaving, boundaries, loss and wire PASS");
}
