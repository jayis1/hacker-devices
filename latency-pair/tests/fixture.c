/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic stimuli through the production algorithm and encoder, not hardware. */
#include "wire.h"
#include <stdio.h>
static Pair p;
static uint32_t now;
static void tick(uint8_t bits) { pair_sample(&p,++now,bits); }
static void pulse(unsigned label, unsigned duration)
{
    uint8_t idle=(uint8_t)(4|(label<<1));
    tick(idle); tick(idle|1);
    for(unsigned i=1;i<duration;++i) tick(idle|1);
    tick(idle);
}
int main(void)
{
    char out[384];
    pair_init(&p);
    size_t n=wire_boot(out,sizeof out,0); fwrite(out,1,n,stdout);
    n=wire_policy(out,sizeof out); fwrite(out,1,n,stdout);
    tick(4);
    for(unsigned i=0;i<32;++i) {
        unsigned first=i&1u;
        pulse(first,first?20:10); pulse(!first,first?10:20);
        n=wire_status(out,sizeof out,&p); fwrite(out,1,n,stdout);
    }
    return ferror(stdout)?1:0;
}
