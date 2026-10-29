/* Author: jayis1. Production scheduler exercised with simulated board I/O. */
#include "board.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#define main embedded_main
#include "../firmware/main.c"
#undef main
static uint32_t clock_tick;
static unsigned boots,policies,statuses;
static bool healthy=true;
void board_init(void) { clock_tick=0; }
uint32_t board_millis(void) { return clock_tick; }
uint32_t board_reset_flags(void) { return 42; }
uint8_t board_levels(void) { return 0; }
bool board_watchdog_start(void) { return healthy; }
void board_watchdog_feed(void) { }
bool serial_busy(void) { return false; }
void serial_poll(void) { }
bool serial_take_timeout(void) { return false; }
bool serial_submit(const char *data,size_t n)
{
    assert(n>6 && n<SERIAL_BUFFER);
    assert(!memcmp(data,"CT1,",4));
    if(data[4]=='B') ++boots;
    if(data[4]=='P') ++policies;
    if(data[4]=='S') { ++statuses; assert(ledger_valid(&sensor)); }
    return true;
}
int main(void)
{
    initialize();
    for(clock_tick=0;clock_tick<=2200;++clock_tick) iteration();
    assert(boots==1 && policies==3 && statuses>=20);
    assert(sensor.ready && sensor.observed && !sensor.faults);
    healthy=false; initialize();
    assert(sensor.halted && ledger_valid(&sensor));
    for(clock_tick=0;clock_tick<200;++clock_tick) iteration();
    assert(sensor.halted && !sensor.observed);
    puts("main: periodic late-attach policy, one-shot boot, halted startup passed");
}
