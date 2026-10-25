/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "quarantine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct quarantine c;
static uint32_t now;
static void run(unsigned mask,unsigned ticks)
{
    while(ticks--) rq_sample(&c,++now,(uint8_t)mask);
}
static void init(void) { now=0; rq_init(&c,now); }
static void reset(void) { run(1,3); assert(c.state==RQ_RESET); }
static void settle(void) { run(2,3); assert(c.state==RQ_SETTLING); }
int main(void)
{
    init(); run(2,300); assert(c.state==RQ_UNKNOWN && c.releases==0);
    run(6,3); assert(c.unsafe==1); /* Cold attach is not a reset witness. */
    init(); reset(); settle(); run(2,249); assert(c.state==RQ_SETTLING);
    run(2,1); assert(c.state==RQ_ELIGIBLE && c.releases==1);
    run(6,3); assert(c.unsafe==0);
    run(14,3); assert(c.unsafe==1 && c.revoked==1 && c.state==RQ_LOCKED);
    run(2,300); assert(c.state==RQ_LOCKED); /* Fault requires a new reset. */
    reset(); settle(); run(2,250); assert(c.releases==2);
    init(); reset(); settle(); run(2,247); run(6,3);
    assert(c.unsafe==1 && c.state==RQ_LOCKED); /* Access exactly at expiry is not later. */
    init(); reset(); settle(); run(2,100); run(0,3);
    assert(c.interrupted==1 && c.state==RQ_RELEASED);
    settle(); run(2,250); assert(c.releases==1);
    init(); reset(); run(3,3); assert(c.contradictions==1 && c.state==RQ_RESET);
    run(1,3); assert(c.contradictions==1);
    settle(); run(2,20); run(6,3); assert(c.state==RQ_LOCKED && c.unsafe==1);
    init(); reset(); settle(); now+=5; rq_sample(&c,now,2);
    assert(!c.valid && c.gaps==1 && c.missed==4 && c.state==RQ_UNKNOWN);
    run(2,300); assert(c.releases==0); reset(); settle(); run(2,250);
    assert(c.releases==1);
    init(); now=UINT32_MAX-10; rq_init(&c,now); reset(); settle(); run(2,250);
    assert(c.releases==1 && c.state==RQ_ELIGIBLE);
    c.age=UINT32_MAX-1; run(2,4); assert(c.age==UINT32_MAX);
    init(); reset(); settle(); run(2,248); run(6,1); run(2,1);
    assert(c.state==RQ_ELIGIBLE); /* Subqualification glitches are not observations. */
    run(0,3); assert(c.revoked==1 && c.state==RQ_LOCKED);
    init(); for(unsigned i=0;i<200;++i) { run(1,3); run(0,3); }
    assert(c.count==RQ_QUEUE && c.dropped>0);
    struct rq_event e; unsigned n=0; while(rq_pop(&c,&e)) ++n;
    assert(n==RQ_QUEUE && !rq_pop(&c,NULL));
    c.tx_fail=UINT32_MAX; rq_tx_fail(&c); assert(c.tx_fail==UINT32_MAX);
    char b[384]; assert(rq_crc("123456789",9)==0x29b1);
    assert(rq_status(b,sizeof b,&c)>0 && !strncmp(b,"RQ1,S,",6));
    assert(!rq_status(b,3,&c) && !rq_status(NULL,0,&c));
    assert(!rq_status(b,sizeof b,NULL));
    puts("reset quarantine: timing, cold attach, reset, fault, gaps, wrap, overflow, encoder PASS");
    return 0;
}
