/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic observations, never physical measurement evidence. */
#include "handoff.h"
#include "wire.h"
#include <stdio.h>
static struct handoff h;
static char line[384];
static void hold(uint8_t value, unsigned ticks)
{
    while(ticks--) ha_sample(&h,h.time+1,value);
}
int main(void)
{
    ha_init(&h,0);
    size_t size=ha_boot(line,sizeof line,0);
    fwrite(line,1,size,stdout);
    hold(0,3);
    hold(5,10);
    hold(0,25);
    hold(10,10);
    hold(5,10);
    hold(15,60);
    hold(1,60);
    struct ha_event e;
    while(ha_pop(&h,&e)) {
        size=ha_event_frame(line,sizeof line,&e);
        fwrite(line,1,size,stdout);
    }
    size=ha_status(line,sizeof line,&h);
    fwrite(line,1,size,stdout);
    size=ha_policy(line,sizeof line);
    fwrite(line,1,size,stdout);
    return ferror(stdout)?1:0;
}
