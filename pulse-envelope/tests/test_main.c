/* Author: jayis1. Main-loop substitution; no physical board claimed. */
#include "board.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t clock_ms;
static bool watchdog=true, timeout, busy;
static unsigned boots,policies,statuses,feeds;
void board_init(void) { clock_ms=0; }
uint8_t board_levels(void) { return 0; }
uint32_t board_millis(void) { return clock_ms; }
uint32_t board_reset_flags(void) { return 123; }
bool board_watchdog_start(void) { return watchdog; }
void board_watchdog_feed(void) { ++feeds; }
bool serial_busy(void) { return busy; }
void serial_poll(void) { }
bool serial_take_timeout(void) { bool t=timeout;timeout=false;return t; }
bool serial_submit(const char *data,size_t n)
{
    assert(n && n<SERIAL_BUFFER);
    if(!strncmp(data,"PE1,B,",6)) ++boots;
    else if(!strncmp(data,"PE1,P,",6)) ++policies;
    else if(!strncmp(data,"PE1,S,",6)) ++statuses;
    else assert(false);
    return true;
}
#define main embedded_main
#include "../firmware/main.c"
#undef main
int main(void)
{
    initialize();
    for(clock_ms=0;clock_ms<3100;++clock_ms) iteration();
    assert(boots==1 && policies>=4 && statuses>=30 && feeds==3100);
    assert(witness_consistent(&sensor));
    busy=true;
    for(;clock_ms<3200;++clock_ms) iteration();
    assert(!sensor.gaps && sensor.now==3199);
    busy=false;timeout=true;iteration();
    assert(sensor.tx_fail==1 && (sensor.faults&PE_TX));
    watchdog=false;initialize();
    assert(sensor.halted && witness_consistent(&sensor));
    unsigned before=feeds;
    for(clock_ms=0;clock_ms<110;++clock_ms) iteration();
    assert(feeds==before && witness_consistent(&sensor));
    puts("main: recurring policy, busy TX acquisition, timeout and failed watchdog passed");
}
