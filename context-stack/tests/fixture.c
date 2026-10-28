/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic software fixture, not a hardware capture. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
static Stack s;
static char buffer[768];
static void emit(void)
{
    size_t n=wire_status(buffer,sizeof buffer,&s);
    fwrite(buffer,1,n,stdout);
}
static void ticks(unsigned n,unsigned raw)
{
    while(n--) stack_sample(&s,s.now+1,raw);
    emit();
}
static void op(unsigned tag,unsigned signal)
{
    ticks(12,tag); ticks(8,tag|signal); ticks(8,tag);
}
int main(void)
{
    stack_init(&s);
    size_t n=wire_boot(buffer,sizeof buffer,0); fwrite(buffer,1,n,stdout);
    n=wire_policy(buffer,sizeof buffer); fwrite(buffer,1,n,stdout);
    ticks(30,0);
    op(1,4); op(2,4); op(2,8); op(3,8);
    return 0;
}
