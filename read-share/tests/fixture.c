/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic samples only: not a physical recording. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Ledger s;
static uint32_t now;
static char buffer[768];
static void status(void)
{
    size_t n=wire_status(buffer,sizeof buffer,&s);
    assert(n); assert(fwrite(buffer,1,n,stdout)==n);
}
static void hold(unsigned raw,unsigned count)
{
    while(count--) ledger_sample(&s,++now,(uint8_t)raw);
}
static void event(unsigned tag,unsigned edge)
{
    hold(tag,20); hold(tag|edge,20); hold(tag,20); status();
}
int main(void)
{
    ledger_init(&s);
    size_t n=wire_boot(buffer,sizeof buffer,0); fwrite(buffer,1,n,stdout);
    n=wire_policy(buffer,sizeof buffer); fwrite(buffer,1,n,stdout);
    hold(0,20); status();
    event(0,4); event(1,4); event(0,8); event(0,4);
    event(1,8); event(0,8); /* one completed reader cohort */
    event(3,4); event(2,4); /* reader improperly admitted during writer */
    return 0;
}
