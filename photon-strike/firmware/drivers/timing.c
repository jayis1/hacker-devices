/**
 * drivers/timing.c — cycle counters and delay helpers
 * Author: jayis1
 * License: GPL-2.0
 *
 * Uses the Cortex-M7 DWT cycle counter for microsecond-grade delays
 * and the SysTick for millisecond timestamps.
 */

#include "timing.h"
#include "../registers.h"

#define DWT_BASE       0xE0001000u
#define DWT_CTRL       (DWT_BASE + 0x00u)
#define DWT_CYCCNT     (DWT_BASE + 0x04u)
#define DEMCR          0xE000EDFCu
#define DEMCR_TRCENA   (1u << 24)

static volatile uint32_t s_tick;

void SysTick_Handler(void)
{
    s_tick++;
}

void timing_init(void)
{
    /* Enable DWT cycle counter. */
    REG32(DEMCR) |= DEMCR_TRCENA;
    REG32(DWT_CTRL) |= 1u;          /* CYCCNTENA */
    REG32(DWT_CYCCNT) = 0;
}

void ps_delay_us(uint32_t us)
{
    uint32_t start = REG32(DWT_CYCCNT);
    uint32_t cycles = us * 480u;    /* 480 MHz core */
    while ((REG32(DWT_CYCCNT) - start) < cycles) ;
}

void ps_delay_ms(uint32_t ms)
{
    ps_delay_us(ms * 1000u);
}

uint32_t ps_millis(void)
{
    return s_tick;
}