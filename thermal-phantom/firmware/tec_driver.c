/*
 * tec_driver.c - DRV8873 H-bridge driver for Peltier TEC module
 *
 * Controls bidirectional current through a TEC1-12706 Peltier module.
 * Positive duty = heating (current flows one way)
 * Negative duty = cooling (current reverses)
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#include "tec_driver.h"
#include "board.h"
#include "registers.h"
#include <string.h>

#define TEC_PWM_RESOLUTION   10000  /* 10000 counts = 100% duty */
#define TEC_SOFT_START_STEP_MS  10  /* 10ms per ramp step */

static float current_duty = 0.0f;
static bool tec_enabled = false;
static bool tec_fault = false;

/* Soft-start state */
static bool soft_start_active = false;
static float soft_start_target = 0.0f;
static float soft_start_current = 0.0f;
static float soft_start_step = 0.0f;
static uint32_t soft_start_last_ms = 0;

extern volatile uint32_t systick_ms;

/* ============================================================
 * Low-level PWM configuration
 * ============================================================ */

static void tec_configure_pwm(void)
{
    uint32_t tim_base = TIM1_BASE;
    uint32_t pwm_freq = TEC_PWM_FREQ_HZ;
    uint32_t timer_clk = BOARD_APB2_FREQ_HZ;
    uint32_t arr = (timer_clk / pwm_freq) - 1;
    
    /* Configure TIM1 CH1 for PWM mode 1 */
    TIM_PSC(tim_base) = 0;           /* No prescaler */
    TIM_ARR(tim_base) = arr;          /* Auto-reload = period */
    TIM_CCMR1(tim_base) = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM_CCER(tim_base) = TIM_CCER_CC1E;
    TIM_CCR1(tim_base) = 0;           /* Start at 0% duty */
    TIM_CR1(tim_base) = TIM_CR1_ARPE | TIM_CR1_CEN;  /* Enable timer */
}

static void tec_set_pwm(uint32_t duty_counts)
{
    uint32_t arr = TIM_ARR(TIM1_BASE);
    if (duty_counts > arr)
        duty_counts = arr;
    TIM_CCR1(TIM1_BASE) = duty_counts;
}

/* ============================================================
 * GPIO configuration for direction pins
 * ============================================================ */

static void tec_configure_gpio(void)
{
    uint32_t pe_base = GPIOE_BASE;
    
    /* IN1 (PE9) - output, push-pull */
    GPIO_MODER(pe_base) &= ~(3U << (TEC_IN1_PIN * 2));
    GPIO_MODER(pe_base) |= (GPIO_MODE_OUTPUT_PP << (TEC_IN1_PIN * 2));
    GPIO_OSPEEDR(pe_base) |= (GPIO_SPEED_HIGH << (TEC_IN1_PIN * 2));
    
    /* IN2 (PE11) - output, push-pull */
    GPIO_MODER(pe_base) &= ~(3U << (TEC_IN2_PIN * 2));
    GPIO_MODER(pe_base) |= (GPIO_MODE_OUTPUT_PP << (TEC_IN2_PIN * 2));
    GPIO_OSPEEDR(pe_base) |= (GPIO_SPEED_HIGH << (TEC_IN2_PIN * 2));
    
    /* NSLEEP (PE13) - output, push-pull (active low sleep) */
    GPIO_MODER(pe_base) &= ~(3U << (TEC_NSLEEP_PIN * 2));
    GPIO_MODER(pe_base) |= (GPIO_MODE_OUTPUT_PP << (TEC_NSLEEP_PIN * 2));
    
    /* NFAULT (PE15) - input, pull-up */
    GPIO_MODER(pe_base) &= ~(3U << (TEC_NFAULT_PIN * 2));
    GPIO_PUPDR(pe_base) |= (GPIO_PULLUP << (TEC_NFAULT_PIN * 2));
    
    /* IPROPI (PA0) - analog input for current sensing */
    uint32_t pa_base = GPIOA_BASE;
    GPIO_MODER(pa_base) &= ~(3U << (TEC_IPROPI_PIN * 2));
    GPIO_MODER(pa_base) |= (GPIO_MODE_ANALOG << (TEC_IPROPI_PIN * 2));
}

/* ============================================================
 * ADC configuration for current monitoring
 * ============================================================ */

static void tec_configure_adc(void)
{
    uint32_t adc_base = ADC1_BASE;
    
    /* Enable ADC clock via RCC - simplified for bare-metal */
    /* Configure ADC channel 0 for IPROPI pin */
    /* Set sample time to 15 cycles */
    REG32(adc_base + 0x0C) = 0;  /* Clear sample time */
    REG32(adc_base + 0x08) = (7U << 0);  /* Channel 0, 15 cycles */
    
    /* Enable ADC */
    REG32(adc_base + 0x08) |= (1U << 0);
}

static uint16_t tec_read_adc(void)
{
    uint32_t adc_base = ADC1_BASE;
    /* Start conversion */
    REG32(adc_base + 0x08) |= (1U << 2);
    /* Wait for conversion complete */
    while (!(REG32(adc_base + 0x08) & (1U << 2)));
    /* Read result */
    return (uint16_t)(REG32(adc_base + 0x40) & 0xFFF);
}

/* ============================================================
 * Public API
 * ============================================================ */

void tec_init(void)
{
    tec_configure_gpio();
    tec_configure_pwm();
    tec_configure_adc();
    
    /* Start in disabled state */
    GPIO_CLR(GPIOE_BASE, TEC_IN1_PIN);
    GPIO_CLR(GPIOE_BASE, TEC_IN2_PIN);
    GPIO_CLR(GPIOE_BASE, TEC_NSLEEP_PIN);  /* Sleep mode */
    
    current_duty = 0.0f;
    tec_enabled = false;
    tec_fault = false;
    soft_start_active = false;
}

void tec_enable(void)
{
    GPIO_SET(GPIOE_BASE, TEC_NSLEEP_PIN);  /* Wake up DRV8873 */
    tec_enabled = true;
}

void tec_disable(void)
{
    GPIO_CLR(GPIOE_BASE, TEC_NSLEEP_PIN);  /* Sleep mode */
    GPIO_CLR(GPIOE_BASE, TEC_IN1_PIN);
    GPIO_CLR(GPIOE_BASE, TEC_IN2_PIN);
    tec_set_pwm(0);
    current_duty = 0.0f;
    tec_enabled = false;
    soft_start_active = false;
}

void tec_set_duty(float duty_pct)
{
    /* Handle soft-start ramp */
    if (soft_start_active) {
        uint32_t now = systick_ms;
        if (now - soft_start_last_ms >= TEC_SOFT_START_STEP_MS) {
            soft_start_last_ms = now;
            soft_start_current += soft_start_step;
            if (soft_start_step > 0 && soft_start_current >= soft_start_target) {
                soft_start_current = soft_start_target;
                soft_start_active = false;
            } else if (soft_start_step < 0 && soft_start_current <= soft_start_target) {
                soft_start_current = soft_start_target;
                soft_start_active = false;
            }
            duty_pct = soft_start_current;
        } else {
            return;  /* Not time to update yet */
        }
    }
    
    /* Clamp duty */
    if (duty_pct > 100.0f) duty_pct = 100.0f;
    if (duty_pct < -100.0f) duty_pct = -100.0f;
    
    current_duty = duty_pct;
    
    if (!tec_enabled) {
        tec_enable();
    }
    
    /* Check for fault */
    if (!GPIO_READ(GPIOE_BASE, TEC_NFAULT_PIN)) {
        tec_fault = true;
        tec_disable();
        return;
    }
    tec_fault = false;
    
    uint32_t arr = TIM_ARR(TIM1_BASE);
    
    if (duty_pct > 0.0f) {
        /* Heating: IN1 = PWM, IN2 = LOW */
        GPIO_SET(GPIOE_BASE, TEC_IN1_PIN);
        GPIO_CLR(GPIOE_BASE, TEC_IN2_PIN);
        uint32_t duty_counts = (uint32_t)((duty_pct / 100.0f) * arr);
        tec_set_pwm(duty_counts);
    } else if (duty_pct < 0.0f) {
        /* Cooling: IN1 = LOW, IN2 = PWM */
        GPIO_CLR(GPIOE_BASE, TEC_IN1_PIN);
        GPIO_SET(GPIOE_BASE, TEC_IN2_PIN);
        uint32_t duty_counts = (uint32_t)((-duty_pct / 100.0f) * arr);
        tec_set_pwm(duty_counts);
    } else {
        /* Zero duty: coast */
        GPIO_CLR(GPIOE_BASE, TEC_IN1_PIN);
        GPIO_CLR(GPIOE_BASE, TEC_IN2_PIN);
        tec_set_pwm(0);
    }
}

float tec_get_duty(void)
{
    return current_duty;
}

bool tec_is_fault(void)
{
    return tec_fault || !GPIO_READ(GPIOE_BASE, TEC_NFAULT_PIN);
}

uint16_t tec_read_current_ma(void)
{
    /* DRV8873 IPROPI: 1 µA per 500 mA of load current
     * With a 1k resistor: 1mV per 500mA
     * ADC: 0-3.3V over 12 bits (0-4095)
     * current_mA = (adc_val / 4095) * 3300mV * (500mA / 1mV) / 1000
     *             = adc_val * 3300 * 500 / (4095 * 1000)
     *             = adc_val * 0.403
     */
    uint16_t adc_val = tec_read_adc();
    uint32_t current_ma = (uint32_t)(adc_val * 0.403f);
    return (uint16_t)current_ma;
}

void tec_soft_start(float target_duty, uint32_t ramp_ms)
{
    soft_start_target = target_duty;
    soft_start_current = current_duty;
    uint32_t steps = ramp_ms / TEC_SOFT_START_STEP_MS;
    if (steps == 0) steps = 1;
    soft_start_step = (target_duty - current_duty) / (float)steps;
    soft_start_last_ms = systick_ms;
    soft_start_active = true;
}