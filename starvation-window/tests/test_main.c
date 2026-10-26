/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Exercise production loop with a deterministic board boundary, not silicon. */
#include "board.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uint32_t now;
static uint8_t inputs;
static bool watchdog = true;
static bool timeout_once;
static unsigned policies, statuses, boots;
void board_init(void) { now=0; }
uint32_t board_millis(void) { return now; }
uint32_t board_reset_flags(void) { return 3; }
uint8_t board_levels(void) { return inputs; }
bool board_watchdog_start(void) { return watchdog; }
void board_watchdog_feed(void) { }
bool serial_busy(void) { return false; }
void serial_poll(void) { }
bool serial_take_timeout(void) { bool v=timeout_once; timeout_once=false; return v; }
bool serial_submit(const char *data, size_t size)
{
    assert(size>0);
    if (!strncmp(data,"SW1,P,",6)) ++policies;
    if (!strncmp(data,"SW1,B,",6)) ++boots;
    if (!strncmp(data,"SW1,S,",6)) ++statuses;
    return true;
}
#define main embedded_main
#include "../firmware/main.c"
#undef main
int main(void)
{
    initialize();
    for(now=0;now<2200;now++) iteration();
    assert(boots==1 && policies==3 && statuses>=20);
    assert(sensor.state==IW_READY && !sensor.faults);
    timeout_once=true; iteration(); assert(sensor.faults&IW_TX);
    watchdog=false; initialize();
    assert(sensor.state==IW_HALTED && isolation_valid(&sensor));
    for(now=0;now<1100;now++) iteration();
    assert(sensor.state==IW_HALTED && !sensor.switches);
    puts("PASS main repeated policy, startup watchdog, serial loss");
}
