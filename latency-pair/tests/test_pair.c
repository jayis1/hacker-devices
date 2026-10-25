/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "pair.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

static Pair p;
static uint32_t now;
static void tick(uint8_t levels)
{
    pair_sample(&p, ++now, levels);
    assert(pair_valid(&p));
}
static void reset(void)
{
    now = 0;
    pair_init(&p);
    tick(4);
    tick(4);
}
static void pulse(unsigned label, unsigned duration)
{
    tick((uint8_t)(4 | (label << 1)));
    tick((uint8_t)(5 | (label << 1)));
    for (unsigned i=1; i<duration; ++i)
        tick((uint8_t)(5 | (label << 1)));
    tick((uint8_t)(4 | (label << 1)));
}
int main(void)
{
    reset();
    pulse(0,10); pulse(1,15);
    assert(p.pairs == 1 && p.sum_a == 10 && p.sum_b == 15);
    assert(p.b_slower == 1 && p.a_slower == 0 && p.large == 1);
    pulse(1,20); pulse(0,20);
    assert(p.pairs == 2 && p.equal == 1);
    reset();
    pulse(0,10); pulse(0,11); pulse(1,12);
    assert(p.pairs == 0 && p.rejected == 1);
    assert(p.faults & LP_ORDER);
    reset();
    tick(5); tick(7); tick(6);
    assert(p.rejected == 1 && p.pairs == 0 && (p.faults & LP_LABEL));
    reset();
    tick(5); tick(1); tick(4);
    assert(p.rejected == 1 && (p.faults & LP_INVALID));
    reset();
    tick(5); tick(13); tick(4);
    assert(p.rejected == 1 && (p.faults & LP_INHIBIT));
    reset();
    tick(5);
    for (unsigned i=0;i<=LP_MAX_DURATION;++i) tick(5);
    assert(p.rejected == 1 && (p.faults & LP_TIMEOUT));
    tick(4); pulse(0,LP_MAX_DURATION); pulse(1,1);
    assert(p.pairs == 1 && p.sum_a == LP_MAX_DURATION);
    reset(); pulse(0,10);
    now += 3; tick(4);
    assert(p.gaps == 1 && p.rejected == 1 && !p.pending);
    pulse(1,11); assert(!p.pairs);
    reset();
    tick(5);
    now += 10; tick(4);
    assert(p.rejected == 1 && (p.faults & LP_GAP));
    reset(); pulse(0,1);
    for (unsigned i=0;i<LP_PAIR_WAIT;++i) tick(4);
    assert(p.rejected == 1 && !p.pending);
    reset();
    for(unsigned i=0;i<LP_COHORT;++i) { pulse(i&1u,3); pulse(!(i&1u),5); }
    assert(p.pairs == LP_COHORT && p.state == LP_DONE);
    pulse(0,1); pulse(1,2);
    assert(p.pairs == LP_COHORT);
    reset();
    uint32_t before = p.last;
    pair_sample(&p,before,5);
    assert(!p.active);
    pair_init(&p); now = UINT32_MAX-4;
    tick(4); tick(4); pulse(0,10); pulse(1,12);
    assert(p.pairs == 1 && p.sum_a == 10 && p.sum_b == 12);
    reset(); p.rejected = UINT32_MAX;
    tick(5); tick(7);
    assert(p.rejected == UINT32_MAX);
    reset(); pair_tx_failure(&p);
    assert(p.tx_fail == 1 && (p.faults & LP_TX));
    /* Same-sample label setup is not silently accepted. */
    reset(); tick(7); tick(6);
    assert(p.rejected == 1 && (p.faults & LP_LABEL));
    /* High-at-attach must first return to a known idle baseline. */
    pair_init(&p); now=0; tick(5); tick(5); tick(4);
    assert(!p.pending && !p.pairs && !p.rejected);
    pulse(0,1); pulse(1,1); assert(p.equal==1);
    /* Deterministic independent aggregate oracle over varied legal pairs. */
    uint32_t random=1234567;
    for (unsigned run=0;run<100;++run) {
        reset();
        uint32_t suma=0,sumb=0,as=0,bs=0,eq=0,large=0,maximum=0;
        for(unsigned i=0;i<LP_COHORT;++i) {
            random=random*1664525u+1013904223u;
            unsigned a=random%1000u+1;
            random=random*1664525u+1013904223u;
            unsigned b=random%1000u+1;
            if(i&1u) { pulse(1,b); pulse(0,a); }
            else { pulse(0,a); pulse(1,b); }
            suma+=a; sumb+=b;
            as+=a>b; bs+=b>a; eq+=a==b;
            unsigned diff=a>b?a-b:b-a;
            large+=diff>=LP_THRESHOLD;
            if(diff>maximum) maximum=diff;
        }
        assert(p.sum_a==suma && p.sum_b==sumb && p.a_slower==as);
        assert(p.b_slower==bs && p.equal==eq && p.large==large);
        assert(p.max_delta==maximum && !p.rejected && !p.faults);
    }
    reset();
    for(unsigned i=0;i<100000;++i) {
        random=random*1664525u+1013904223u;
        if((random&255u)==0) now+=2;
        tick((uint8_t)(random>>28));
    }
    puts("pair: timing, ordering, rejection, gaps, bounds, wrap, cohort PASS");
}
