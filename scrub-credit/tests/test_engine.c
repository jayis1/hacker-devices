/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned paths;
static void hold(Ledger *s, unsigned raw, unsigned n)
{
    while (n--) {
        ledger_sample(s, s->now + 1, (uint8_t)raw);
        assert(ledger_valid(s));
    }
}
/* Operations: 0=allocate, 1=return, 2=scrub completion. */
static void op(Ledger *s, unsigned slot, unsigned operation)
{
    unsigned tag=slot | (operation==2 ? 2u : 0u);
    hold(s,tag,12);
    hold(s,tag | (operation==0 ? 4u : 8u),8);
    hold(s,tag,8);
}
static Ledger baseline(void)
{
    Ledger s; ledger_init(&s); hold(&s,0,20);
    assert(s.phase==OB_TRACK); return s;
}
/* Independent per-slot automaton: dirty=0, clean=1, occupied=2. */
static void explore(Ledger s, const unsigned state[2], unsigned depth)
{
    if (!depth) return;
    for (unsigned slot=0;slot<2;++slot)
    for (unsigned operation=0;operation<3;++operation) {
        unsigned fault=0, next_state=state[slot];
        switch (operation) {
        case 0:
            if (state[slot]==2) fault=OB_DUPLICATE;
            else if (state[slot]==0) fault=OB_CREDIT;
            else next_state=2;
            break;
        case 1:
            if (state[slot]!=2) fault=OB_ORPHAN;
            else next_state=0;
            break;
        default:
            if (state[slot]!=0) fault=OB_CREDIT;
            else next_state=1;
            break;
        }
        Ledger next=s; op(&next,slot,operation); ++paths;
        assert(next.faults==fault);
        if (fault) {
            assert(next.active==s.active && next.clean==s.clean);
            continue;
        }
        unsigned states[2]={state[0],state[1]}; states[slot]=next_state;
        for (unsigned i=0;i<2;++i) {
            assert(!!(next.active&(1u<<i))==(states[i]==2));
            assert(!!(next.clean&(1u<<i))==(states[i]==1));
        }
        explore(next,states,depth-1);
    }
}
int main(void)
{
    Ledger s=baseline(); const unsigned states[2]={0,0};
    explore(s,states,9);
    /* Credits are neither pooled, retained across use, nor stackable. */
    s=baseline(); op(&s,0,2); op(&s,1,0); assert(s.faults==OB_CREDIT);
    s=baseline(); op(&s,0,2); op(&s,0,2); assert(s.faults==OB_CREDIT && s.clean==1);
    s=baseline(); op(&s,0,2); op(&s,0,0); op(&s,0,2);
    assert(s.faults==OB_CREDIT && s.active==1);
    s=baseline(); op(&s,0,2); op(&s,0,0); op(&s,0,1); op(&s,0,0);
    assert(s.faults==OB_CREDIT && !s.active && !s.clean);
    for (unsigned first=0;first<2;++first) {
        s=baseline(); op(&s,0,2); op(&s,1,2); op(&s,0,0); op(&s,1,0);
        assert(s.active==3 && s.peak==2 && s.scrubbed==2);
        op(&s,first,1); op(&s,first,2); op(&s,first,0);
        op(&s,first^1u,1); op(&s,first,1);
        assert(!s.halted && s.rounds==1 && s.completed==3);
    }
    s=baseline(); hold(&s,12,8); assert(s.faults==OB_INTERFACE && s.rejected==2);
    s=baseline(); hold(&s,2,12); hold(&s,6,8); assert(s.faults==OB_INTERFACE);
    s=baseline(); ledger_sample(&s,s.now+3,0);
    assert(s.faults==OB_GAP && s.missed==2 && ledger_valid(&s));
    Ledger frozen=s; hold(&s,15,50); frozen.now=s.now;
    assert(!memcmp(&s,&frozen,sizeof s));
    s=baseline(); Ledger same=s; ledger_sample(&s,s.now,15);
    assert(!memcmp(&s,&same,sizeof s));
    s.now=UINT32_MAX; ledger_sample(&s,0,0); assert(!s.faults);
    s=baseline(); s.observed=UINT32_MAX; ledger_sample(&s,s.now+1,0);
    assert(s.faults==OB_EXHAUSTED && s.observed==UINT32_MAX);
    s=baseline(); hold(&s,8,2); hold(&s,0,10); assert(!s.attempts);
    op(&s,0,2); hold(&s,0,12); hold(&s,4,40); assert(s.launched==1);
    s=baseline(); hold(&s,4,3); hold(&s,12,1); assert(s.faults==OB_INTERFACE);
    s=baseline();
    for (unsigned i=0;i<8;++i) hold(&s,4|((i&1u)<<1),1);
    assert(s.faults==OB_INTERFACE);
    /* Attempt overflow on a scrub must not grant a credit. */
    s=baseline();
    s.launched=s.completed=s.scrubbed=s.rounds=1431655765u;
    s.attempts=UINT32_MAX; s.observed=UINT32_MAX-100u; s.peak=1;
    assert(ledger_valid(&s)); op(&s,0,2);
    assert(s.faults==OB_EXHAUSTED && s.clean==0 && s.scrubbed==1431655765u);
    char out[768]; size_t n=wire_status(out,sizeof out,&s); assert(n);
    assert(!wire_status(out,n-1,&s)); assert(!wire_status(NULL,sizeof out,&s));
    s=baseline(); s.active=1; assert(!ledger_valid(&s));
    printf("engine: %u independent oracle branches; scrub and failure tests PASS\n",paths);
}
