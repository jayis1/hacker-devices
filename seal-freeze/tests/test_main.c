/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Production scheduler, modeled peripheral functions, no claimed silicon. */
#define main embedded_main
#include "../firmware/main.c"
#undef main
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t fake_time;
static bool watchdog_result = true;
static bool reject;
static unsigned boots, policies, statuses, feeds;
void board_init(void) { fake_time=0; }
uint32_t board_millis(void) { return fake_time; }
uint32_t board_reset_flags(void) { return 123; }
uint8_t board_levels(void) { return 0; }
bool board_watchdog_start(void) { return watchdog_result; }
void board_watchdog_feed(void) { ++feeds; }
bool serial_busy(void) { return false; }
void serial_poll(void) { }
bool serial_take_timeout(void) { return false; }
bool serial_submit(const char *p,size_t n)
{
    assert(n>6 && n<SERIAL_BUFFER && p[n-1]=='\n');
    assert(!memcmp(p,"SF1,",4));
    if(p[4]=='B') ++boots;
    if(p[4]=='P') ++policies;
    if(p[4]=='S') { ++statuses; assert(ledger_valid(&sensor)); }
    if(reject) { reject=false; return false; }
    return true;
}
int main(void)
{
    initialize();
    reject=true;
    for(fake_time=1;fake_time<=2200;++fake_time) iteration();
    assert(boots==1 && policies>=3 && statuses>=20 && feeds==2200);
    assert(sensor.valid==15 && sensor.tx_fail==1 && !sensor.state);
    watchdog_result=false;
    initialize();
    assert(sensor.halted && (sensor.faults&SF_WATCHDOG));
    assert(ledger_valid(&sensor));
    unsigned oldfeeds=feeds;
    for(fake_time=1;fake_time<=200;++fake_time) iteration();
    assert(feeds==oldfeeds && !sensor.valid);
    puts("PASS main: lost boot, periodic policy, watchdog startup failure");
}
