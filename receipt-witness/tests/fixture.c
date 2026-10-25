/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic fixture; NOT physical measurements. */
#include "receipt.h"
#include "wire.h"
#include <stdio.h>
static Receipt r;
static char frame[384];
static void step(unsigned n,uint8_t raw)
{
    while(n--) receipt_tick(&r,r.time+1,raw);
    wire_status(frame,sizeof frame,&r,0);
    fputs(frame,stdout);
}
int main(void)
{
    receipt_init(&r,0);
    wire_boot(frame,sizeof frame,0); fputs(frame,stdout);
    wire_policy(frame,sizeof frame); fputs(frame,stdout);
    step(30,0);
    step(5,ACTION|APPEND); step(5,0);
    step(5,DURABLE); step(5,0);
    step(5,ACTION|BLOCKED); step(5,0);
    step(60,0); step(30,0);
    return 0;
}
