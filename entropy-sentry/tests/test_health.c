/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "health.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void bit(Health *h, unsigned b)
{
    for (unsigned i=0;i<5;i++) health_tick(h,h->time+1,(uint8_t)b);
    for (unsigned i=0;i<5;i++) health_tick(h,h->time+1,(uint8_t)(b|2));
}
int main(void)
{
    Health h;
    health_init(&h,0);
    assert(!h.qualified && !h.faults);
    for (unsigned i=0;i<31;i++) bit(&h,0);
    assert(h.run==31 && !(h.faults&FAULT_REPEAT));
    bit(&h,0);
    assert(h.run==32 && (h.faults&FAULT_REPEAT));
    health_init(&h,0);
    for (unsigned i=0;i<64;i++) bit(&h,i&1);
    assert(h.samples==64 && h.windows==1 && h.qualified && !h.faults);
    for (unsigned i=0;i<32;i++) bit(&h,1);
    assert(h.faults & FAULT_REPEAT);
    assert(!h.qualified);
    for (unsigned i=0;i<128;i++) bit(&h,i&1);
    assert(h.faults & FAULT_REPEAT); /* A later passing window cannot clear it. */
    health_init(&h,0);
    for (unsigned i=0;i<64;i++) bit(&h,(i%4)!=3);
    assert(!h.faults && h.last_matches==48);
    health_init(&h,0);
    for (unsigned i=0;i<64;i++) bit(&h,(i%4)!=3 || i==63);
    assert(h.faults & FAULT_PROPORTION);
    assert(h.last_matches==49);
    health_init(&h,0);
    health_tick(&h,1,8);
    assert(h.unsafe_episodes==1 && h.unsafe_ms==1);
    health_tick(&h,2,8);
    assert(h.unsafe_episodes==1 && h.unsafe_ms==2);
    health_tick(&h,3,0);
    health_tick(&h,4,8);
    assert(h.unsafe_episodes==2);
    health_init(&h,0);
    health_tick(&h,1,0);
    health_tick(&h,2,3); /* data changes on sampling edge */
    assert(h.faults & FAULT_SETUP);
    health_init(&h,0);
    health_tick(&h,1,0);
    health_tick(&h,2,0);
    health_tick(&h,3,2);
    health_tick(&h,4,0);
    health_tick(&h,5,2);
    assert(h.faults & FAULT_RATE);
    health_init(&h,0);
    for (unsigned i=0;i<64;i++) bit(&h,i&1);
    health_tick(&h,h.time+4,0);
    assert(h.gaps==1 && h.missed==3 && !h.qualified && !h.window_used);
    for (unsigned i=0;i<64;i++) bit(&h,i&1);
    assert(h.qualified);
    for (unsigned i=0;i<101;i++) health_tick(&h,h.time+1,0);
    assert(!h.qualified && h.stalls==1);
    health_init(&h,0);
    bit(&h,0);
    while (h.edge_age<100) health_tick(&h,h.time+1,0);
    health_tick(&h,h.time+1,2);
    assert(h.stalls==1 && !h.faults && h.samples==2);
    health_init(&h,0);
    bit(&h,0);
    while (h.edge_age<99) health_tick(&h,h.time+1,0);
    health_tick(&h,h.time+1,2);
    assert(!h.stalls && !h.faults && h.samples==2);
    health_init(&h,0);
    for (unsigned i=0;i<64*40;i++) bit(&h,i&1);
    assert(h.count==EVENT_CAPACITY && h.dropped==8);
    Event e;
    for (unsigned i=0;i<EVENT_CAPACITY;i++) {
        assert(health_pop(&h,&e) && e.sequence==i && e.kind==2);
    }
    assert(!health_pop(&h,&e));
    health_init(&h,UINT32_MAX-5);
    bit(&h,0);
    assert(h.samples==1 && !h.gaps);
    h.unsafe_ms=UINT32_MAX;
    health_tick(&h,h.time+1,8);
    assert(h.unsafe_ms==UINT32_MAX);
    char output[384];
    assert(wire_status(output,sizeof output,&h,0)>0);
    assert(strncmp(output,"ES1,S,",6)==0);
    assert(wire_status(output,8,&h,0)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    assert(wire_policy(output,sizeof output)>0);
    puts("health boundaries, persistence, continuity, timing, saturation and encoder PASS");
}
