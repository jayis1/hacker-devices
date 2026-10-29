/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <stdlib.h>
static Ledger s;
static char line[768];
static void emit(size_t n)
{
    if (!n || fwrite(line, 1, n, stdout) != n) exit(1);
}
static void hold(unsigned mask, unsigned ticks)
{
    while(ticks--) ledger_sample(&s, s.now+1, (uint8_t)mask);
    emit(wire_status(line,sizeof line,&s));
}
int main(int argc, char **argv)
{
    (void)argv;
    ledger_init(&s);
    emit(wire_boot(line,sizeof line,0));
    emit(wire_policy(line,sizeof line));
    hold(8,100); /* synthetic reset baseline */
    hold(0,100); /* configuration window */
    hold(4,100); /* sealed zero */
    hold(5,100); /* deviation */
    hold(1,100); /* seal withdrawn, capture remains zero */
    hold(4,100); /* restoration does not erase findings */
    if (argc > 1) {
        /* Reset and release between reports, with SEAL still high. */
        for (unsigned i=0; i<8; ++i) ledger_sample(&s,s.now+1,12);
        hold(4,100);
    }
    return 0;
}
