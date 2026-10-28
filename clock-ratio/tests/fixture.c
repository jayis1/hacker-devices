/* Author: jayis1. Synthetic fixture, not physical data. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
static Clock c;
static uint32_t now;
static void hold(unsigned bits,unsigned ticks)
{
    while(ticks--) clock_sample(&c,++now,bits);
}
int main(void)
{
    char line[768]; size_t n;
    clock_init(&c);
    n=wire_policy(line,sizeof line); fwrite(line,1,n,stdout);
    hold(0,10); hold(4,10); hold(6,5); hold(4,5);
    for(unsigned i=0;i<4;i++) { hold(5,5); hold(4,5); }
    hold(6,5); hold(4,5);
    n=wire_status(line,sizeof line,&c); fwrite(line,1,n,stdout);
    hold(6,5); hold(4,5);
    n=wire_status(line,sizeof line,&c); fwrite(line,1,n,stdout);
    return ferror(stdout) ? 1 : 0;
}
