/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "consent.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static struct consent c;
static uint32_t tick;
static void run(uint8_t value, unsigned ms)
{
    while (ms--) cg_sample(&c,++tick,value);
}
static void reset(uint32_t now)
{
    tick=now;
    cg_init(&c,now);
    run(0,3);
    assert(c.state==CG_READY && c.valid);
}
static void pair(void)
{
    run(1,3);
    run(3,3);
    assert(c.state==CG_PAIRED && c.pairs==1);
}
int main(void)
{
    reset(0);
    pair();
    run(7,3);
    assert(c.unsafe==0);
    run(3,3); /* permit dropping does not rearm held consents */
    run(7,3);
    assert(c.unsafe==0 && c.pairs==1);
    run(0,3);
    assert(c.state==CG_READY);
    run(2,3);
    run(3,3);
    assert(c.state==CG_PAIRED && c.pairs==2);
    reset(0);
    run(3,3); /* simultaneous qualified consent vector */
    assert(c.state==CG_PAIRED && c.skew_ms==0);
    run(15,3); /* inhibit must invalidate even an established pair */
    assert(c.state==CG_LOCKED && c.unsafe==1 && c.revoked==1);
    run(7,3);
    assert(c.state==CG_LOCKED && c.unsafe==1);
    run(0,3);
    assert(c.state==CG_READY);
    reset(0);
    run(1,3);
    run(1,247);
    run(3,3); /* exactly 250ms between qualified edges */
    assert(c.state==CG_PAIRED && c.skew_ms==250);
    reset(0);
    run(1,3);
    run(1,248);
    run(3,3);
    assert(c.state==CG_LOCKED && c.expired==1 && c.pairs==0);
    reset(0);
    run(1,3);
    run(2,3); /* hand exchange without both released */
    assert(c.state==CG_LOCKED && c.rejected==1);
    run(3,3);
    assert(c.pairs==0);
    reset(0);
    run(7,3); /* permission and pair in same vector: no proven ordering */
    assert(c.unsafe==1 && c.pairs==1);
    run(7,5);
    assert(c.unsafe==1);
    reset(0);
    run(4,3);
    assert(c.unsafe==1 && c.state==CG_LOCKED);
    run(0,3);
    pair();
    run(5,3); /* one consent removed while permission held */
    assert(c.unsafe==2 && c.revoked==1 && c.state==CG_LOCKED);
    reset(UINT32_MAX-5);
    pair();
    assert(c.pairs==1);
    cg_sample(&c,tick+10,7);
    tick+=10;
    assert(!c.valid && c.state==CG_UNKNOWN && c.gaps==1 && c.missed==9);
    run(7,3);
    assert(c.unsafe==1 && c.state==CG_LOCKED);
    run(0,3);
    assert(c.state==CG_READY);
    reset(0);
    cg_sample(&c,tick,15); /* duplicate tick cannot qualify */
    assert(c.levels==0);
    run(1,1); run(0,1); run(1,1);
    assert(c.state==CG_READY);
    run(1,2);
    assert(c.state==CG_WAIT_A);
    c.age=UINT32_MAX-1;
    run(1,3);
    assert(c.state==CG_LOCKED); /* age cannot wrap into a valid window */
    c.tx_fail=UINT32_MAX;
    cg_tx_fail(&c);
    assert(c.tx_fail==UINT32_MAX);
    reset(0);
    for(unsigned i=0;i<100;++i) { run(3,3); run(0,3); }
    assert(c.dropped>0 && c.count==CG_QUEUE);
    struct cg_event event;
    unsigned count=0;
    while(cg_pop(&c,&event)) ++count;
    assert(count==CG_QUEUE && !cg_pop(&c,&event));
    assert(cg_crc("123456789",9)==0x29b1);
    char out[384];
    assert(cg_status(out,sizeof out,&c)>0);
    assert(strncmp(out,"CG1,S,",6)==0);
    assert(cg_status(out,8,&c)==0);
    assert(cg_status(NULL,0,&c)==0);
    assert(cg_status(out,sizeof out,NULL)==0);
    /* Every qualified input mask in every reachable state; invariants matter
       more than enumerating visually plausible success-only examples. */
    for(unsigned mask=0;mask<16;++mask) {
        reset(0); pair(); run((uint8_t)mask,3);
        if(mask!=3 && mask!=7) assert(c.state!=CG_PAIRED);
        if((mask&4) && ((mask&3)!=3 || (mask&8))) assert(c.unsafe);
    }
    puts("consent boundary, anti-tie-down, inhibit, gap, wrap, queue and encoder PASS");
    return 0;
}
