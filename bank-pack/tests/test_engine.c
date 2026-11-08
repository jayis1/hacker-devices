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
/* Independent oracle: compare the destination bank population to the
 * largest non-full bank population. No production masks or XOR pairing. */
typedef struct { bool live[4]; } Oracle;
static bool allowed(const Oracle *o, unsigned tag)
{
    if(o->live[tag]) return false;
    unsigned count[2]={0,0}, best=0;
    for(unsigned i=0;i<4;i++) count[i/2]+=o->live[i];
    for(unsigned b=0;b<2;b++)
        if(count[b]<2 && count[b]>best) best=count[b];
    return count[tag/2]==best;
}
static void enumerate(Ledger state, Oracle oracle, unsigned depth)
{
    if(!depth || state.halted) return;
    for(unsigned action=0;action<8;action++) {
        Ledger next=state; Oracle expected=oracle;
        unsigned tag=action%4, reason=0;
        if(action<4) {
            if(oracle.live[tag]) reason=4;
            else if(!allowed(&oracle,tag)) reason=1;
            else expected.live[tag]=true;
        } else {
            if(!oracle.live[tag]) reason=2;
            else expected.live[tag]=false;
        }
        clock_ms=next.now; op(&next,tag,action<4);
        assert(next.faults==reason);
        for(unsigned j=0;j<4;j++) {
            assert(!!(next.active&(1u<<j))==expected.live[j]);
            assert(ledger_eligible(&next,j)==allowed(&expected,j));
        }
        words++; enumerate(next,expected,depth-1);
    }
}
int main(void)
{
    Ledger s;
    start(&s);
    for(unsigned i=0;i<4;i++) assert(ledger_eligible(&s,i));
    Oracle empty={{false}}; enumerate(s,empty,8);
    /* Releases may create two partial banks: this is not a state invariant. */
    start(&s); for(unsigned i=0;i<4;i++) op(&s,i,true);
    op(&s,0,false); op(&s,2,false);
    assert(ledger_eligible(&s,0) && ledger_eligible(&s,2));
    op(&s,2,true); op(&s,0,true); assert(!s.halted);
    /* Either bank can be opened first; its sibling must follow. */
    for(unsigned i=0;i<4;i++) {
        start(&s); op(&s,i,true);
        assert(ledger_eligible(&s,i^1u));
        op(&s,i^2u,true); assert(s.faults==1 && s.active==(1u<<i));
    }
    /* Complete drain and immediate reuse impose no recency obligation. */
    start(&s); op(&s,3,true); op(&s,3,false); op(&s,3,true);
    assert(!s.halted && s.uses[3]==2);
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
    assert(!ledger_valid(&s)); /* Empty pool cannot create packing fault. */
    puts("engine: qualification, precedence, freeze, wrap, serialization PASS");
    printf("engine: %u independent population-oracle transitions PASS\n",words);
}
