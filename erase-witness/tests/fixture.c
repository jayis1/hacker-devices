/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic diagnostic vectors, not physical measurements. */
#include "erase.h"
#include "wire.h"
#include <stdio.h>
static char output[384];
static uint32_t now;
static void hold(struct erase *e, uint8_t levels, unsigned ticks)
{
    while (ticks--) ew_sample(e,++now,levels);
}
int main(void)
{
    struct erase e;
    ew_init(&e,now);
    ew_boot(output,sizeof output,0); fputs(output,stdout);
    ew_policy(output,sizeof output); fputs(output,stdout);
    hold(&e,0,4);
    hold(&e,1,3);
    hold(&e,3,110);
    hold(&e,5,3);
    hold(&e,13,3);
    hold(&e,0,3);
    hold(&e,9,3);
    hold(&e,0,3);
    struct ew_event event;
    while (ew_pop(&e,&event)) {
        ew_event_frame(output,sizeof output,&event); fputs(output,stdout);
    }
    ew_status(output,sizeof output,&e); fputs(output,stdout);
}
