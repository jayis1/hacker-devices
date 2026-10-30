/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic sample stream through the production engine and encoder. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Ledger state;
static uint32_t now;
static void emit(void)
{
    char frame[768];
    size_t n=wire_status(frame,sizeof frame,&state);
    assert(n); assert(fwrite(frame,1,n,stdout)==n);
}
static void hold(unsigned raw,unsigned count)
{
    while(count--) ledger_sample(&state,++now,raw);
    emit();
}
static void pulse(unsigned tag,unsigned kind)
{
    hold(tag,20); hold(tag|kind,10); hold(tag,10);
}
int main(void)
{
    char frame[768]; size_t n=wire_policy(frame,sizeof frame);
    assert(fwrite(frame,1,n,stdout)==n);
    ledger_init(&state); hold(0,20);
    unsigned order[]={3,1,0,2};
    for(unsigned i=0;i<4;i++) pulse(order[i],4);
    for(unsigned i=0;i<4;i++) pulse(i,8);
    pulse(1,4); pulse(1,8); /* Deliberate out-of-order visibility. */
    hold(0,100);
    return 0;
}
