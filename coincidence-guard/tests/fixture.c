/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic inputs only; this executable does not measure hardware. */
#include "consent.h"
#include "wire.h"
#include <stdio.h>
static struct consent c;
static uint32_t now;
static char line[384];
static void run(uint8_t mask,unsigned ticks)
{
    while(ticks--) cg_sample(&c,++now,mask);
}
int main(void)
{
    cg_init(&c,0);
    cg_boot(line,sizeof line,0); fputs(line,stdout);
    cg_policy(line,sizeof line); fputs(line,stdout);
    run(0,3); run(1,3); run(3,3); run(7,3); run(15,3); run(0,3);
    cg_status(line,sizeof line,&c); fputs(line,stdout);
    struct cg_event e;
    while(cg_pop(&c,&e)) {
        cg_event_frame(line,sizeof line,&e); fputs(line,stdout);
    }
    return 0;
}
