/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
static jmp_buf done;
static uint32_t now;
static unsigned boots, policies, statuses, feeds;
#define main production_main
#include "../firmware/main.c"
#undef main
void board_init(void) { now=0; }
uint32_t board_millis(void) { return now; }
uint32_t board_reset_flags(void) { return 0; }
bool board_watchdog_start(void) { return true; }
void board_watchdog_feed(void)
{
    ++feeds;
    if(++now==2100) longjmp(done,1);
}
uint8_t board_levels(void) { return now<20?0:3; }
void serial_poll(void) { }
bool serial_busy(void) { return false; }
bool serial_take_timeout(void) { return now==50; }
bool serial_submit(const char *data,size_t size)
{
    assert(size>0 && size<SERIAL_BUFFER);
    if(!strncmp(data,"CG1,B,",6)) { ++boots; return boots>1; }
    if(!strncmp(data,"CG1,P,",6)) ++policies;
    if(!strncmp(data,"CG1,S,",6)) ++statuses;
    return true;
}
int main(void)
{
    if(!setjmp(done)) (void)production_main();
    assert(boots==2 && policies==2 && statuses>=19 && feeds==2100);
    assert(monitor.pairs==1 && monitor.tx_fail==2);
    puts("production main: boot retry, periodic policy, status, watchdog, TX loss PASS");
    return 0;
}
