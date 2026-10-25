/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#define main embedded_main
#include "../firmware/main.c"
#undef main
static uint32_t clock_ms;
static uint8_t input=4;
static bool wd_result=true, stall, lose;
static unsigned records;
static char latest[384];
void board_init(void) {}
uint32_t board_millis(void) { return clock_ms; }
uint8_t board_levels(void) { return input; }
uint32_t board_reset_flags(void) { return 123; }
bool board_watchdog_start(void) { return wd_result; }
void board_watchdog_feed(void) {}
bool serial_busy(void) { return stall; }
void serial_poll(void) {}
bool serial_take_timeout(void) { bool v=lose; lose=false; return v; }
bool serial_submit(const char *p,size_t n)
{
    assert(n<sizeof latest); memcpy(latest,p,n); latest[n]=0;
    ++records; return true;
}
static void step(uint8_t bits)
{
    ++clock_ms; input=bits; iteration();
}
int main(void)
{
    initialize(); iteration();
    assert(!strncmp(latest,"LP1,B,123*",10));
    iteration(); assert(!strncmp(latest,"LP1,P,1000,10000,32,5*",21));
    stall=true;
    step(5); step(5); step(4); step(6); step(7); step(7); step(7); step(6);
    assert(pair.pairs==1 && pair.sum_a==2 && pair.sum_b==3);
    stall=false;
    for(unsigned i=0;i<1100;++i) step(4);
    assert(records>10); /* Includes periodic policy after missed boot. */
    lose=true; step(4); assert(pair.tx_fail==1);
    clock_ms+=10; iteration(); assert(pair.gaps==1);
    wd_result=false; initialize();
    assert(pair.faults==LP_WATCHDOG && pair_valid(&pair));
    for(unsigned i=0;i<100;++i) step(5);
    assert(pair.pairs==0 && pair.state==LP_WAIT);
    puts("main: scheduling, stalled link, gap, loss, watchdog fail-closed PASS");
}
