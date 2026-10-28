/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic fixture, not physical measurement. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Barrier s;
static uint32_t now;
static char buffer[768];
static void send(size_t n) { assert(n); assert(fwrite(buffer,1,n,stdout)==n); }
static void hold(unsigned raw,unsigned n)
{
    while(n--) barrier_sample(&s,++now,raw);
}
static void status(void) { send(wire_status(buffer,sizeof buffer,&s)); }
int main(void)
{
    barrier_init(&s);
    send(wire_boot(buffer,sizeof buffer,0));
    send(wire_policy(buffer,sizeof buffer));
    hold(0,30); status();
    for(unsigned tag=0;tag<4;++tag) {
        hold(tag,12); hold(tag|4,8); hold(tag,8);
        status();
    }
    hold(8,8); hold(0,30); status();
    hold(1,12); hold(5,8); hold(1,8);
    hold(5,8); status();
    assert(s.completed==1 && s.duplicate==1);
}
