/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic laboratory vectors through the production engine and encoder. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Ledger s;
static uint32_t now;
static char out[768];
static void emit(size_t n) { assert(n); assert(fwrite(out,1,n,stdout)==n); }
static void hold(unsigned raw)
{
    for(unsigned i=0;i<100;i++) ledger_sample(&s,now++,raw);
    emit(wire_status(out,sizeof out,&s));
}
int main(void)
{
    ledger_init(&s);
    emit(wire_boot(out,sizeof out,42));
    emit(wire_policy(out,sizeof out));
    hold(0); hold(1); hold(3); hold(7); hold(0);
    hold(3); hold(11); hold(3); hold(7); hold(0);
    hold(3); hold(1); hold(3); hold(7); hold(3); hold(7); hold(0);
    hold(7); hold(0); hold(1); hold(0);
    hold(3); now+=10; hold(3); hold(0);
    assert(s.accepted==1 && s.vetoed==2 && s.early==1 && s.repeated==1);
    assert(s.cancelled==1 && s.abandoned==1 && s.halted);
    return 0;
}
