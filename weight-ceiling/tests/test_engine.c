/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include "drivers/crc16.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void hold(Ledger *s, unsigned raw, unsigned n)
{
    while (n--) {
        ledger_sample(s, s->now + 1, (uint8_t)raw);
        assert(ledger_valid(s));
    }
}
static void operation(Ledger *s, unsigned tag, bool allocate)
{
    hold(s, tag, 12);
    hold(s, tag | (allocate ? 4 : 8), 8);
    hold(s, tag, 8);
}
static Ledger baseline(void)
{
    Ledger s;
    ledger_init(&s);
    hold(&s, 0, 20);
    assert(s.phase == WC_TRACK);
    return s;
}
/* Independent array oracle with explicit costs, not firmware bit arithmetic. */
static unsigned paths;
static void explore(Ledger s, const bool occupied[4], unsigned depth)
{
    if (!depth) return;
    const unsigned costs[4] = {1,2,3,4};
    unsigned used = 0;
    for (unsigned i=0;i<4;++i) if (occupied[i]) used += costs[i];
    for (unsigned tag=0;tag<4;++tag)
    for (unsigned alloc=0;alloc<2;++alloc) {
        Ledger next=s;
        unsigned fault=0;
        if (alloc) {
            if (occupied[tag]) fault=WC_DUPLICATE;
            else if (used+costs[tag]>5) fault=WC_BUDGET;
        } else if (!occupied[tag]) fault=WC_ORPHAN;
        operation(&next,tag,alloc);
        assert(next.faults==fault);
        ++paths;
        if (!fault) {
            bool copy[4]; memcpy(copy,occupied,sizeof copy); copy[tag]=alloc;
            unsigned sum=0;
            for (unsigned i=0;i<4;++i) {
                assert(!!(next.active&(1u<<i))==copy[i]);
                if(copy[i]) sum+=costs[i];
            }
            assert(next.load==sum);
            explore(next,copy,depth-1);
        } else assert(next.active==s.active && next.load==s.load);
    }
}
int main(void)
{
    Ledger s=baseline();
    /* Exact ceiling accepted; release either identity, then reuse credit. */
    operation(&s,0,true); operation(&s,3,true);
    assert(!s.faults && s.load==5 && s.peak==5);
    operation(&s,0,false); operation(&s,0,true);
    assert(!s.faults && s.load==5);
    operation(&s,1,true);
    assert(s.faults==WC_BUDGET && s.active==9 && s.load==5);
    const bool empty[4]={false,false,false,false};
    explore(baseline(),empty,7);
    /* Equal occupancy does not mean equal weighted use. */
    s=baseline(); operation(&s,1,true); operation(&s,2,true);
    assert(!s.faults && s.load==5);
    s=baseline(); operation(&s,2,true); operation(&s,3,true);
    assert(s.faults==WC_BUDGET && s.load==3);
    s=baseline(); operation(&s,3,true); operation(&s,3,true);
    assert(s.faults==WC_DUPLICATE); /* duplicate precedence over budget */
    Ledger frozen=s; hold(&s,15,50); frozen.now=s.now;
    assert(!memcmp(&s,&frozen,sizeof s));
    s=baseline(); hold(&s,12,8);
    assert(s.faults==WC_INTERFACE && s.rejected==2);
    s=baseline(); ledger_sample(&s,s.now+3,0);
    assert(s.faults==WC_GAP && s.missed==2 && ledger_valid(&s));
    s=baseline(); Ledger same=s; ledger_sample(&s,s.now,15);
    assert(!memcmp(&s,&same,sizeof s));
    s.now=UINT32_MAX; ledger_sample(&s,0,0); assert(!s.faults);
    s=baseline(); s.observed=UINT32_MAX; ledger_sample(&s,s.now+1,0);
    assert(s.faults==WC_EXHAUSTED && s.observed==UINT32_MAX);
    s=baseline(); hold(&s,4,2); hold(&s,0,10); assert(!s.attempts);
    hold(&s,4,8); hold(&s,4,30); assert(s.launched==1);
    s=baseline(); hold(&s,6,8); assert(s.faults==WC_INTERFACE);
    s=baseline(); hold(&s,4,3); hold(&s,12,1);
    assert(s.faults==WC_INTERFACE && s.rejected==1);
    s=baseline();
    for(unsigned i=0;i<8;++i) hold(&s,4|((i&1u)<<1),1);
    assert(s.faults==WC_INTERFACE);
    s=baseline(); operation(&s,0,true);
    s.launched=2147483648u; s.completed=s.rounds=2147483647u;
    s.attempts=UINT32_MAX; s.observed=UINT32_MAX-100u;
    assert(ledger_valid(&s)); operation(&s,0,false);
    assert(s.faults==WC_EXHAUSTED && s.active==1 && s.load==1);
    assert(s.completed==2147483647u && s.attempts==UINT32_MAX);
    char out[768]; size_t n=wire_status(out,sizeof out,&s); assert(n);
    assert(!wire_status(out,n-1,&s)); assert(!wire_status(NULL,sizeof out,&s));
    assert(wire_boot(out,sizeof out,0)); assert(wire_policy(out,sizeof out));
    uint16_t crc; assert(crc16_buffer("123456789",9,&crc) && crc==0x29b1);
    s=baseline(); s.active=1; assert(!ledger_valid(&s));
    printf("engine: %u weighted array-oracle branches; boundaries and faults PASS\n",paths);
}
