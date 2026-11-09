/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
static uint32_t clock_ms;
static unsigned words;
static void hold(Ledger *s,unsigned raw,unsigned n)
{
    while(n--) { ledger_sample(s,++clock_ms,raw); assert(ledger_valid(s)); }
}
static void start(Ledger *s) { clock_ms=0; ledger_init(s); hold(s,0,20); }
static void op(Ledger *s,unsigned tag,bool alloc)
{
    hold(s,tag,12); hold(s,tag|(alloc?4u:8u),6); hold(s,tag,6);
}
/* Independent literal free-list oracle. Head is the only admissible tag. */
typedef struct { unsigned list[4], count; } Oracle;
static bool vacant(const Oracle *o,unsigned tag)
{
    for(unsigned i=0;i<o->count;i++) if(o->list[i]==tag) return true;
    return false;
}
static void enumerate(Ledger s,Oracle o,unsigned depth)
{
    if(!depth || s.halted) return;
    for(unsigned action=0;action<8;action++) {
        Ledger next=s; Oracle want=o;
        unsigned tag=action%4,reason=0;
        if(action<4) {
            if(!vacant(&o,tag)) reason=4;
            else if(o.list[0]!=tag) reason=1;
            else {
                for(unsigned i=1;i<want.count;i++) want.list[i-1]=want.list[i];
                want.count--;
            }
        } else if(vacant(&o,tag)) reason=2;
        else {
            for(unsigned i=want.count;i>0;i--) want.list[i]=want.list[i-1];
            want.list[0]=tag; want.count++;
        }
        clock_ms=next.now; op(&next,tag,action<4);
        assert(next.faults==reason);
        for(unsigned i=0;i<4;i++) {
            assert(!!(next.active&(1u<<i))==!vacant(&want,i));
            assert(ledger_eligible(&next,i)==(want.count && want.list[0]==i));
        }
        words++; enumerate(next,want,depth-1);
    }
}
int main(void)
{
    Ledger s; start(&s);
    op(&s,0,true); op(&s,1,true); op(&s,0,false); op(&s,1,false);
    assert(ledger_eligible(&s,1)); /* distinguishes numeric minimum */
    op(&s,1,true); op(&s,0,true); assert(!s.halted);
    start(&s); Oracle initial={{0,1,2,3},4}; enumerate(s,initial,10);
    /* Every return ordering is legal, independently of admission order. */
    for(unsigned a=0;a<4;a++) for(unsigned b=0;b<4;b++)
    for(unsigned c=0;c<4;c++) for(unsigned d=0;d<4;d++) {
        if(a==b||a==c||a==d||b==c||b==d||c==d) continue;
        start(&s); for(unsigned i=0;i<4;i++) op(&s,i,true);
        op(&s,a,false); op(&s,b,false); op(&s,c,false); op(&s,d,false);
        op(&s,d,true); op(&s,c,true); op(&s,b,true); op(&s,a,true);
        assert(!s.halted && s.active==15);
    }
    start(&s); op(&s,0,true); op(&s,0,true); assert(s.faults==4);
    start(&s); op(&s,2,false); assert(s.faults==2);
    start(&s); hold(&s,12,6); assert(s.faults==8 && s.rejected==2);
    start(&s); ledger_sample(&s,s.now+2,0); assert(s.faults==16 && ledger_valid(&s));
    Ledger frozen=s; clock_ms=s.now; hold(&s,15,20); frozen.now=s.now;
    assert(!memcmp(&s,&frozen,sizeof s));
    start(&s); hold(&s,4,3); s.observed=UINT32_MAX; hold(&s,4,1);
    assert(s.faults==32 && !s.attempts);
    start(&s); hold(&s,4,3); hold(&s,12,1); assert(s.faults==8);
    start(&s); hold(&s,4,3); hold(&s,5,1); assert(s.faults==8);
    start(&s); Ledger old=s; ledger_sample(&s,s.now,15); assert(!memcmp(&s,&old,sizeof s));
    s.now=UINT32_MAX; clock_ms=UINT32_MAX; hold(&s,0,1); assert(!s.halted);
    char out[768]; assert(wire_status(out,sizeof out,&s));
    assert(!wire_status(out,3,&s) && !wire_status(NULL,100,&s));
    printf("PASS: %u free-stack oracle transitions; 24 return permutations; fault boundaries\n",words);
}
