/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
static void hold(Ledger *s,unsigned raw,unsigned n)
{
    while(n--) {
        ledger_sample(s,s->now+1u,raw);
        assert(ledger_valid(s));
    }
}
static void start(Ledger *s)
{
    ledger_init(s); hold(s,0,20);
}
static void op(Ledger *s,unsigned tag,bool alloc)
{
    hold(s,tag,12); hold(s,tag|(alloc?4u:8u),6); hold(s,tag,6);
}
int main(void)
{
    Ledger s;
    /* Two immediate renewals allowed; third within rolling second blocked.
     * This intentionally fails against the inherited occupancy-only engine. */
    start(&s); op(&s,0,true); op(&s,0,false);
    op(&s,0,true); op(&s,0,false); op(&s,0,true);
    assert(s.faults==1 && s.launched==2 && !s.active);
    puts("rolling third-admission regression PASS");
}
