/*
 * lumen-tap/firmware/drivers/laser.c
 * Laser driver, PWM, and safety interlock implementation.
 *
 * The laser enable line (PB4) is AND-gated in hardware with the keyed
 * arming switch (PB3). This driver additionally enforces a software
 * Class-1 power cap and an ambient-light safety check via the TSL257.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

#include "laser.h"
#include "../board.h"
#include "../registers.h"

volatile ltm_laser_t g_laser;
volatile ltm_safety_t g_safety;

/* ------------------------------------------------------------------ */
/*  I2C helpers for TSL257                                            */
/* ------------------------------------------------------------------ */
static void i2c1_init(void)
{
    /* Enable I2C1 + GPIOB clocks */
    RCC_AHB4ENR  |= (1U << 1);   /* GPIOBEN */
    RCC_APB1LENR |= (1U << 21);  /* I2C1EN */

    /* PB7 = SDA (AF4), PA15 = SCL (AF4) */
    GPIOB->MODER  = (GPIOB->MODER  & ~(0x3 << (7 * 2)))  | (0x2 << (7 * 2));
    GPIOB->AFRL   = (GPIOB->AFRL   & ~(0xF << (7 * 4)))  | (0x4 << (7 * 4));
    GPIOB->OTYPER |= (1U << 7);   /* open-drain */
    GPIOB->PUPDR  = (GPIOB->PUPDR  & ~(0x3 << (7 * 2)))  | (0x1 << (7 * 2));

    GPIOA->MODER  = (GPIOA->MODER  & ~(0x3 << (15 * 2))) | (0x2 << (15 * 2));
    GPIOA->AFRH   = (GPIOA->AFRH   & ~(0xF << ((15 - 8) * 4))) |
                    (0x4 << ((15 - 8) * 4));
    GPIOA->OTYPER |= (1U << 15);
    GPIOA->PUPDR  = (GPIOA->PUPDR  & ~(0x3 << (15 * 2))) | (0x1 << (15 * 2));

    /* Timing for 100 kHz I2C from 120 MHz PCLK (approx) */
    I2C1->TIMINGR = (0x2 << 28) | (0xC << 20) | (0x12 << 16) |
                    (0x8B << 8)  | (0x0B << 0);
    I2C1->CR1 = I2C_CR1_PE;
}

static int i2c_write(uint8_t addr, const uint8_t *data, int n)
{
    I2C1->CR2 = ((uint32_t)addr << 1) | ((uint32_t)n << 16) | I2C_CR2_START;
    for (int i = 0; i < n; ++i) {
        uint32_t to = 100000;
        while (!(I2C1->ISR & I2C_ISR_TXE) && !(I2C1->ISR & I2C_ISR_NACKF) && --to) {}
        if (I2C1->ISR & I2C_ISR_NACKF) { I2C1->ICR = I2C_ISR_NACKF; return -1; }
        I2C1->TXDR = data[i];
    }
    to = 100000;
    while (!(I2C1->ISR & I2C_ISR_TC) && --to) {}
    I2C1->CR2 |= I2C_CR2_STOP;
    I2C1->ICR = I2C_ISR_TC;
    return 0;
}

static int i2c_read(uint8_t addr, uint8_t reg, uint8_t *out, int n)
{
    /* write 1-byte reg, then repeated-start read */
    uint8_t cmd = reg;
    if (i2c_write(addr, &cmd, 1) < 0) return -1;
    I2C1->CR2 = ((uint32_t)addr << 1) | I2C_CR2_RD_WRN |
                ((uint32_t)n << 16) | I2C_CR2_START;
    for (int i = 0; i < n; ++i) {
        uint32_t to = 100000;
        while (!(I2C1->ISR & I2C_ISR_RXNE) && !(I2C1->ISR & I2C_ISR_NACKF) && --to) {}
        if (I2C1->ISR & I2C_ISR_NACKF) { I2C1->ICR = I2C_ISR_NACKF; return -1; }
        out[i] = (uint8_t)I2C1->RXDR;
    }
    to = 100000;
    while (!(I2C1->ISR & I2C_ISR_TC) && --to) {}
    I2C1->CR2 |= I2C_CR2_STOP;
    I2C1->ICR = I2C_ISR_TC;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  TSL257 ambient-light sensor                                       */
/* ------------------------------------------------------------------ */
static void tsl257_init(void)
{
    uint8_t buf[2];
    buf[0] = TSL257_REG_ENABLE;  buf[1] = 0x03;  /* power + ALS enable */
    i2c_write(TSL257_I2C_ADDR, buf, 2);
    buf[0] = TSL257_REG_ATIME;  buf[1] = 0x80;   /* ~50 ms integration */
    i2c_write(TSL257_I2C_ADDR, buf, 2);
    buf[0] = TSL257_REG_CONTROL; buf[1] = 0x00;  /* 1x gain */
    i2c_write(TSL257_I2C_ADDR, buf, 2);
    buf[0] = TSL257_REG_PERS;   buf[1] = 0x00;   /* every cycle */
    i2c_write(TSL257_I2C_ADDR, buf, 2);
}

static uint16_t tsl257_read(void)
{
    uint8_t d[2] = {0, 0};
    if (i2c_read(TSL257_I2C_ADDR, TSL257_REG_C0DATA, d, 2) < 0)
        return 0xFFFF;  /* error → treat as unsafe (block emission) */
    return (uint16_t)d[0] | ((uint16_t)d[1] << 8);
}

/* ------------------------------------------------------------------ */
/*  PWM on TIM1_CH1 (PA8) to modulate the laser driver                */
/* ------------------------------------------------------------------ */
static void pwm_init(void)
{
    /* enable GPIOA + TIM1 clocks */
    RCC_AHB4ENR |= (1U << 0);    /* GPIOAEN */
    RCC_APB2ENR |= (1U << 0);    /* TIM1EN */

    /* PA8 = TIM1_CH1 (AF1) */
    GPIOA->MODER  = (GPIOA->MODER  & ~(0x3 << (8 * 2)))  | (0x2 << (8 * 2));
    GPIOA->AFRH   = (GPIOA->AFRH   & ~(0xF << ((8 - 8) * 4))) |
                    (0x1 << ((8 - 8) * 4));

    /* TIM1: 100 kHz PWM from 120 MHz APB2 timer clock */
    TIM1->PSC  = 0;
    TIM1->ARR  = (APB2_HZ / LTM_LASER_PWM_HZ) - 1;  /* 1199 */
    TIM1->CCR1 = 0;
    TIM1->CCMR1 = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM1->CCER  = TIM_CCER_CC1E;
    TIM1->CR1  = TIM_CR1_CEN | TIM_CR1_ARPE;
    /* main output enable (TIM1 advanced) */
    TIM1->BDTR |= (1U << 15);   /* MOE */
}

static inline void pwm_set_duty(uint8_t duty)
{
    uint32_t arr = TIM1->ARR + 1;
    uint32_t ccr = ((uint32_t)duty * arr) / 256U;
    if (ccr > arr) ccr = arr;
    TIM1->CCR1 = ccr;
}

/* ------------------------------------------------------------------ */
/*  Laser enable line (PB4) + arm key (PB3)                           */
/* ------------------------------------------------------------------ */
static void gpio_init_laser(void)
{
    RCC_AHB4ENR |= (1U << 1);    /* GPIOBEN */
    /* PB4 = output (laser EN) */
    GPIOB->MODER  = (GPIOB->MODER  & ~(0x3 << (4 * 2)))  | (0x1 << (4 * 2));
    GPIOB->OTYPER &= ~(1U << 4);
    GPIOB->OSPEEDR |= (0x3 << (4 * 2));
    GPIOB->ODR &= ~(1U << 4);    /* off by default */

    /* PB3 = input (arm key, pull-down) */
    GPIOB->MODER &= ~(0x3 << (3 * 2));
    GPIOB->PUPDR = (GPIOB->PUPDR & ~(0x3 << (3 * 2))) | (0x2 << (3 * 2));

    /* PB0, PB1 = LEDs */
    GPIOB->MODER  = (GPIOB->MODER  & ~(0x3 << (0 * 2)))  | (0x1 << (0 * 2));
    GPIOB->MODER  = (GPIOB->MODER  & ~(0x3 << (1 * 2)))  | (0x1 << (1 * 2));
}

static inline int arm_key_present(void)
{
    return (GPIOB->IDR & (1U << PIN_ARM_KEY)) ? 1 : 0;
}

static inline void laser_ena_line(int on)
{
    if (on) GPIOB->BSRR = (1U << PIN_LASER_EN);
    else    GPIOB->BSRR = (1U << (PIN_LASER_EN + 16));
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */
void laser_init(void)
{
    g_laser.enabled = 0;
    g_laser.pwm_duty = 0;
    g_laser.requested = 0;
    g_laser.class1_cap = 1;
    g_laser.ambient_cnt = 0;
    g_laser.on_time_ms = 0;

    g_safety.arm_key_hw = 0;
    g_safety.ambient_safe = 1;
    g_safety.sw_class1_cap = 1;
    g_safety.watchdog_ok = 1;
    g_safety.operator_override = 0;

    gpio_init_laser();
    i2c1_init();
    tsl257_init();
    pwm_init();
    laser_ena_line(0);
    pwm_set_duty(0);
}

void laser_set_power(uint8_t duty)
{
    g_laser.requested = duty;
    if (g_safety.sw_class1_cap && !g_safety.operator_override) {
        if (duty > LTM_LASER_PWR_MAX) {
            duty = LTM_LASER_PWR_MAX;
            g_laser.class1_cap = 1;
        } else {
            g_laser.class1_cap = 0;
        }
    } else {
        g_laser.class1_cap = 0;
    }
    g_laser.pwm_duty = duty;
    pwm_set_duty(duty);
    if (duty > 0 && board_laser_permit())
        laser_ena_line(1);
    else
        laser_ena_line(0);
}

void laser_arm(uint8_t arm)
{
    /* Operator arming is a SW-level concept; the HW arm key is independent. */
    if (!arm) {
        laser_emergency_off();
    }
}

void laser_update_ambient(void)
{
    uint16_t v = tsl257_read();
    g_laser.ambient_cnt = v;
    g_safety.ambient_safe = (v == 0xFFFF || v < LTM_AMBIENT_BLOCK_CNT) ? 1 : 0;
    /* if ambient is unsafe, immediately cut emission */
    if (!g_safety.ambient_safe) laser_emergency_off();
}

int laser_is_emitting(void)
{
    return (GPIOB->ODR & (1U << PIN_LASER_EN)) ? 1 : 0;
}

void laser_emergency_off(void)
{
    pwm_set_duty(0);
    laser_ena_line(0);
    g_laser.pwm_duty = 0;
    g_laser.enabled = 0;
}

void laser_tick_ms(void)
{
    static uint16_t ambient_div = 0;
    /* refresh ambient sensor every 50 ms */
    if (++ambient_div >= 50) {
        ambient_div = 0;
        laser_update_ambient();
    }
    g_safety.arm_key_hw = arm_key_present();
    /* accumulate on-time */
    if (laser_is_emitting())
        g_laser.on_time_ms++;
    /* LED status: green = armed & safe, red = fault */
    int green = (g_safety.arm_key_hw && g_safety.ambient_safe &&
                 g_safety.watchdog_ok);
    int red = !g_safety.ambient_safe || !g_safety.watchdog_ok;
    board_led_set(green, red);
}

/* Defined here to keep board.h API together. */
void board_led_set(int green, int red)
{
    if (green) GPIOB->BSRR = (1U << PIN_LED_STATUS);
    else       GPIOB->BSRR = (1U << (PIN_LED_STATUS + 16));
    if (red)   GPIOB->BSRR = (1U << PIN_LED_FAULT);
    else       GPIOB->BSRR = (1U << (PIN_LED_FAULT + 16));
}

void board_fault(void)
{
    laser_emergency_off();
    for (;;) {
        board_led_set(0, 1);
        /* hang; watchdog will reset if enabled */
    }
}

int board_laser_permit(void)
{
    return (g_safety.arm_key_hw &&
            g_safety.ambient_safe &&
            g_safety.sw_class1_cap || g_safety.operator_override) &&
            g_safety.watchdog_ok;
}