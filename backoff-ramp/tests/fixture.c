/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic diagnostic sequence through production engine and encoder. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Ledger s;
static uint32_t clock_ms;
static char output[768];
static void emit(size_t n) { assert(n); assert(fwrite(output,1,n,stdout)==n); }
static void sample(unsigned raw,unsigned count)
{
    while(count--) ledger_sample(&s,++clock_ms,(uint8_t)raw);
    emit(wire_status(output,sizeof output,&s));
}
int main(void)
{
    ledger_init(&s);
    emit(wire_boot(output,sizeof output,123));
    emit(wire_policy(output,sizeof output));
    sample(8,4); sample(0,4);
    sample(1,4); sample(0,4); sample(2,4); sample(0,4);
    sample(1,4); sample(0,4); sample(4,4); sample(0,4);
    return 0;
}
