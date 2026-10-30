/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic test data, never a physical capture. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Ledger s;
static uint32_t now;
static char out[768];
static void emit(size_t length)
{
    assert(length); assert(fwrite(out,1,length,stdout)==length);
}
static void hold(unsigned raw, unsigned ticks)
{
    while(ticks--) ledger_sample(&s,now++,(uint8_t)raw);
    emit(wire_status(out,sizeof out,&s));
}
int main(void)
{
    ledger_init(&s);
    emit(wire_boot(out,sizeof out,42));
    emit(wire_policy(out,sizeof out));
    hold(0,20); hold(8,20);
    hold(11,12); hold(15,12); hold(11,12); /* fresh 3 */
    hold(9,12); hold(13,12); hold(9,12); /* fresh 1, out of order */
    hold(11,12); hold(15,12); hold(11,12); /* duplicate 3 */
    hold(3,20); hold(11,20); /* new session */
    hold(15,12); hold(11,12); /* fresh 3 again */
    now+=5; hold(11,1); /* terminal observation gap */
    return 0;
}
