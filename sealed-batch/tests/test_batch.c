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
/* Independent oracle uses an explicit accepting/draining phase and a
   membership array, rather than comparing captured size with live depth. */
static void walk(Ledger s,unsigned mask,int draining,unsigned depth)
{
    if(!depth) return;
    for(unsigned action=0;action<8;action++) {
        unsigned tag=action%4,bit=1u<<tag,nextmask=mask,fault=0;
        int nextdrain=draining;
        if(action<4) {
            if(mask&bit) fault=4;
            else if(draining) fault=1;
            else nextmask|=bit;
        } else {
            if(!(mask&bit)) fault=2;
            else { nextmask&=~bit; nextdrain=nextmask!=0; }
        }
        Ledger next=s; op(&next,tag,action<4);
        assert(next.faults==fault && next.active==nextmask);
        for(unsigned j=0;j<4;j++)
            assert(ledger_eligible(&next,j)==(!nextdrain && !(nextmask&(1u<<j))));
        cases++;
        if(!fault) walk(next,nextmask,nextdrain,depth-1);
    }
}
int main(void)
{
    Ledger s=start();
    op(&s,0,1); op(&s,1,1); assert(!s.halted);
    op(&s,0,0); assert(!ledger_eligible(&s,2));
    op(&s,2,1); assert(s.faults==1 && s.active==2);
    walk(start(),0,0,7);
    /* Every filling subset, and every draining subset of a full batch. */
    for(unsigned mask=0;mask<16;mask++) {
        s=start();
        for(unsigned i=0;i<4;i++) if(mask&(1u<<i)) op(&s,i,1);
        for(unsigned i=0;i<4;i++) assert(ledger_eligible(&s,i)==!(mask&(1u<<i)));
        s=start(); for(unsigned i=0;i<4;i++) op(&s,i,1);
        for(unsigned i=0;i<4;i++) if(!(mask&(1u<<i))) op(&s,i,0);
        assert(!s.halted && s.active==mask);
        for(unsigned i=0;i<4;i++) assert(ledger_eligible(&s,i)==(!mask));
    }
    s=start(); for(unsigned i=0;i<3;i++) { op(&s,1,1); op(&s,1,0); }
    assert(!s.halted && s.uses[1]==3 && !s.batch_size);
    assert(!ledger_eligible(NULL,0) && !ledger_eligible(&s,4));
    s=start(); op(&s,0,1); op(&s,1,1); op(&s,0,0); op(&s,1,1);
    assert(s.faults==4); /* Duplicate precedes sealed-batch finding. */
    s=start(); op(&s,2,0); assert(s.faults==2);
    s=start(); hold(&s,12,6); assert(s.faults==8 && s.rejected==2);
    s=start(); ledger_sample(&s,s.now+2,0); assert(s.faults==16 && ledger_valid(&s));
    Ledger frozen=s; hold(&s,15,20); frozen.now=s.now;
    assert(!memcmp(&s,&frozen,sizeof s));
    s=start(); hold(&s,4,3); s.observed=UINT32_MAX; hold(&s,4,1);
    assert(s.faults==32 && !s.attempts && !s.batch_size);
    s=start(); hold(&s,4,3); hold(&s,12,1); assert(s.faults==8);
    s=start(); hold(&s,4,3); hold(&s,5,1); assert(s.faults==8);
    s=start(); Ledger old=s; ledger_sample(&s,s.now,15); assert(!memcmp(&s,&old,sizeof s));
    s.now=UINT32_MAX; hold(&s,0,1); assert(!s.halted);
    char out[768]; assert(wire_status(out,sizeof out,&s));
    assert(!wire_status(out,3,&s) && !wire_status(NULL,100,&s));
    s=start(); s.batch_size=1; assert(!ledger_valid(&s));
    s=start(); op(&s,0,1); s.batch_size=5; assert(!ledger_valid(&s));
    s=start(); op(&s,0,1); s.halted=1; s.valid=0; s.faults=1;
    s.rejected=1; s.attempts++; s.levels=1; assert(!ledger_valid(&s));
    printf("batch: %u independent phase-oracle transitions PASS\n",cases);
}
