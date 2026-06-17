/**
 * timing.c — Microsecond/Millisecond Delay and Timer Services
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 *
 * Provides delay_us() and delay_ms() used throughout the firmware.
 * Implements a simple SysTick-based timer running at 1 kHz for
 * millisecond delays, and a DWT cycle counter for microsecond delays.
 */

#include <stdint.h>
#include "board.h"

static volatile uint64_t g_systick_ms = 0;

/**
 * SysTick_Handler: Incremented every 1 ms.
 */
void SysTick_Handler(void)
{
    g_systick_ms++;
}

/**
 * timing_init: Configure SysTick for 1 ms interrupts.
 * Core clock: 480 MHz, SysTick reload = 480000 - 1 = 479999
 */
void timing_init(void)
{
    /* Enable DWT cycle counter for microsecond timing */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0;

    /* Configure SysTick: 480 MHz / 480000 = 1 kHz = 1 ms */
    SysTick_Config(CORE_CLOCK_HZ / 1000);
}

/**
 * delay_us: Busy-wait for approximately 'us' microseconds.
 * Uses DWT cycle counter for cycle-accurate timing.
 */
void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (CORE_CLOCK_HZ / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

/**
 * delay_ms: Blocking millisecond delay using SysTick.
 */
void delay_ms(uint32_t ms)
{
    uint64_t target = g_systick_ms + ms;
    while (g_systick_ms < target);
}

/**
 * get_ms: Return current system timestamp in milliseconds.
 */
uint64_t get_ms(void)
{
    return g_systick_ms;
}

/**
 * get_us: Return current system timestamp in microseconds.
 */
uint64_t get_us(void)
{
    return DWT->CYCCNT / (CORE_CLOCK_HZ / 1000000);
}