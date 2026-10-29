/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
static Ledger s;
static uint32_t clock_ms;
static void ticks(uint8_t raw, unsigned n)
{
    while (n--) {
        ledger_sample(&s, ++clock_ms, raw);
        if (!ledger_valid(&s)) {
            fprintf(stderr,"invalid at %u raw %u state %u attempts %u outcomes %u/%u/%u coverage %u faults %u\n",clock_ms,raw,s.state,s.attempts,s.failures,s.successes,s.abandoned,s.coverage,s.faults);
            assert(ledger_valid(&s));
        }
        char output[768];
        assert(wire_status(output, sizeof output, &s));
    }
}
static void level(uint8_t raw) { ticks(raw,4); }
static void baseline(void)
{
    ledger_init(&s); clock_ms=0;
    level(8); level(0);
    assert(s.state==BR_READY && s.resets==1);
}
static void attempt(void) { level(1); level(0); }
static void failure(void) { level(2); level(0); }
static void success(void) { level(4); level(0); }
int main(void)
{
    baseline(); attempt(); failure();
    assert(s.grade==1 && s.failures==1 && s.remaining==6);
    attempt(); assert(s.early==1); success();
    assert(s.grade==0 && s.remaining==0 && s.successes==1);
    baseline();
    for(unsigned i=1;i<=9;++i) {
        ticks(0,s.remaining); attempt(); failure();
        unsigned g=i>7?7:i;
        assert(s.grade==g && s.remaining==(10u<<(g-1))-4);
    }
    assert(!s.early);
    /* Qualified arrival at deadline is accepted; one tick before is early. */
    for(unsigned early=0;early<2;++early) {
        baseline(); attempt(); failure();
        ticks(0,s.remaining-4-early); level(1);
        assert(s.early==early);
    }
    baseline(); attempt(); level(6);
    assert(s.state==BR_UNKNOWN && s.abandoned==1 && s.malformed==1);
    baseline(); success(); assert(s.malformed==1 && !s.attempts);
    baseline(); attempt(); ticks(0,1000-s.age); level(0);
    assert(s.timeouts==1 && s.abandoned==1 && !s.state);
    baseline(); attempt();
    ticks(0,1000-s.age-4); level(2);
    assert(s.failures==1 && !s.timeouts);
    baseline(); attempt();
    ledger_sample(&s,clock_ms+=10,0);
    assert(s.gaps==1 && s.missed==9 && s.abandoned==1 && !s.state);
    assert(ledger_valid(&s));
    level(0); assert(!s.state); level(8); level(0); assert(s.state==1);
    unsigned oldresets=s.resets;
    ledger_sample(&s,clock_ms+=10,8); ticks(8,3);
    assert(s.resets==oldresets+1); level(0);
    baseline(); attempt(); level(8);
    assert(s.abandoned==1 && s.resets==2); level(0);
    baseline(); attempt(); level(0); level(1);
    assert(s.attempts==2 && s.abandoned==2 && s.malformed==1);
    baseline(); s.attempts=s.successes=s.coverage=UINT32_MAX;
    level(1); assert(s.halted && (s.faults&BR_EXHAUSTED));
    baseline(); attempt(); s.coverage=UINT32_MAX;
    ticks(0,1); assert(s.halted && s.abandoned==1 && s.attempts==1);
    baseline(); clock_ms=UINT32_MAX-2; s.now=clock_ms;
    ticks(0,8); assert(!s.gaps);
    /* Held strobes do not repeat. Unrelated chatter cannot hide an attempt. */
    baseline(); level(1); ticks(1,50); assert(s.attempts==1);
    baseline();
    for(unsigned i=0;i<8;++i) ticks((uint8_t)(1|((i&1)?2:0)),1);
    assert(s.attempts==1);
    /* All short raw sequences: conservation and serialization remain valid. */
    for(unsigned seed=0;seed<64;++seed) {
        baseline(); unsigned x=seed+1;
        for(unsigned i=0;i<2000;++i) {
            x=x*1664525u+1013904223u;
            ticks((uint8_t)(x>>28),1+(x&7u));
        }
    }
    baseline(); char tiny[1]; assert(!wire_status(tiny,1,&s));
    assert(!wire_status(NULL,768,&s));
    s.early=1; assert(!ledger_valid(&s));
    puts("PASS engine: ramp, cap, boundaries, ambiguity, timeout, reset, gap, overflow, wrap, chatter, random conservation");
}
