/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static uint32_t clock_ms;
static unsigned words;
static void hold(Ledger *s, unsigned raw, unsigned n)
{
    while (n--) {
        ledger_sample(s, ++clock_ms, raw);
        assert(ledger_valid(s));
    }
}
static void start(Ledger *s)
{
    clock_ms=0; ledger_init(s); hold(s,0,20);
}
static void op(Ledger *s,unsigned tag,bool alloc)
{
    hold(s,tag,12); hold(s,tag|(alloc?4u:8u),6); hold(s,tag,6);
}
/* Independent oracle: a sorted list of vacant numeric identities. */
typedef struct { unsigned free[4], count; } Oracle;
static bool allowed(const Oracle *o, unsigned tag)
{
    return o->count && o->free[0]==tag;
}
static bool vacant(const Oracle *o, unsigned tag)
{
    for(unsigned i=0;i<o->count;i++) if(o->free[i]==tag) return true;
    return false;
}
static void enumerate(Ledger state, Oracle oracle, unsigned depth)
{
    if(!depth || state.halted) return;
    for(unsigned action=0;action<8;action++) {
        Ledger next=state; Oracle expected=oracle;
        unsigned tag=action%4, reason=0;
        if(action<4) {
            if(!vacant(&oracle,tag)) reason=4;
            else if(!allowed(&oracle,tag)) reason=1;
            else {
                for(unsigned j=1;j<expected.count;j++) expected.free[j-1]=expected.free[j];
                expected.count--;
            }
        } else {
            if(vacant(&oracle,tag)) reason=2;
            else {
                unsigned pos=0;
                while(pos<expected.count && expected.free[pos]<tag) pos++;
                for(unsigned j=expected.count;j>pos;j--) expected.free[j]=expected.free[j-1];
                expected.free[pos]=tag; expected.count++;
            }
        }
        clock_ms=next.now; op(&next,tag,action<4);
        assert(next.faults==reason);
        for(unsigned j=0;j<4;j++) {
            assert(!!(next.active&(1u<<j))==!vacant(&expected,j));
            assert(ledger_eligible(&next,j)==allowed(&expected,j));
        }
        words++; enumerate(next,expected,depth-1);
    }
}
int main(void)
{
    Ledger s;
    start(&s);
    assert(ledger_eligible(&s,0));
    for(unsigned i=1;i<4;i++) assert(!ledger_eligible(&s,i));
    Oracle empty={{0,1,2,3},4}; enumerate(s,empty,10);
    /* Every occupancy is reachable by filling then returning arbitrary slots. */
    for(unsigned mask=0;mask<16;mask++) {
        start(&s); for(unsigned i=0;i<4;i++) op(&s,i,true);
        for(unsigned i=0;i<4;i++) if(!(mask&(1u<<i))) op(&s,i,false);
        unsigned first=0; while(first<4 && (mask&(1u<<first))) first++;
        for(unsigned i=0;i<4;i++) assert(ledger_eligible(&s,i)==(i==first));
        assert(!s.halted && s.active==mask);
    }
    /* Drain does not rotate preference; return order does not set rank. */
    start(&s); op(&s,0,true); op(&s,0,false); op(&s,0,true);
    assert(!s.halted && s.uses[0]==2);
    start(&s); for(unsigned i=0;i<4;i++) op(&s,i,true);
    op(&s,3,false); op(&s,1,false); op(&s,1,true); op(&s,3,true);
    assert(!s.halted);
    start(&s); op(&s,0,true); op(&s,2,true); assert(s.faults==1 && s.active==1);
    start(&s); op(&s,0,true); op(&s,0,true); assert(s.faults==4);
    start(&s); op(&s,2,false); assert(s.faults==2);
    start(&s); hold(&s,12,6); assert(s.faults==8 && s.rejected==2);
    start(&s); ledger_sample(&s,s.now+2,0); assert(s.faults==16 && ledger_valid(&s));
    Ledger frozen=s; clock_ms=s.now; hold(&s,15,20); frozen.now=s.now;
    assert(!memcmp(&s,&frozen,sizeof s));
    start(&s); hold(&s,4,3); s.observed=UINT32_MAX; hold(&s,4,1);
    assert(s.faults==32 && !s.attempts && !s.stamp[0]);
    start(&s); hold(&s,4,3); hold(&s,12,1); assert(s.faults==8);
    start(&s); hold(&s,4,3); hold(&s,5,1); assert(s.faults==8);
    start(&s); Ledger old=s; ledger_sample(&s,s.now,15); assert(!memcmp(&s,&old,sizeof s));
    s.now=UINT32_MAX; clock_ms=UINT32_MAX; hold(&s,0,1); assert(!s.halted);
    char out[768]; assert(wire_status(out,sizeof out,&s));
    assert(!wire_status(out,3,&s) && !wire_status(NULL,100,&s));
    start(&s); op(&s,0,true); s.stamp[0]=0; assert(!ledger_valid(&s));
    start(&s); s.halted=1; s.valid=0; s.faults=1; s.rejected=1; s.attempts=1;
    assert(!ledger_valid(&s)); /* Tag zero cannot bypass any lower free slot. */
    puts("engine: all occupancies, precedence, freeze, wrap, serialization PASS");
    printf("engine: %u sorted-free-list oracle transitions PASS\n",words);
}
