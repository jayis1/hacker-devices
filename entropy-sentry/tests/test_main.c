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
    if (++now==2100) longjmp(done,1);
}
uint8_t board_levels(void)
{
    return (uint8_t)(((now/10)&1u) | (now%10>=5?2:0));
}
void serial_poll(void) { }
bool serial_busy(void) { return false; }
bool serial_take_timeout(void) { return now==50; }
bool serial_submit(const char *data,size_t size)
{
    assert(size>0 && size<SERIAL_BUFFER);
    if (!strncmp(data,"ES1,B,",6)) { ++boots; return false; }
    if (!strncmp(data,"ES1,P,",6)) ++policies;
    if (!strncmp(data,"ES1,S,",6)) {
        ++statuses;
        if (now>=100) {
            /* Rejecting boot plus one timeout must appear in every later status. */
            const char *last=strrchr(data,',');
            assert(last && last[1]=='2' && last[2]=='*');
        }
    }
    return true;
}
int main(void)
{
    if (!setjmp(done)) (void)production_main();
    assert(boots==1 && policies==3 && statuses==20 && feeds==2100);
    puts("production main: lost boot, repeated policy, status, watchdog, TX loss PASS");
    return 0;
}
