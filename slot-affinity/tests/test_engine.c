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
static void op(Ledger *s, unsigned slot, unsigned domain, unsigned alloc)
{
    unsigned tag = slot | (domain << 1);
    hold(s, tag, 12); hold(s, tag | (alloc ? 4u : 8u), 8); hold(s, tag, 8);
}
static Ledger baseline(void)
{
    Ledger s; ledger_init(&s); hold(&s, 0, 20);
    assert(s.phase == OB_TRACK); return s;
}
/* Independent oracle retains immutable assignment separately from occupancy. */
static void explore(Ledger s, const int assigned[2], unsigned occupied, unsigned depth)
{
    if (!depth) return;
    for (unsigned slot=0; slot<2; ++slot)
    for (unsigned domain=0; domain<2; ++domain)
    for (unsigned alloc=0; alloc<2; ++alloc) {
        unsigned bit=1u<<slot, fault=0;
        if (alloc) {
            if (occupied & bit) fault=OB_DUPLICATE;
            else if (assigned[slot]>=0 && assigned[slot]!=(int)domain) fault=OB_BINDING;
        } else {
            if (!(occupied & bit)) fault=OB_ORPHAN;
            else if (assigned[slot]!=(int)domain) fault=OB_BINDING;
        }
        Ledger next=s; op(&next,slot,domain,alloc); ++paths;
        assert(next.faults==fault);
        if (fault) {
            assert(next.active==s.active && next.order==s.order);
            continue;
        }
        int labels[2]={assigned[0],assigned[1]};
        if (alloc) labels[slot]=(int)domain;
        unsigned live=alloc ? occupied|bit : occupied&~bit;
        assert(next.active==live);
        for (unsigned i=0;i<2;++i) {
            assert(!!(next.order&(4u<<i))==(labels[i]>=0));
            if (labels[i]>=0) assert(((next.order>>i)&1u)==(unsigned)labels[i]);
        }
        explore(next,labels,live,depth-1);
    }
}
int main(void)
{
    Ledger s=baseline(); const int labels[2]={-1,-1};
    explore(s,labels,0,7);
    for (unsigned domain=0;domain<2;++domain) {
        s=baseline(); op(&s,0,domain,1); op(&s,1,domain^1u,1);
        assert(!s.halted && s.active==3 && s.queued==1);
        op(&s,0,domain,0); op(&s,1,domain^1u,0);
        op(&s,0,domain,1); op(&s,0,domain,0);
        assert(!s.halted && s.rounds==2);
        op(&s,0,domain^1u,1);
        assert(s.faults==OB_BINDING && s.active==0);
        Ledger frozen=s; hold(&s,15,50); frozen.now=s.now;
        assert(!memcmp(&s,&frozen,sizeof s));
    }
    s=baseline(); hold(&s,12,8); assert(s.faults==OB_INTERFACE && s.rejected==2);
    s=baseline(); ledger_sample(&s,s.now+3,0);
    assert(s.faults==OB_GAP && s.missed==2 && ledger_valid(&s));
    s=baseline(); Ledger same=s; ledger_sample(&s,s.now,15);
    assert(!memcmp(&s,&same,sizeof s));
    s.now=UINT32_MAX; ledger_sample(&s,0,0); assert(!s.faults);
    s=baseline(); s.observed=UINT32_MAX; ledger_sample(&s,s.now+1,0);
    assert(s.faults==OB_EXHAUSTED && s.observed==UINT32_MAX);
    s=baseline(); hold(&s,4,2); hold(&s,0,10); assert(!s.attempts);
    hold(&s,4,8); hold(&s,4,30); assert(s.launched==1);
    s=baseline(); hold(&s,6,8); assert(s.faults==OB_INTERFACE);
    s=baseline(); hold(&s,4,3); hold(&s,12,1); assert(s.faults==OB_INTERFACE);
    s=baseline();
    for (unsigned i=0;i<8;++i) hold(&s,4|((i&1u)<<1),1);
    assert(s.faults==OB_INTERFACE);
    s=baseline(); op(&s,0,0,1);
    s.launched=2147483648u; s.completed=s.rounds=2147483647u;
    s.attempts=UINT32_MAX; s.observed=UINT32_MAX-100u;
    assert(ledger_valid(&s)); op(&s,0,0,0);
    assert(s.faults==OB_EXHAUSTED && s.active==1 && s.completed==2147483647u);
    char out[768]; size_t n=wire_status(out,sizeof out,&s); assert(n);
    assert(!wire_status(out,n-1,&s)); assert(!wire_status(NULL,sizeof out,&s));
    s=baseline(); s.active=1; assert(!ledger_valid(&s));
    printf("engine: %u independent oracle branches; affinity and failure tests PASS\n",paths);
}
