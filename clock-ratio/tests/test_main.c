/* Author: jayis1. Main loop test with simulated board and transport. */
#define main firmware_main
#include "../firmware/main.c"
#undef main
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t simulated;
static unsigned boot,policy,status,feeds;
static bool healthy=true;
void board_init(void) { simulated=0; }
uint32_t board_millis(void) { return simulated; }
uint32_t board_reset_flags(void) { return 0; }
uint8_t board_levels(void) { return 0; }
bool board_watchdog_start(void) { return healthy; }
void board_watchdog_feed(void) { feeds++; }
bool serial_busy(void) { return false; }
void serial_poll(void) { }
bool serial_take_timeout(void) { return false; }
bool serial_submit(const char *s,size_t n)
{
    assert(n>6);
    if(s[4]=='B') boot++;
    if(s[4]=='P') policy++;
    if(s[4]=='S') status++;
    return true;
}
int main(void)
{
    initialize();
    for(simulated=0;simulated<2100;simulated++) iteration();
    assert(boot==1 && policy==3 && status>=20 && feeds==2100);
    healthy=false; initialize(); iteration();
    assert(sensor.halted && (sensor.faults & CR_WATCHDOG));
    assert(clock_valid(&sensor));
    puts("main: repeated policy, statuses, watchdog failure passed");
    return 0;
}
