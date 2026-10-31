/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Production scheduler with controlled board transport, not a physical MCU. */
#define main target_main
#include "../firmware/main.c"
#undef main
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t clock_ms;
static bool healthy=true, busy=false, reject=false, expired=false;
static unsigned boots, policies, statuses;
void board_init(void) { clock_ms=0; }
uint32_t board_millis(void) { return clock_ms; }
uint8_t board_levels(void) { return 0; }
uint32_t board_reset_flags(void) { return 42; }
bool board_watchdog_start(void) { return healthy; }
void board_watchdog_feed(void) { }
bool serial_busy(void) { return busy; }
void serial_poll(void) { }
bool serial_take_timeout(void) { bool r=expired; expired=false; return r; }
bool serial_submit(const char *data,size_t size)
{
    assert(size>5 && size<768);
    assert(!memcmp(data,"UQ1,",4));
    if(reject) { reject=false; return false; }
    if(data[4]=='B') boots++;
    else if(data[4]=='P') policies++;
    else if(data[4]=='S') statuses++;
    else assert(0);
    return true;
}
int main(void)
{
    initialize(); reject=true; iteration(); /* lose boot, not periodic policy */
    for(clock_ms=1;clock_ms<=2200;clock_ms++) iteration();
    assert(!boots && policies==3 && statuses>=20 && sensor.tx_fail==1);
    busy=true; clock_ms++; iteration(); busy=false;
    expired=true; clock_ms++; iteration(); assert(sensor.tx_fail==2);
    healthy=false; initialize(); assert(sensor.halted && ledger_valid(&sensor));
    iteration(); assert(boots==1);
    puts("main: late-attach policy, lost boot, timeout, watchdog startup PASS");
}
