/*
 * trigger.c - Hardware trigger I/O with precise timing
 *
 * Uses EXTI for input edge detection and TIM3 for output pulse generation.
 * Supports configurable delay between trigger and action.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#include "trigger.h"
#include "board.h"
#include "registers.h"

static trigger_config_t trig_config;
static volatile bool trigger_pending = false;
static volatile uint32_t trigger_timestamp_ms = 0;

extern volatile uint32_t systick_ms;

/* ============================================================
 * GPIO and EXTI configuration
 * ============================================================ */

static void trigger_configure_gpio(void)
{
    uint32_t pc_base = GPIOC_BASE;
    
    /* Trigger input (PC6) - input, pull-down */
    GPIO_MODER(pc_base) &= ~(3U << (TRIG_IN_PIN * 2));
    GPIO_PUPDR(pc_base) &= ~(3U << (TRIG_IN_PIN * 2));
    GPIO_PUPDR(pc_base) |= (GPIO_PULLDOWN << (TRIG_IN_PIN * 2));
    
    /* Trigger output (PC7) - output, push-pull */
    GPIO_MODER(pc_base) &= ~(3U << (TRIG_OUT_PIN * 2));
    GPIO_MODER(pc_base) |= (GPIO_MODE_OUTPUT_PP << (TRIG_OUT_PIN * 2));
    GPIO_OSPEEDR(pc_base) |= (GPIO_SPEED_VHIGH << (TRIG_OUT_PIN * 2));
    
    /* Configure EXTI for trigger input */
    uint32_t exti_base = EXTI_BASE;
    uint32_t syscfg_base = SYSCFG_BASE;
    
    /* Select PC6 as EXTI source */
    REG32(syscfg_base + 0x08) &= ~(0xFU << (TRIG_IN_EXTI_LINE * 4));
    REG32(syscfg_base + 0x08) |= (2U << (TRIG_IN_EXTI_LINE * 4));  /* Port C = 2 */
    
    /* Configure rising edge by default */
    REG32(exti_base + 0x00) |= (1U << TRIG_IN_EXTI_LINE);  /* Rising trigger */
    REG32(exti_base + 0x08) &= ~(1U << TRIG_IN_EXTI_LINE);  /* Disable falling */
    
    /* Enable interrupt mask */
    REG32(exti_base + 0x00) |= (1U << TRIG_IN_EXTI_LINE);
}

static void trigger_configure_timer(void)
{
    uint32_t tim_base = TIM3_BASE;
    /* TIM3 for output pulse timing */
    uint32_t timer_clk = BOARD_APB1_FREQ_HZ;
    uint32_t arr = timer_clk - 1;  /* 1µs per tick at 120MHz / 1 */
    
    TIM_PSC(tim_base) = 0;
    TIM_ARR(tim_base) = arr;
    TIM_CCMR1(tim_base) = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM_CCER(tim_base) = TIM_CCER_CC1E;
    TIM_CCR1(tim_base) = 0;
    TIM_CR1(tim_base) = TIM_CR1_ARPE;
    /* Don't enable yet - enable on trigger */
}

/* ============================================================
 * EXTI interrupt handler
 * ============================================================ */

void EXTI9_5_IRQHandler(void)
{
    uint32_t exti_base = EXTI_BASE;
    
    /* Check if our line (6) triggered */
    if (REG32(exti_base + 0x0C) & (1U << TRIG_IN_EXTI_LINE)) {
        /* Clear pending bit */
        REG32(exti_base + 0x14) = (1U << TRIG_IN_EXTI_LINE);
        
        if (trig_config.armed) {
            trigger_pending = true;
            trigger_timestamp_ms = systick_ms;
            trig_config.triggered = true;
        }
    }
}

/* ============================================================
 * Public API
 * ============================================================ */

void trigger_init(void)
{
    trigger_configure_gpio();
    trigger_configure_timer();
    
    memset(&trig_config, 0, sizeof(trig_config));
    trig_config.source = TRIG_SOURCE_NONE;
    trig_config.edge = TRIG_EDGE_RISING;
    trig_config.pulse_width_us = 10;
    trigger_pending = false;
}

void trigger_arm(trigger_source_t source, trigger_edge_t edge, uint32_t delay_us)
{
    uint32_t exti_base = EXTI_BASE;
    
    trig_config.source = source;
    trig_config.edge = edge;
    trig_config.delay_us = delay_us;
    trig_config.armed = true;
    trig_config.triggered = false;
    trigger_pending = false;
    
    /* Configure edge detection */
    switch (edge) {
    case TRIG_EDGE_RISING:
        REG32(exti_base + 0x00) |= (1U << TRIG_IN_EXTI_LINE);
        REG32(exti_base + 0x08) &= ~(1U << TRIG_IN_EXTI_LINE);
        break;
    case TRIG_EDGE_FALLING:
        REG32(exti_base + 0x00) &= ~(1U << TRIG_IN_EXTI_LINE);
        REG32(exti_base + 0x08) |= (1U << TRIG_IN_EXTI_LINE);
        break;
    case TRIG_EDGE_BOTH:
        REG32(exti_base + 0x00) |= (1U << TRIG_IN_EXTI_LINE);
        REG32(exti_base + 0x08) |= (1U << TRIG_IN_EXTI_LINE);
        break;
    default:
        REG32(exti_base + 0x00) &= ~(1U << TRIG_IN_EXTI_LINE);
        REG32(exti_base + 0x08) &= ~(1U << TRIG_IN_EXTI_LINE);
        break;
    }
    
    /* Enable EXTI interrupt */
    NVIC_EnableIRQ(23);  /* EXTI9_5_IRQn = 23 on Cortex-M */
}

void trigger_disarm(void)
{
    uint32_t exti_base = EXTI_BASE;
    REG32(exti_base + 0x00) &= ~(1U << TRIG_IN_EXTI_LINE);
    REG32(exti_base + 0x08) &= ~(1U << TRIG_IN_EXTI_LINE);
    NVIC_DisableIRQ(23);
    
    trig_config.armed = false;
    trig_config.triggered = false;
    trigger_pending = false;
}

bool trigger_is_armed(void)
{
    return trig_config.armed;
}

bool trigger_is_pending(void)
{
    return trigger_pending;
}

void trigger_clear_pending(void)
{
    trigger_pending = false;
    trig_config.triggered = false;
}

void trigger_output_pulse(uint32_t width_us)
{
    uint32_t tim_base = TIM3_BASE;
    
    /* Set pulse width */
    uint32_t timer_clk = BOARD_APB1_FREQ_HZ;
    uint32_t counts = (width_us * (timer_clk / 1000000));
    
    TIM_ARR(tim_base) = counts;
    TIM_CCR1(tim_base) = counts / 2;  /* 50% duty for the pulse */
    TIM_CNT(tim_base) = 0;
    
    /* Also drive the GPIO directly for immediate response */
    GPIO_SET(GPIOC_BASE, TRIG_OUT_PIN);
    
    /* Simple delay for pulse width (microsecond-accurate) */
    for (volatile uint32_t i = 0; i < width_us * 48; i++) {
        __NOP();
    }
    
    GPIO_CLR(GPIOC_BASE, TRIG_OUT_PIN);
}

trigger_config_t *trigger_get_config(void)
{
    return &trig_config;
}

void trigger_set_config(const trigger_config_t *config)
{
    if (config) {
        memcpy(&trig_config, config, sizeof(trigger_config_t));
    }
}