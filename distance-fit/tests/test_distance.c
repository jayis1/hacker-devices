/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static unsigned cases;
static void hold(Ledger *s,unsigned raw,unsigned count)
{
    while(count--) { ledger_sample(s,s->now+1,raw); assert(ledger_valid(s)); }
}
static void op(Ledger *s,unsigned tag,int alloc)
{
    hold(s,tag,12); hold(s,tag|(alloc?4:8),6); hold(s,tag,6);
}
static Ledger start(void)
{
    Ledger s; ledger_init(&s); hold(&s,0,20); return s;
}
/* Independent discrete distance transform: expand occupied wavefronts one
   step at a time along a non-wrapping line. Last reached free cells win. */
static unsigned oracle(unsigned mask)
{
    unsigned reached=mask, last=0;
    if(!mask) return 15;
    while(reached!=15) {
        unsigned next=(reached|(reached<<1)|(reached>>1))&15;
        last=next&~reached;
        reached=next;
    }
    return last;
}
static void walk(Ledger s,unsigned mask,unsigned depth)
{
    if(!depth) return;
    for(unsigned action=0;action<8;action++) {
        unsigned tag=action%4,bit=1u<<tag,nextmask=mask,fault=0;
        if(action<4) {
            if(mask&bit) fault=4;
            else if(!(oracle(mask)&bit)) fault=1;
            else nextmask|=bit;
        } else {
            if(!(mask&bit)) fault=2;
            else nextmask&=~bit;
        }
        Ledger next=s; op(&next,tag,action<4);
        assert(next.faults==fault && next.active==nextmask);
        for(unsigned j=0;j<4;j++)
            assert(ledger_eligible(&next,j)==!!(oracle(nextmask)&(1u<<j)));
        cases++;
        if(!fault) walk(next,nextmask,depth-1);
    }
}
int main(void)
{
    Ledger s=start();
    assert(ledger_eligible(&s,1)); /* RED against endpoint-only inheritance. */
    assert(ledger_eligible(&s,2));
    walk(s,0,7);
    /* Full occupancy then arbitrary return reaches every retained subset. */
    const unsigned order[]={0,3,1,2};
    for(unsigned mask=0;mask<16;mask++) {
        s=start(); for(unsigned i=0;i<4;i++) op(&s,order[i],1);
        for(unsigned i=0;i<4;i++) if(!(mask&(1u<<i))) op(&s,i,0);
        assert(!s.halted && s.active==mask);
        for(unsigned i=0;i<4;i++) assert(ledger_eligible(&s,i)==!!(oracle(mask)&(1u<<i)));
    }
    s=start(); op(&s,0,1); op(&s,1,1);
    assert(s.faults==1 && s.active==1 && s.uses[1]==0);
    s=start(); op(&s,1,1); assert(ledger_eligible(&s,3));
    assert(!ledger_eligible(&s,0) && !ledger_eligible(&s,2));
    op(&s,3,1); assert(ledger_eligible(&s,0) && ledger_eligible(&s,2));
    /* A return can revoke eligibility without any allocation. */
    s=start(); op(&s,0,1); op(&s,3,1);
    assert(ledger_eligible(&s,1)); op(&s,3,0);
    assert(!ledger_eligible(&s,1) && ledger_eligible(&s,3));
    s=start(); for(unsigned i=0;i<3;i++) { op(&s,1,1); op(&s,1,0); }
    assert(!s.halted && s.uses[1]==3);
    assert(!ledger_eligible(NULL,0) && !ledger_eligible(&s,4));
    s=start(); op(&s,0,1); op(&s,0,1); assert(s.faults==4);
    s=start(); op(&s,2,0); assert(s.faults==2);
    s=start(); hold(&s,12,6); assert(s.faults==8 && s.rejected==2);
    s=start(); ledger_sample(&s,s.now+2,0); assert(s.faults==16 && ledger_valid(&s));
    Ledger frozen=s; hold(&s,15,20); frozen.now=s.now;
    assert(!memcmp(&s,&frozen,sizeof s));
    s=start(); hold(&s,4,3); s.observed=UINT32_MAX; hold(&s,4,1);
    assert(s.faults==32 && !s.attempts && !s.stamp[0]);
    s=start(); hold(&s,4,3); hold(&s,12,1); assert(s.faults==8);
    s=start(); hold(&s,4,3); hold(&s,5,1); assert(s.faults==8);
    s=start(); Ledger old=s; ledger_sample(&s,s.now,15); assert(!memcmp(&s,&old,sizeof s));
    s.now=UINT32_MAX; hold(&s,0,1); assert(!s.halted);
    char out[768]; assert(wire_status(out,sizeof out,&s));
    assert(!wire_status(out,3,&s) && !wire_status(NULL,100,&s));
    s=start(); op(&s,0,1); s.stamp[0]=0; assert(!ledger_valid(&s));
    s=start(); s.halted=1; s.valid=0; s.faults=1; s.rejected=1; s.attempts=1;
    assert(!ledger_valid(&s)); /* No distance preference in an empty pool. */
    s=start(); s.halted=1; s.valid=0; s.faults=4; s.rejected=1; s.attempts=1;
    assert(!ledger_valid(&s));
    s=start(); op(&s,0,1); s.halted=1; s.valid=0; s.faults=2;
    s.rejected=1; s.attempts++;
    assert(!ledger_valid(&s));
    printf("distance: %u wavefront-oracle transitions; all masks/faults PASS\n",cases);
}
