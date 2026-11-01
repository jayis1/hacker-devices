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
/* Independent oracle: retain the last two admitted identities as a queue.
 * No ordinal subtraction and no use counters implement eligibility here. */
static void enumerate(Ledger state, int older, int newer, unsigned depth)
{
    if (!depth || state.halted) return;
    for (unsigned action=0;action<8;action++) {
        Ledger next=state;
        unsigned tag=action%4, reason=0, mask=state.active;
        int a=older,b=newer;
        if (action<4) {
            if(mask&(1u<<tag)) reason=4;
            else if((int)tag==a || (int)tag==b) reason=1;
            else {mask|=1u<<tag; a=b; b=(int)tag;}
        } else {
            if(!(mask&(1u<<tag))) reason=2;
            else mask&=~(1u<<tag);
        }
        clock_ms=next.now; op(&next,tag,action<4);
        assert(next.faults==reason && next.active==mask);
        if(!reason) for(unsigned j=0;j<4;j++)
            assert(ledger_eligible(&next,j)==(!(mask&(1u<<j)) && (int)j!=a && (int)j!=b));
        words++; enumerate(next,a,b,depth-1);
    }
}
int main(void)
{
    Ledger s; start(&s); enumerate(s,-1,-1,8);
    for(unsigned tag=0;tag<4;tag++) {
        start(&s); op(&s,tag,true); op(&s,tag,false);
        hold(&s,tag,1000); op(&s,tag,true);
        assert(s.faults==1 && s.uses[tag]==1);
        start(&s); op(&s,tag,true); op(&s,tag,false);
        op(&s,(tag+1)%4,true); op(&s,(tag+1)%4,false);
        op(&s,tag,true); assert(s.faults==1);
        start(&s);
        for(unsigned round=0;round<6;round++) for(unsigned j=0;j<3;j++) {
            unsigned t=(tag+j)%4; op(&s,t,true); op(&s,t,false);
        }
        assert(!s.halted && s.launched==18 && !s.uses[(tag+3)%4]);
    }
    start(&s); op(&s,0,true); op(&s,1,true); op(&s,2,true);
    op(&s,0,true); assert(s.faults==4); /* Occupancy still wins. */
    start(&s); op(&s,0,true); op(&s,0,false); op(&s,1,true);
    op(&s,2,true); op(&s,0,true); assert(!s.halted && s.stamp[0]==4);
    start(&s); hold(&s,12,6); assert(s.faults==8 && s.rejected==2);
    start(&s); op(&s,3,false); assert(s.faults==2);
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
    start(&s);
    for(unsigned j=0;j<5;j++) {op(&s,j%4,true); op(&s,j%4,false);}
    s.stamp[2]=1; assert(!ledger_valid(&s));
    assert(!ledger_eligible(NULL,0) && !ledger_eligible(&s,4));
    printf("engine: %u independent two-entry queue transitions PASS\n",words);
}
