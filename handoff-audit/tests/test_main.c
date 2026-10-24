/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Run the actual production main loop against a deterministic board model.
 * UART register behavior is independently tested in test_serial.c. */
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#define main firmware_main
#include "../firmware/main.c"
#undef main
static jmp_buf finished;
static uint32_t now;
static bool occupied;
static bool failed;
static bool first=true;
static uint32_t start;
static unsigned policies;
static unsigned statuses;
void board_init(void) { now=0; }
uint32_t board_millis(void) { return ++now; }
uint32_t board_reset_flags(void) { return 0; }
uint8_t board_levels(void) { return 0; }
bool board_watchdog_start(void) { return true; }
void board_watchdog_feed(void)
{
    if(now>=2200) longjmp(finished,1);
}
bool serial_busy(void) { return occupied; }
bool serial_submit(const char *data,size_t size)
{
    assert(!occupied && size>0 && size<=SERIAL_BUFFER);
    if(first) {
        assert(strncmp(data,"HA1,B,",6)==0);
        first=false;
        occupied=true;
        start=now;
    } else {
        if(strncmp(data,"HA1,P,",6)==0) ++policies;
        if(strncmp(data,"HA1,S,",6)==0) ++statuses;
    }
    return true;
}
void serial_poll(void)
{
    if(occupied && now-start>=100) {
        occupied=false;
        failed=true;
    }
}
bool serial_take_timeout(void)
{
    bool result=failed;
    failed=false;
    return result;
}
int main(void)
{
    if(setjmp(finished)==0) (void)firmware_main();
    assert(policies==2 && statuses>=20);
    assert(monitor.tx_fail==1 && monitor.synced);
    puts("PASS: production main survives lost boot transmission; periodic policy/status continue");
}
