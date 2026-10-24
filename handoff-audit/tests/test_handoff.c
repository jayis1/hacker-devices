/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "handoff.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void hold(struct handoff *h, unsigned vector, unsigned ticks)
{
    while (ticks--) ha_sample(h,h->time+1,(uint8_t)vector);
}
static void ready(struct handoff *h)
{
    ha_init(h,0);
    hold(h,0,3);
    assert(h->synced);
}
int main(void)
{
    struct handoff h;
    ready(&h);
    hold(&h,5,3); /* command A + feedback A */
    assert(h.last_path==1 && h.transitions==0);
    hold(&h,0,3);
    hold(&h,0,20);
    hold(&h,10,3);
    assert(h.transitions==1 && h.short_break==0);
    assert(h.last_dead==23);
    /* Exact dead-time boundary: 3 ticks are spent qualifying the next vector. */
    for(unsigned duration=19;duration<=20;++duration) {
        ready(&h);
        hold(&h,5,3);
        hold(&h,0,3);
        hold(&h,0,duration-3);
        hold(&h,10,3);
        assert(h.last_dead==duration);
        assert(h.short_break==(duration<20));
    }
    ready(&h);
    hold(&h,2,53);
    assert(h.mismatch[1]==1 && h.mismatch[0]==0);
    hold(&h,10,3);
    hold(&h,0,3);
    hold(&h,10,3);
    assert(h.transitions==0); /* Returning to the same domain is not a handoff. */
    ready(&h);
    hold(&h,5,3);
    hold(&h,10,3); /* no observed all-off interval */
    assert(h.short_break==1 && h.last_dead==0);
    hold(&h,15,20);
    assert(h.command_overlap==1 && h.feedback_overlap==1);
    hold(&h,15,20);
    assert(h.command_overlap==1 && h.feedback_overlap==1);
    ready(&h);
    hold(&h,1,3);
    hold(&h,1,49);
    assert(h.mismatch[0]==0);
    hold(&h,1,1);
    assert(h.mismatch[0]==1);
    hold(&h,1,100);
    assert(h.mismatch[0]==1);
    hold(&h,5,3);
    hold(&h,1,53);
    assert(h.mismatch[0]==2);
    ready(&h);
    hold(&h,5,3);
    ha_sample(&h,h.time+10,0);
    assert(!h.synced && h.levels==5 && h.missed==9);
    hold(&h,10,20);
    assert(!h.synced && h.transitions==0);
    hold(&h,0,3);
    assert(h.synced && h.last_path==0);
    hold(&h,10,3);
    assert(h.transitions==0);
    ha_init(&h,UINT32_MAX-2);
    hold(&h,0,3);
    assert(h.synced && h.time==0);
    hold(&h,5,3);
    hold(&h,0,3);
    h.off_age=UINT32_MAX-1;
    hold(&h,0,4);
    assert(h.off_age==UINT32_MAX);
    hold(&h,10,3);
    assert(h.last_dead==UINT32_MAX && !h.short_break);
    ready(&h);
    for(unsigned i=0;i<100;++i) hold(&h,(i&1)?15:0,3);
    assert(h.dropped>0);
    struct ha_event e;
    unsigned count=0;
    while(ha_pop(&h,&e)) ++count;
    assert(count==HA_QUEUE && !ha_pop(&h,NULL));
    h.tx_fail=UINT32_MAX;
    ha_tx_fail(&h);
    assert(h.tx_fail==UINT32_MAX);
    assert(ha_crc("123456789",9)==0x29b1);
    char buf[384];
    size_t n=ha_status(buf,sizeof buf,&h);
    assert(n>0 && strstr(buf,"HA1,S,")==buf && buf[n-1]=='\n');
    for(size_t cap=0;cap<=n;++cap) assert(ha_status(buf,cap,&h)==0);
    assert(ha_status(NULL,sizeof buf,&h)==0);
    assert(ha_status(buf,sizeof buf,NULL)==0);
    assert(ha_event_frame(buf,sizeof buf,NULL)==0);
    ready(&h);
    ha_sample(&h,h.time,15);
    assert(h.levels==0);
    puts("PASS: handoff policy, gap recovery, boundaries, saturation, queue, encoder");
}
