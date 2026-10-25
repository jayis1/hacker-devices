/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    char out[384];
    Pair p;
    pair_init(&p);
    assert(wire_crc("123456789",9) == 0x29b1);
    size_t n=wire_policy(out,sizeof out);
    assert(n && !memcmp(out,"LP1,P,1000,10000,32,5*",21));
    for(size_t capacity=0;capacity<n;++capacity)
        assert(!wire_policy(out,capacity));
    assert(!wire_boot(NULL,10,0));
    assert(wire_boot(out,sizeof out,UINT32_MAX));
    assert(wire_status(out,sizeof out,&p));
    p.pairs=33;
    assert(!wire_status(out,sizeof out,&p));
    pair_init(&p);
    p.pairs=1; p.sum_a=10; p.sum_b=15; p.equal=1;
    p.large=1; p.max_delta=5;
    assert(!wire_status(out,sizeof out,&p));
    p.equal=0; p.b_slower=1; p.max_delta=999;
    assert(!wire_status(out,sizeof out,&p));
    p.max_delta=5; p.active=true; p.state=LP_ACTIVE;
    assert(!wire_status(out,sizeof out,&p));
    puts("wire: CRC known vector, bounds, null, invalid state PASS");
}
