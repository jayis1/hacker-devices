/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Independent least-population oracle; tests run against the real qualifier. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static unsigned checked;
static void hold(Ledger *s,unsigned raw,unsigned ticks)
{
    while(ticks--) {
        ledger_sample(s,s->now+1,raw);
        assert(ledger_valid(s));
    }
}
static void op(Ledger *s,unsigned tag,bool alloc)
{
    hold(s,tag,12); hold(s,tag|(alloc?4:8),6); hold(s,tag,6);
}
static Ledger fresh(void)
{
    Ledger s; ledger_init(&s); hold(&s,0,20); return s;
}
static bool oracle(const bool live[4],unsigned tag)
{
    if(live[tag]) return false;
    unsigned counts[2]={0,0};
    for(unsigned i=0;i<4;i++) counts[i/2]+=live[i];
    unsigned minimum=counts[0]<counts[1]?counts[0]:counts[1];
    return counts[tag/2]==minimum;
}
static void walk(Ledger state,bool live[4],unsigned remaining)
{
    if(!remaining) return;
    for(unsigned action=0;action<8;action++) {
        unsigned tag=action%4,reason=0;
        bool next_live[4];
        for(unsigned i=0;i<4;i++) next_live[i]=live[i];
        if(action<4) {
            if(live[tag]) reason=4;
            else if(!oracle(live,tag)) reason=1;
            else next_live[tag]=true;
        } else {
            if(!live[tag]) reason=2;
            else next_live[tag]=false;
        }
        Ledger next=state;
        op(&next,tag,action<4);
        assert(next.faults==reason);
        for(unsigned i=0;i<4;i++) {
            assert(!!(next.active&(1u<<i))==next_live[i]);
            assert(ledger_eligible(&next,i)==oracle(next_live,i));
        }
        char wire[768]; assert(wire_status(wire,sizeof wire,&next));
        checked++;
        if(!reason) walk(next,next_live,remaining-1);
    }
}
int main(void)
{
    Ledger s=fresh();
    op(&s,0,true);
    assert(!ledger_eligible(&s,1));
    assert(ledger_eligible(&s,2) && ledger_eligible(&s,3));
    bool empty[4]={false}; s=fresh(); walk(s,empty,8);
    /* Every occupancy subset is reachable by legal fill then arbitrary drain. */
    for(unsigned mask=0;mask<16;mask++) {
        s=fresh(); unsigned order[4]={0,2,1,3};
        for(unsigned i=0;i<4;i++) op(&s,order[i],true);
        for(unsigned i=0;i<4;i++) if(!(mask&(1u<<i))) op(&s,i,false);
        assert(!s.halted && s.active==mask);
    }
    /* Returns do not force eviction or migration of the two incumbents. */
    s=fresh(); op(&s,0,true); op(&s,2,true); op(&s,1,true); op(&s,2,false);
    assert(s.active==3 && !s.halted);
    op(&s,3,true); assert(!s.halted);
    /* No historical balancing: full drain erases any preference. */
    s=fresh(); for(unsigned i=0;i<100;i++) { op(&s,3,true); op(&s,3,false); }
    assert(!s.halted && s.uses[3]==100);
    printf("spread: %u independent operation transitions, all occupancy subsets, repeated reuse PASS\n",checked);
    return 0;
}
