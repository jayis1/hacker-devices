/*
 * laser_driver.c - 450nm laser diode pulse control with safety interlocks
 *
 * Drives a 1W 450nm laser diode via LM359 constant-current driver.
 * Includes hardware safety shutter (fail-safe closed) and interlock.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#include "laser_driver.h"
#include "board.h"
#include "registers.h"

static bool laser_armed = false;
static bool shutter_open = false;
static float last_power_pct = 0.0f;
static uint32_t last_fire_ms = 0;
static uint32_t fire_end_ms = 0;
static bool laser_active = false;

extern volatile uint32_t systick_ms;

/* ============================================================
 * GPIO and timer configuration
 * ============================================================ */

static void laser_configure_gpio(void)
{
    uint32_t pd_base = GPIOD_BASE;
    
    /* EN pin (PD12) - output, push-pull */
    GPIO_MODER(pd_base) &= ~(3U << (LASER_EN_PIN * 2));
    GPIO_MODER(pd_base) |= (GPIO_MODE_OUTPUT_PP << (LASER_EN_PIN * 2));
    GPIO_OSPEEDR(pd_base) |= (GPIO_SPEED_HIGH << (LASER_EN_PIN * 2));
    
    /* Shutter pin (PD13) - output, push-pull (HIGH=open, LOW=closed) */
    GPIO_MODER(pd_base) &= ~(3U << (LASER_SHUTTER_PIN * 2));
    GPIO_MODER(pd_base) |= (GPIO_MODE_OUTPUT_PP << (LASER_SHUTTER_PIN * 2));
    
    /* Interlock pin (PD14) - input, pull-up
     * LOW = interlock engaged (safe, laser disabled)
     * HIGH = interlock OK (laser can fire) */
    GPIO_MODER(pd_base) &= ~(3U << (LASER_INTERLOCK_PIN * 2));
    GPIO_PUPDR(pd_base) |= (GPIO_PULLUP << (LASER_INTERLOCK_PIN * 2));
}

static void laser_configure_pwm(void)
{
    uint32_t tim_base = TIM2_BASE;
    uint32_t timer_clk = BOARD_APB1_FREQ_HZ;
    uint32_t arr = (timer_clk / LASER_PWM_FREQ_HZ) - 1;
    
    TIM_PSC(tim_base) = 0;
    TIM_ARR(tim_base) = arr;
    TIM_CCMR1(tim_base) = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM_CCER(tim_base) = TIM_CCER_CC1E;
    TIM_CCR1(tim_base) = 0;
    TIM_CR1(tim_base) = TIM_CR1_ARPE | TIM_CR1_CEN;
}

static void laser_set_pwm(float power_pct)
{
    uint32_t arr = TIM_ARR(TIM2_BASE);
    uint32_t duty = (uint32_t)((power_pct / 100.0f) * arr);
    if (duty > arr) duty = arr;
    TIM_CCR1(TIM2_BASE) = duty;
}

/* ============================================================
 * Public API
 * ============================================================ */

void laser_init(void)
{
    laser_configure_gpio();
    laser_configure_pwm();
    
    /* Start with laser disabled and shutter closed (fail-safe) */
    GPIO_CLR(GPIOD_BASE, LASER_EN_PIN);
    GPIO_CLR(GPIOD_BASE, LASER_SHUTTER_PIN);
    laser_set_pwm(0);
    
    laser_armed = false;
    shutter_open = false;
    laser_active = false;
}

bool laser_is_interlock_ok(void)
{
    /* Interlock pin HIGH = OK */
    return GPIO_READ(GPIOD_BASE, LASER_INTERLOCK_PIN) != 0;
}

bool laser_open_shutter(void)
{
    /* Safety: can only open shutter if interlock is engaged */
    if (!laser_is_interlock_ok()) {
        return false;
    }
    GPIO_SET(GPIOD_BASE, LASER_SHUTTER_PIN);
    shutter_open = true;
    return true;
}

bool laser_close_shutter(void)
{
    GPIO_CLR(GPIOD_BASE, LASER_SHUTTER_PIN);
    shutter_open = false;
    return true;
}

bool laser_is_shutter_open(void)
{
    return shutter_open;
}

bool laser_fire(float power_pct, uint32_t duration_ms)
{
    /* Safety checks */
    if (!laser_is_interlock_ok()) {
        return false;
    }
    
    if (!shutter_open) {
        if (!laser_open_shutter()) {
            return false;
        }
    }
    
    /* Clamp power */
    if (power_pct < 0.0f) power_pct = 0.0f;
    if (power_pct > LASER_MAX_POWER_PCT) power_pct = LASER_MAX_POWER_PCT;
    
    /* Clamp duration */
    if (duration_ms == 0) duration_ms = 1;
    if (duration_ms > LASER_MAX_PULSE_MS) duration_ms = LASER_MAX_PULSE_MS;
    
    /* Set power and enable */
    laser_set_pwm(power_pct);
    GPIO_SET(GPIOD_BASE, LASER_EN_PIN);
    
    last_power_pct = power_pct;
    last_fire_ms = systick_ms;
    fire_end_ms = systick_ms + duration_ms;
    laser_active = true;
    laser_armed = true;
    
    return true;
}

void laser_disable(void)
{
    GPIO_CLR(GPIOD_BASE, LASER_EN_PIN);
    laser_set_pwm(0.0f);
    laser_close_shutter();
    laser_active = false;
    laser_armed = false;
}

uint32_t laser_get_last_fire_ms(void)
{
    return last_fire_ms;
}

float laser_get_power_pct(void)
{
    return last_power_pct;
}

/* Called from main loop to check if laser pulse should end */
bool laser_check_timeout(void)
{
    if (laser_active && systick_ms >= fire_end_ms) {
        GPIO_CLR(GPIOD_BASE, LASER_EN_PIN);
        laser_set_pwm(0.0f);
        laser_close_shutter();
        laser_active = false;
        return true;  /* Laser just timed out */
    }
    return false;
}