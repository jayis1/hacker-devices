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
/* Independent oracle: each resident owns two physical-style debt tokens.
 * Every admission removes one from all OLD residents, then installs two
 * for the newcomer. No global ordinal or timestamp subtraction is used. */
typedef struct { bool live[4]; unsigned debt[4]; } Oracle;
static void enumerate(Ledger state, Oracle oracle, unsigned depth)
{
    if(!depth || state.halted) return;
    for(unsigned action=0;action<8;action++) {
        Ledger next=state; Oracle expected=oracle;
        unsigned tag=action%4,reason=0;
        if(action<4) {
            if(oracle.live[tag]) reason=4;
            else {
                for(unsigned i=0;i<4;i++)
                    if(expected.live[i] && expected.debt[i]) --expected.debt[i];
                expected.live[tag]=true; expected.debt[tag]=2;
            }
        } else {
            if(!oracle.live[tag]) reason=2;
            else if(oracle.debt[tag]) reason=1;
            else expected.live[tag]=false;
        }
        clock_ms=next.now; op(&next,tag,action<4);
        assert(next.faults==reason);
        for(unsigned j=0;j<4;j++) {
            assert(!!(next.active&(1u<<j))==expected.live[j]);
            assert(ledger_eligible(&next,j)==(expected.live[j]&&!expected.debt[j]));
        }
        ++words; enumerate(next,expected,depth-1);
    }
}
int main(void)
{
    Ledger s;
    start(&s); op(&s,0,true); op(&s,0,false);
    assert(s.faults==1 && s.active==1 && !s.completed);
    start(&s); Oracle empty={{false},{0}}; enumerate(s,empty,10);
    /* Inclusive threshold: zero and one subsequent admission reject. */
    for(unsigned later=0;later<4;later++) {
        start(&s); op(&s,0,true);
        for(unsigned i=1;i<=later;i++) op(&s,i,true);
        op(&s,0,false); assert(s.faults==(later<2?1u:0u));
    }
    /* Younger but mature can return before older: this is NOT FIFO. */
    start(&s); for(unsigned i=0;i<4;i++) op(&s,i,true);
    op(&s,1,false); op(&s,1,true); op(&s,2,false); op(&s,2,true);
    op(&s,0,false); op(&s,0,true); assert(!s.halted && s.depth==4);
    /* Waiting and other returns do not buy ordinal credit. */
    op(&s,3,false); hold(&s,0,2000); op(&s,0,false); assert(s.faults==1);
    start(&s); op(&s,2,true); op(&s,2,true); assert(s.faults==4);
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
    assert(!ledger_valid(&s)); /* Early-return finding cannot name a vacancy. */
    printf("engine: %u independent resident-debt oracle transitions PASS\n",words);
}
