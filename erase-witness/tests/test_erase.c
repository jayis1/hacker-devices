/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "erase.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static void hold(struct erase *e, uint8_t levels, unsigned ticks)
{
    while (ticks--) ew_sample(e, ++now, levels);
}
static void baseline(struct erase *e)
{
    now=0; ew_init(e,now); hold(e,0,4);
    assert(e->state==EW_IDLE && e->valid);
}
int main(void)
{
    struct erase e;
    baseline(&e);
    hold(&e,1,3); assert(e.state==EW_WAIT_BUSY);
    hold(&e,3,110); assert(e.state==EW_BUSY);
    hold(&e,5,3); assert(e.state==EW_COMPLETE && e.completed==1);
    hold(&e,13,3); assert(e.unsafe==0);
    hold(&e,0,3); assert(e.state==EW_IDLE);
    hold(&e,9,3); assert(e.unsafe==1);
    hold(&e,9,20); assert(e.unsafe==1);
    hold(&e,0,3); assert(e.aborted==1);
    baseline(&e); hold(&e,5,3); assert(e.rejected==1 && e.completed==0);
    baseline(&e); hold(&e,1,3); hold(&e,3,10); hold(&e,5,3);
    assert(e.rejected==1 && e.completed==0);
    baseline(&e); hold(&e,1,505); assert(e.timeouts==1 && e.state==EW_FAULT);
    baseline(&e); hold(&e,1,3); hold(&e,3,5010);
    assert(e.timeouts==1 && e.completed==0);
    baseline(&e); hold(&e,3,3); assert(e.rejected==1); /* simultaneous request/busy */
    baseline(&e); hold(&e,1,3); hold(&e,3,110); hold(&e,13,3);
    assert(e.completed==1 && e.unsafe==1); /* access at completion is too early */
    baseline(&e); hold(&e,1,3); now+=10; ew_sample(&e,now,3);
    assert(!e.valid && e.gaps==1 && e.state==EW_UNKNOWN);
    hold(&e,5,5); assert(e.completed==0 && e.state==EW_UNKNOWN);
    hold(&e,0,3); assert(e.state==EW_IDLE);
    baseline(&e); now=UINT32_MAX-3; ew_init(&e,now); hold(&e,0,5);
    assert(e.valid && !e.gaps); /* tick wrap */
    uint8_t valid=e.valid; ew_sample(&e,now,15); assert(e.valid==valid);
    baseline(&e); e.unsafe=UINT32_MAX; hold(&e,8,3); assert(e.unsafe==UINT32_MAX);
    baseline(&e);
    for(unsigned i=0;i<100;++i) { hold(&e,8,3); hold(&e,0,3); }
    assert(e.dropped>0);
    struct ew_event event; unsigned count=0;
    while(ew_pop(&e,&event)) ++count;
    assert(count==EW_QUEUE);
    char output[384];
    assert(ew_crc("123456789",9)==0x29b1);
    assert(ew_status(output,sizeof output,&e)>0);
    assert(strncmp(output,"EW1,S,",6)==0);
    for(size_t cap=0;cap<12;++cap) assert(!ew_status(output,cap,&e));
    assert(!ew_status(NULL,100,&e));
    puts("PASS erase ordering, dwell, deadline, access, gaps, wrap, saturation, queue, encoder");
}
