/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic inputs only; this executable does not measure hardware. */
#include "quarantine.h"
#include "wire.h"
#include <stdio.h>
static struct quarantine c;
static uint32_t now;
static char line[384];
static void run(uint8_t mask,unsigned ticks)
{
    while(ticks--) rq_sample(&c,++now,mask);
}
int main(void)
{
    rq_init(&c,0);
    rq_boot(line,sizeof line,0); fputs(line,stdout);
    rq_policy(line,sizeof line); fputs(line,stdout);
    run(1,3); run(2,253); run(6,3); run(14,3); run(1,3);
    rq_status(line,sizeof line,&c); fputs(line,stdout);
    struct rq_event e;
    while(rq_pop(&c,&e)) {
        rq_event_frame(line,sizeof line,&e); fputs(line,stdout);
    }
    return 0;
}
