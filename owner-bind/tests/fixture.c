/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Ledger s;
static void emit(void)
{
    char b[768]; size_t n=wire_status(b,sizeof b,&s);
    assert(n); fwrite(b,1,n,stdout);
}
static void hold(unsigned raw,unsigned count)
{
    while(count--) ledger_sample(&s,s.now+1,(uint8_t)raw);
    emit();
}
static void op(unsigned tag,unsigned edge)
{
    hold(tag,20); hold(tag|edge,20); hold(tag,20);
}
int main(void)
{
    char b[768]; ledger_init(&s);
    size_t n=wire_boot(b,sizeof b,0); fwrite(b,1,n,stdout);
    n=wire_policy(b,sizeof b); fwrite(b,1,n,stdout);
    hold(0,20);
    op(0,4); op(0,8); op(2,4); op(0,8);
    assert(s.faults==OB_BINDING);
    return 0;
}
