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
/* Independent oracle: an explicit array of booleans and descending scan.
 * No admission stamps and no production mask-shift eligibility formula. */
static void enumerate(Ledger state, const bool live[4], unsigned depth)
{
    if (!depth || state.halted) return;
    for (unsigned action=0;action<8;action++) {
        Ledger next=state;
        bool expected[4]; memcpy(expected,live,sizeof expected);
        unsigned tag=action%4, reason=0;
        int highest=-1;
        for(int i=0;i<4;i++) if(live[i]) highest=i;
        if (action<4) {
            if(live[tag]) reason=4;
            else expected[tag]=true;
        } else {
            if(!live[tag]) reason=2;
            else if((int)tag!=highest) reason=1;
            else expected[tag]=false;
        }
        clock_ms=next.now; op(&next,tag,action<4);
        assert(next.faults==reason);
        for(unsigned j=0;j<4;j++) assert(!!(next.active&(1u<<j))==expected[j]);
        words++; enumerate(next,expected,depth-1);
    }
}
int main(void)
{
    Ledger s;
    /* A later lower-rank admission must NOT become the next return. */
    start(&s); op(&s,3,true); op(&s,0,true); op(&s,3,false);
    assert(!s.halted && s.active==1);
    op(&s,0,false); op(&s,0,true); op(&s,0,false);
    assert(!s.halted && s.completed==3); /* Immediate reuse is allowed. */
    start(&s); const bool empty[4]={false,false,false,false}; enumerate(s,empty,8);
    for(unsigned mask=1;mask<16;mask++) for(unsigned release=0;release<4;release++) {
        start(&s);
        for(int i=3;i>=0;i--) if(mask&(1u<<i)) op(&s,(unsigned)i,true);
        int highest=3; while(!(mask&(1u<<highest))) highest--;
        op(&s,release,false);
        unsigned reason=!(mask&(1u<<release))?2u:((int)release==highest?0u:1u);
        assert(s.faults==reason);
    }
    start(&s); op(&s,1,true); op(&s,3,true); op(&s,1,false);
    assert(s.faults==1 && s.active==10 && !s.completed && s.rejected==1);
    start(&s); op(&s,2,true); op(&s,2,true); assert(s.faults==4);
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
    start(&s); op(&s,0,true);
    s.halted=1; s.valid=0; s.faults=UQ_RANK; s.rejected=1; s.attempts++;
    assert(!ledger_valid(&s)); /* No higher live rank can justify this fault. */
    printf("engine: %u independent descending-rank oracle transitions PASS\n",words);
}
