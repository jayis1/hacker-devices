/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic evidence produced by the real engine and wire encoder. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Ledger s;
static void emit(void)
{
    char out[768]; size_t n=wire_status(out,sizeof out,&s);
    assert(n); assert(fwrite(out,1,n,stdout)==n);
}
static void hold(unsigned raw,unsigned ticks)
{
    while(ticks--) ledger_sample(&s,s.now+1,(uint8_t)raw);
}
static void operation(unsigned tag,unsigned allocate)
{
    hold(tag,12); hold(tag|(allocate?4:8),8); hold(tag,8); emit();
}
int main(void)
{
    char out[768]; ledger_init(&s);
    size_t n=wire_policy(out,sizeof out); assert(fwrite(out,1,n,stdout)==n);
    hold(0,20); emit();
    operation(3,1); operation(0,1); /* exact load five */
    operation(3,0); operation(2,1); /* rolling replacement, load four */
    operation(0,0); operation(3,1); /* rejected 3+4, preserve tag two */
    return 0;
}
