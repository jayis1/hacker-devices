/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "drain.h"
#include <assert.h>
#include <stdio.h>
static Drain d;
static uint32_t now;
static void tick(unsigned levels) { drain_sample(&d, ++now, levels); assert(drain_consistent(&d)); }
static void run(unsigned n, unsigned levels) { while(n--) tick(levels); }
static void fresh(void) { drain_init(&d); now=0; tick(0); tick(0); }
int main(void)
{
    fresh();
    assert(d.state==RD_READY);
    tick(RD_REVOKE|RD_BUSY);
    assert(d.started==1 && d.state==RD_DRAIN);
    run(5,RD_REVOKE|RD_BUSY);
    tick(RD_REVOKE);
    run(RD_QUIET_MS,RD_REVOKE);
    tick(RD_REVOKE|RD_ACK);
    assert(d.completed==1 && d.state==RD_SEALED && !d.faults);
    tick(0); tick(0);
    assert(d.state==RD_READY);
    /* Same-tick revoke/admission is not ordered before revocation. */
    tick(RD_REVOKE|RD_ADMIT);
    assert(d.admissions==1 && d.blocked==1 && (d.faults&RD_LEAK));
    /* Acknowledgment at the exact quiet deadline is not later than it. */
    fresh(); tick(RD_REVOKE); run(RD_QUIET_MS-1,RD_REVOKE);
    tick(RD_REVOKE|RD_ACK);
    assert(d.early==1 && !d.completed);
    tick(RD_REVOKE); tick(RD_REVOKE|RD_ACK);
    assert(d.completed==1); /* A fresh, later acknowledgment can complete. */
    tick(RD_REVOKE|RD_ACK|RD_BUSY);
    assert(d.faults&RD_REOPEN);
    fresh(); tick(RD_REVOKE); run(RD_DEADLINE_MS-1,RD_REVOKE);
    tick(RD_REVOKE|RD_ACK);
    assert(d.completed==1 && !(d.faults&RD_TIMEOUT));
    /* No baseline: attachment halfway through revocation is not a trial. */
    drain_init(&d); now=0; run(40,RD_REVOKE|RD_ACK);
    assert(!d.started && d.state==RD_WAIT);
    fresh(); tick(RD_REVOKE|RD_BUSY); run(RD_DEADLINE_MS,RD_REVOKE|RD_BUSY);
    assert(d.state==RD_DRAIN);
    tick(RD_REVOKE|RD_BUSY);
    assert(d.abandoned==1 && (d.faults&RD_TIMEOUT));
    run(30,RD_REVOKE); assert(d.started==1);
    tick(0); tick(RD_REVOKE); assert(d.started==2);
    /* Acquisition gaps conserve trials and never extrapolate quiet. */
    fresh(); tick(RD_REVOKE); now+=10; tick(RD_REVOKE|RD_ACK);
    assert(d.gaps==1 && d.abandoned==1 && !d.completed);
    tick(0); tick(RD_REVOKE); tick(0);
    assert(d.abandoned==2 && (d.faults&RD_WITHDRAWN));
    /* Independent sampled inputs: chatter cannot mask admission. */
    fresh(); tick(RD_REVOKE|RD_BUSY);
    for(unsigned i=0;i<10;i++) { tick(RD_REVOKE|RD_ADMIT|(i&1?RD_BUSY:0)); tick(RD_REVOKE); }
    assert(d.blocked==10);
    /* Timer wrap and duplicates. */
    fresh(); d.last=UINT32_MAX; now=UINT32_MAX;
    tick(RD_REVOKE); assert(d.state==RD_DRAIN && !d.gaps);
    drain_sample(&d,now,15); assert(d.levels==RD_REVOKE);
    /* Watchdog/counter exhaustion terminate rather than wrap. */
    fresh(); d.admissions=UINT32_MAX; tick(RD_ADMIT);
    assert(d.state==RD_FAULT && (d.faults&RD_OVERFLOW));
    fresh(); tick(RD_REVOKE); d.admissions=UINT32_MAX;
    tick(RD_REVOKE|RD_ADMIT);
    assert(d.abandoned==1 && d.started==d.completed+d.abandoned);
    fresh(); drain_halt(&d,RD_WATCHDOG); tick(0);
    assert(d.state==RD_FAULT);
    /* Deterministic adversarial sequences exercise invariant conservation. */
    fresh();
    uint32_t seed=17;
    for(unsigned i=0;i<1000000;++i) {
        seed=seed*1664525u+1013904223u;
        if ((seed&1023u)==0) now+=5;
        tick(seed>>28);
    }
    puts("PASS drain state boundaries, conservation, gap, wrap, overflow");
}
