/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic observations through the real encoder, not physical evidence. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Ledger s;
static char frame[768];
static uint32_t now;
static void emit(size_t n)
{
    assert(n); assert(fwrite(frame,1,n,stdout)==n);
}
static void hold(unsigned raw)
{
    for (unsigned i=0;i<100;i++) ledger_sample(&s,++now,raw);
    emit(wire_status(frame,sizeof frame,&s));
}
int main(void)
{
    ledger_init(&s);
    emit(wire_boot(frame,sizeof frame,0));
    emit(wire_policy(frame,sizeof frame));
    hold(0); hold(1); hold(3); hold(2); hold(3); hold(0);
    hold(12); hold(0); hold(5); hold(0); hold(10); hold(0);
    ledger_sample(&s,now+=10,0); hold(0);
    ledger_halt(&s,LR_WATCHDOG); hold(0);
}
