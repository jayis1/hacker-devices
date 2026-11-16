/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include <assert.h>
#include <stdio.h>
/* An independent ordinary-capacity oracle, with no numeric ordering. */
static unsigned cases;
static void hold(Ledger *s,unsigned raw,unsigned count)
{
    while(count--) { ledger_sample(s,s->now+1,raw); assert(ledger_valid(s)); }
}
static void op(Ledger *s,unsigned tag,int alloc)
{
    hold(s,tag,12); hold(s,tag|(alloc?4:8),6); hold(s,tag,6);
}
static void walk(Ledger s,unsigned occupied[4],unsigned depth)
{
    if(!depth) return;
    for(unsigned action=0;action<8;action++) {
        unsigned tag=action%4,copy[4],fault=0;
        for(unsigned j=0;j<4;j++) copy[j]=occupied[j];
        if(action<4) {
            if(copy[tag]) fault=4;
            else if(tag==3 && (!copy[0] || !copy[1] || !copy[2])) fault=1;
            else copy[tag]=1;
        } else {
            if(!copy[tag]) fault=2;
            else copy[tag]=0;
        }
        Ledger next=s; op(&next,tag,action<4);
        assert(next.faults==fault);
        for(unsigned j=0;j<4;j++) {
            assert(!!(next.active&(1u<<j))==copy[j]);
            int expected=!copy[j] && (j!=3 || (copy[0]&&copy[1]&&copy[2]));
            assert(ledger_eligible(&next,j)==expected);
        }
        cases++;
        if(!fault) walk(next,copy,depth-1);
    }
}
int main(void)
{
    Ledger s; ledger_init(&s); hold(&s,0,20);
    unsigned empty[4]={0};
    assert(ledger_eligible(&s,2)); /* Fails against inherited minimum policy. */
    walk(s,empty,7);
    /* Reserve may remain live after ordinary returns; no forced eviction. */
    op(&s,2,1); op(&s,0,1); op(&s,1,1); op(&s,3,1);
    op(&s,0,0); op(&s,1,0); op(&s,2,0);
    assert(!s.halted && s.active==8);
    op(&s,3,0); op(&s,3,1);
    assert(s.faults==1 && !s.active);
    printf("reserve: %u independent operation transitions PASS\n",cases);
}
