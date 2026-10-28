/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Latch c;
static uint32_t t;
static void step(uint8_t raw, unsigned n)
{
    while (n--) {
        latch_sample(&c, ++t, raw);
        assert(latch_valid(&c));
    }
}
static void baseline(uint8_t code)
{
    latch_init(&c);
    t=0;
    latch_sample(&c,t,code);
    step(code,20);
}
static void boundaries(void)
{
    /* Raw setup interruption: four intervals fails, five passes. */
    baseline(1);
    step(3,1);
    step(9,4);
    assert(c.setup==1 && c.active);
    baseline(1);
    step(3,1);
    step(1,1);
    step(9,4);
    assert(c.setup==0 && c.active);

    baseline(1);
    step(9,4);
    step(9,9);
    assert(c.active && c.age==9);
    step(11,1);
    assert(c.bad==1 && c.hold==1 && !c.active);
    baseline(1);
    step(9,14);
    step(11,1);
    assert(c.good==1 && c.hold==0);

    /* A new rising edge at endpoint wins over completion. */
    baseline(1);
    step(9,4);
    step(1,6);
    step(9,4);
    assert(c.collisions==1 && c.aborted==2 && !c.completed);
    baseline(1);
    step(9,4);
    step(1,7);
    step(9,4);
    assert(c.completed==1 && c.starts==2 && c.active);

    baseline(9);
    assert(c.starts==0);
    step(1,4);
    step(9,14);
    assert(c.good==1);

    /* Overflow occurs after the speculative copy abandoned a trial.
     * Rollback must retain the real pending start, then abandon it once. */
    baseline(1);
    step(9,4);
    c.starts=UINT32_MAX;
    c.completed=UINT32_MAX-1;
    c.good=UINT32_MAX-1;
    c.bins[1]=UINT32_MAX-1;
    assert(latch_valid(&c));
    step(1,4);
    step(9,4);
    assert(c.halted && c.aborted==1 && c.collisions==0);
    assert(c.completed==UINT32_MAX-1 && c.starts==UINT32_MAX);

    baseline(1);
    step(9,4);
    c.gaps=1; c.missed=UINT32_MAX; c.faults|=PL_GAP;
    latch_sample(&c,t+=3,9);
    assert(c.halted && c.aborted==1 && c.gaps==1 && c.missed==UINT32_MAX);
    assert(latch_valid(&c));
    baseline(1);
    step(9,4);
    latch_sample(&c,t+=8,9);
    assert(c.missed==7);
}

static void randomized(void)
{
    /* Deterministic adversarial input: mix held states, glitches and gaps. */
    uint32_t rng=12345;
    uint8_t raw=0;
    baseline(0);
    for (unsigned i=0;i<100000;++i) {
        rng=rng*1664525u+1013904223u;
        if ((rng & 15u)==0)
            raw=(uint8_t)((rng>>16)&15u);
        uint32_t delta=(rng%997u)==0 ? 4u : 1u;
        latch_sample(&c,t+=delta,raw);
        assert(latch_valid(&c));
    }
}

int main(void)
{
    char buf[768];
    baseline(1);
    step(9,4);
    assert(c.starts==1 && c.active);
    step(9,10);
    assert(c.good==1 && c.completed==1 && !c.active);
    assert(c.bins[1]==1);
    step(9,20);
    assert(c.starts==1);
    baseline(0);
    step(8,14);
    assert(c.bad==1 && c.parity==1 && c.good==0);
    baseline(1);
    step(9,4);
    step(11,1); /* Raw excursion too brief to qualify still violates hold. */
    step(9,9);
    assert(c.hold==1 && c.bad==1);
    baseline(1);
    step(8,4); /* Data changes with strobe; setup and parity both fail. */
    step(8,10);
    assert(c.setup==1 && c.parity==1 && c.bad==1);
    baseline(1);
    step(9,4);
    latch_sample(&c,t+=8,9);
    assert(c.aborted==1 && c.gaps==1 && !c.active);
    step(9,20);
    assert(c.starts==1); /* No invented post-gap edge. */
    step(1,10);
    step(9,14);
    assert(c.starts==2 && c.good==1);
    baseline(1);
    t=UINT32_MAX-2;
    latch_sample(&c,t,1);
    step(1,20);
    step(9,14);
    assert(c.good==1);
    baseline(1);
    uint32_t before=c.now;
    latch_sample(&c,before,9);
    assert(c.levels==1 && c.starts==0);
    step(9,4);
    step(1,4);
    step(9,4);
    step(9,5);
    assert(c.starts==2 && c.aborted==2 && c.collisions==1);
    baseline(1);
    c.starts=UINT32_MAX;
    c.completed=UINT32_MAX;
    c.good=UINT32_MAX;
    c.bins[1]=UINT32_MAX;
    step(9,4);
    assert(c.halted && (c.faults & PL_EXHAUSTED));
    assert(wire_status(buf,sizeof buf,&c)>0);
    assert(wire_status(buf,4,&c)==0);
    c.good--;
    assert(!latch_valid(&c) && !wire_status(buf,sizeof buf,&c));
    baseline(1);
    step(9,14);
    c.parity=1;
    c.faults|=PL_PARITY;
    assert(!latch_valid(&c));
    boundaries();
    randomized();
    puts("engine: parity, setup, raw hold, gap, wrap, rearming, overflow, encoder passed");
    return 0;
}
