/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void)
{
    char out[384]; Drain d;
    drain_init(&d);
    assert(wire_crc("123456789",9)==0x29b1);
    assert(!wire_status(out,2,&d));
    assert(!wire_status(NULL,sizeof out,&d));
    assert(wire_status(out,sizeof out,&d)>0);
    assert(!strncmp(out,"RD1,S,",6));
    d.completed=1;
    assert(!wire_status(out,sizeof out,&d));
    assert(wire_policy(out,sizeof out)>0);
    assert(!strncmp(out,"RD1,P,20,500*",13));
    assert(wire_boot(out,sizeof out,7)>0);
    puts("PASS wire CRC, capacity, invariant guard");
}
