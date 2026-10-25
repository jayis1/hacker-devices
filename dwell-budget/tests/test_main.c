/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#define main embedded_main
#include "../firmware/main.c"
#undef main
static uint32_t clock_ms;
static uint8_t input;
static bool wd_result = true;
static bool stall;
static unsigned records;
static char latest[384];
void board_init(void) {}
uint32_t board_millis(void) { return clock_ms; }
uint8_t board_levels(void) { return input; }
uint32_t board_reset_flags(void) { return 123; }
bool board_watchdog_start(void) { return wd_result; }
void board_watchdog_feed(void) {}
bool serial_busy(void) { return stall; }
void serial_poll(void) {}
bool serial_take_timeout(void) { return false; }
bool serial_submit(const char *p,size_t n)
{
    assert(n < sizeof latest);
    memcpy(latest,p,n);
    latest[n]=0;
    ++records;
    return true;
}
int main(void)
{
    initialize();
    iteration();
    assert(strncmp(latest,"DB1,B,123*",10)==0);
    iteration();
    assert(strncmp(latest,"DB1,P,10000,2000,20*",19)==0);
    input=3;
    for(clock_ms=1;clock_ms<=3000;++clock_ms) iteration();
    assert(budget.total_active==3000 && budget.occupied==3000);
    assert(budget.faults==(DB_EXCESS|DB_NO_ALARM));
    assert(records>20);
    /* A busy host link cannot block acquisition. */
    stall=true;
    for(;clock_ms<=4000;++clock_ms) iteration();
    assert(budget.total_active==4000);
    stall=false;
    clock_ms+=10;
    iteration();
    assert(budget.fill==0 && budget.gaps==1);
    wd_result=false;
    initialize();
    assert(budget.faults==DB_WATCHDOG && budget_valid(&budget));
    for(unsigned i=0;i<100;++i) { ++clock_ms; iteration(); }
    assert(budget.fill==0 && budget.total_active==0);
    puts("PASS production main scheduling, busy UART, gaps, watchdog fail-closed");
}
