/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Main-loop hardware substitution: no claim of physical board execution. */
#include "board.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t clock_ms;
static bool watchdog = true;
static unsigned boots, policies, statuses, feeds;
static bool timeout;
void board_init(void) { clock_ms=0; }
uint8_t board_levels(void) { return 4; }
uint32_t board_millis(void) { return clock_ms; }
uint32_t board_reset_flags(void) { return 123; }
bool board_watchdog_start(void) { return watchdog; }
void board_watchdog_feed(void) { ++feeds; }
bool serial_busy(void) { return false; }
void serial_poll(void) { }
bool serial_take_timeout(void) { bool result=timeout; timeout=false; return result; }
bool serial_submit(const char *data, size_t n)
{
    assert(n>0 && n<SERIAL_BUFFER);
    if(!strncmp(data,"BB1,B,",6)) ++boots;
    else if(!strncmp(data,"BB1,P,",6)) ++policies;
    else if(!strncmp(data,"BB1,S,",6)) ++statuses;
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
    assert(budget_valid(&sensor));
    timeout=true; iteration(); assert(sensor.tx_fail==1 && (sensor.faults&BB_TX));
    watchdog=false; initialize();
    assert(sensor.phase==BB_HALTED && budget_valid(&sensor));
    unsigned before=feeds;
    for(clock_ms=0;clock_ms<110;++clock_ms) iteration();
    assert(feeds==before && budget_valid(&sensor));
    puts("PASS production main loop, repeated policy, transmit failure, watchdog startup");
}
