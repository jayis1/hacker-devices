/* Author: jayis1. Synthetic input only; not a physical measurement. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Latch c;
static uint32_t now;
static void run(uint8_t raw, unsigned ticks)
{
    while (ticks--)
        latch_sample(&c, ++now, raw);
}
static void status(void)
{
    char buf[768];
    size_t n=wire_status(buf,sizeof buf,&c);
    assert(n);
    assert(fwrite(buf,1,n,stdout)==n);
}
int main(void)
{
    char buf[768];
    latch_init(&c);
    size_t n=wire_policy(buf,sizeof buf);
    assert(fwrite(buf,1,n,stdout)==n);
    run(1,20);
    status();
    run(9,20);
    status();
    run(0,20);
    run(8,20);
    status();
    return 0;
}
