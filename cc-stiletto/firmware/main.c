/*
 * main.c — CC-Stiletto firmware entry point.
 *
 * Initializes the clock tree, GPIO, two FUSB302B PD PHYs, the power path, the
 * console, and the policy engine, then runs the main loop: poll both PHYs for
 * incoming PD messages, dispatch them to the active attack policy, poll the
 * console for commands, and run the per-millisecond policy tick.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * This firmware is part of CC-Stiletto, a USB-C Power Delivery Configuration
 * Channel attack tool intended solely for authorized security research.
 */

#include "board.h"
#include "registers.h"
#include "pd_phy.h"
#include "pd_stack.h"
#include "policy_engine.h"
#include "power_path.h"
#include "console.h"
#include <string.h>

/* Global SysTick cycle counter (incremented every 1 ms). */
volatile uint32_t g_cycles = 0;
volatile uint32_t g_ms     = 0;

/* Globals referenced by other modules */
policy_ctx_t g_policy;
pd_id_t      g_ids;
pd_phy_t     g_phy_src;    /* toward source/charger */
pd_phy_t     g_phy_snk;    /* toward sink/DUT */

/* ---- Clock tree: HSI 16 MHz -> PLL -> 170 MHz ----------------------------- */
static void clock_init(void)
{
    /* Enable HSI and wait until ready */
    RCC->CR |= (1u << 8);                       /* HSION */
    while (!(RCC->CR & (1u << 10))) { }         /* HSIRDY */
    /* Configure PLL: source = HSI16, M=1, N=85, R=2  => 16/1*85/2 = 680 MHz? 
     * Use the documented G474 setting: PLLM=1, PLLN=17, PLLR=2 => 16/1*17/2=136? 
     * Correct for SYSCLK=170: HSI16 / PLLM(1) * PLLN(85) / PLLR(4) = 340? 
     * Use: PLLM=2, PLLN=85, PLLR=4 => 16/2*85/4 = 170 MHz. */
    RCC->CFGR &= ~(0xFu << 4);                   /* PLLM = 2 */
    RCC->CFGR |=  (1u << 4);
    RCC->CFGR &= ~(0x7Fu << 8);                  /* PLLN = 85 */
    RCC->CFGR |=  (85u << 8);
    RCC->CFGR &= ~(0x3u << 25);                  /* PLLR = 4 */
    RCC->CFGR |=  (3u << 25);
    RCC->CR |= (1u << 24);                       /* PLLON */
    while (!(RCC->CR & (1u << 25))) { }          /* PLLRDY */
    /* Switch system clock to PLL */
    RCC->CFGR &= ~(0x3u << 0);
    RCC->CFGR |=  (0x3u << 0);                   /* SW = PLL */
    while ((RCC->CFGR & (0x3u << 2)) != (0x3u << 2)) { } /* SWS = PLL */
}

/* ---- GPIO enable --------------------------------------------------------- */
static void gpio_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN
                  | RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIOFEN;

    /* I2C1 pins (PB6 SCL, PB7 SDA) — alt function 4, open-drain, pull-up */
    GPIOB->MODER &= ~(0x3u << (6*2));  GPIOB->MODER |= (GPIO_MODE_AF << (6*2));
    GPIOB->MODER &= ~(0x3u << (7*2));  GPIOB->MODER |= (GPIO_MODE_AF << (7*2));
    GPIOB->OTYPER |= (GPIO_OTYPE_OD << 6) | (GPIO_OTYPE_OD << 7);
    GPIOB->PUPDR  |= (GPIO_PUPD_PU << (6*2)) | (GPIO_PUPD_PU << (7*2));
    GPIOB->AFRL &= ~(0xFu << (6*4));   GPIOB->AFRL |= (4u << (6*4));
    GPIOB->AFRL &= ~(0xFu << (7*4));   GPIOB->AFRL |= (4u << (7*4));

    /* I2C2 pins (PB10 SCL, PB11 SDA) — AF4 */
    GPIOB->MODER &= ~(0x3u << (10*2)); GPIOB->MODER |= (GPIO_MODE_AF << (10*2));
    GPIOB->MODER &= ~(0x3u << (11*2)); GPIOB->MODER |= (GPIO_MODE_AF << (11*2));
    GPIOB->OTYPER |= (GPIO_OTYPE_OD << 10) | (GPIO_OTYPE_OD << 11);
    GPIOB->PUPDR  |= (GPIO_PUPD_PU << (10*2)) | (GPIO_PUPD_PU << (11*2));
    GPIOB->AFRL &= ~(0xFu << ((10-8)*4+16)); GPIOB->AFRH |= (4u << ((10-8)*4));
    GPIOB->AFRH &= ~(0xFu << ((11-8)*4));    GPIOB->AFRH |= (4u << ((11-8)*4));

    /* I2C3 pins (PC9 SDA, PC8 SCL) — AF8? on G474 I2C3_SDA=PC9, SCL=PC8. 
     * Use PA8 for SCL? per board.h we settled PC8/PC9. */
    GPIOC->MODER &= ~(0x3u << (8*2));  GPIOC->MODER |= (GPIO_MODE_AF << (8*2));
    GPIOC->MODER &= ~(0x3u << (9*2));  GPIOC->MODER |= (GPIO_MODE_AF << (9*2));
    GPIOC->OTYPER |= (GPIO_OTYPE_OD << 8) | (GPIO_OTYPE_OD << 9);
    GPIOC->PUPDR  |= (GPIO_PUPD_PU << (8*2)) | (GPIO_PUPD_PU << (9*2));
    GPIOC->AFRH &= ~(0xFu << ((8-8)*4));  GPIOC->AFRH |= (8u << ((8-8)*4));
    GPIOC->AFRH &= ~(0xFu << ((9-8)*4));  GPIOC->AFRH |= (8u << ((9-8)*4));

    /* FUSB interrupt pins as input (PB4, PA15) */
    GPIOB->MODER &= ~(0x3u << (4*2));    /* PB4 input */
    GPIOA->MODER &= ~(0x3u << (15*2));   /* PA15 input */

    /* eFuse EN (PC13) + FLT (PC14) + sink MOSFET (PC15) */
    GPIOC->MODER &= ~(0x3u << (13*2)); GPIOC->MODER |= (GPIO_MODE_OUTPUT << (13*2));
    GPIOC->MODER &= ~(0x3u << (14*2)); /* PC14 input */
    GPIOC->MODER &= ~(0x3u << (15*2)); GPIOC->MODER |= (GPIO_MODE_OUTPUT << (15*2));

    /* HRTIM CHA1 on PA8 — AF13 */
    GPIOA->MODER &= ~(0x3u << (8*2));  GPIOA->MODER |= (GPIO_MODE_AF << (8*2));
    GPIOA->AFRH  &= ~(0xFu << ((8-8)*4)); GPIOA->AFRH |= (13u << ((8-8)*4));

    /* Status LED pins (PA0/1/2) — output for now; PWM added later */
    for (uint8_t p = 0; p < 3; p++) {
        GPIOA->MODER &= ~(0x3u << (p*2));
        GPIOA->MODER |= (GPIO_MODE_OUTPUT << (p*2));
    }

    /* Button PB3 input with pull-up */
    GPIOB->MODER &= ~(0x3u << (3*2));
    GPIOB->PUPDR  |= (GPIO_PUPD_PU << (3*2));

    /* Isolator enable PB15 */
    GPIOB->MODER &= ~(0x3u << (15*2));
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << (15*2));
    GPIOB->BSRR  = (1u << 15);          /* enable isolator */

    /* ADC1 channel 5 (PA4) — analog mode */
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;
    GPIOA->MODER &= ~(0x3u << (4*2));
    GPIOA->MODER |= (GPIO_MODE_ANALOG << (4*2));
}

static void led_set(led_state_t s)
{
    /* Simple decode: red on PA0, green PA1, blue PA2 */
    uint8_t r = 0, g = 0, b = 0;
    switch (s) {
    case LED_OFF:        break;
    case LED_SNIFF:      g = 1; break;
    case LED_INJECT:     r = 1; break;
    case LED_SPOOF:      r = 1; b = 1; break;
    case LED_GLITCH:     r = 1; break;
    case LED_ROLE_HIJACK:g = 1; b = 1; break;
    case LED_VCONN:      r = 1; g = 1; break;
    case LED_DEAD_BATT:  b = 1; break;
    case LED_FAULT:      r = 1; break;
    }
    if (r) GPIOA->BSRR = (1u << 0); else GPIOA->BRR = (1u << 0);
    if (g) GPIOA->BSRR = (1u << 1); else GPIOA->BRR = (1u << 1);
    if (b) GPIOA->BSRR = (1u << 2); else GPIOA->BRR = (1u << 2);
}

/* ---- SysTick: 1 ms tick at 170 MHz -------------------------------------- */
static void systick_init(void)
{
    /* Reload for 1 ms at 170 MHz = 170000 - 1 */
    *(volatile uint32_t *)0xE000E014u = SYSCLK_FREQ_HZ / 1000u - 1u;
    *(volatile uint32_t *)0xE000E018u = 0;
    *(volatile uint32_t *)0xE000E010u = 0x7u;   /* CLKSOURCE=processor, ENABLE, TICKINT */
}

/* SysTick interrupt handler (weak alias) */
void SysTick_Handler(void)
{
    g_cycles += (SYSCLK_FREQ_HZ / 1000u);
    g_ms++;
}

/* ---- Button debouncer --------------------------------------------------- */
static bool button_pressed(void)
{
    static uint8_t debounce;
    bool level = (GPIOB->IDR & (1u << BTN_PIN)) == 0;   /* active low */
    if (level) { if (debounce < 10) debounce++; }
    else       { debounce = 0; }
    return debounce == 10;
}

/* ---- Main ---------------------------------------------------------------- */
int main(void)
{
    clock_init();
    gpio_init();
    systick_init();
    console_init();
    power_path_init();

    /* Set up the two PHY handles */
    g_phy_src.i2c       = I2C1;
    g_phy_src.int_port   = FUSB_SRC_INT_PORT;
    g_phy_src.int_pin    = FUSB_SRC_INT_PIN;
    g_phy_src.addr_w     = (uint8_t)(FUSB302_I2C_ADDR_7BIT << 1);
    g_phy_src.addr_r     = (uint8_t)((FUSB302_I2C_ADDR_7BIT << 1) | 1u);
    g_phy_src.data_role  = PD_ROLE_SINK;
    g_phy_src.power_role = PD_POWER_SINK;

    g_phy_snk.i2c       = I2C2;
    g_phy_snk.int_port   = FUSB_SNK_INT_PORT;
    g_phy_snk.int_pin    = FUSB_SNK_INT_PIN;
    g_phy_snk.addr_w     = (uint8_t)(FUSB302_I2C_ADDR_7BIT << 1);
    g_phy_snk.addr_r     = (uint8_t)((FUSB302_I2C_ADDR_7BIT << 1) | 1u);
    g_phy_snk.data_role  = PD_ROLE_SINK;
    g_phy_snk.power_role = PD_POWER_SINK;

    pd_stack_init(&g_ids);
    pd_phy_init(&g_phy_src);
    pd_phy_init(&g_phy_snk);

    policy_init(&g_policy, &g_phy_src, &g_phy_snk, &g_ids);
    policy_set(&g_policy, POL_SNIFF);
    led_set(LED_SNIFF);

    console_emit("boot CC-Stiletto by jayis1 v1.0");

    uint32_t last_tick = 0;
    bool     src_alive = pd_phy_detect(&g_phy_src);
    bool     snk_alive = pd_phy_detect(&g_phy_snk);
    if (!src_alive && !snk_alive) {
        console_emit("warn no FUSB302 detected on either side");
        led_set(LED_FAULT);
    }

    for (;;) {
        /* Poll both PHYs for incoming PD messages */
        pd_msg_t msg;
        if (pd_phy_recv(&g_phy_src, &msg) > 0)
            policy_dispatch_event(&g_policy, &g_phy_src, &msg);
        if (pd_phy_recv(&g_phy_snk, &msg) > 0)
            policy_dispatch_event(&g_policy, &g_phy_snk, &msg);

        /* Console */
        console_poll();

        /* Button: cycle modes, or emergency Hard Reset if held > 2 s */
        static uint32_t btn_down_ms;
        static bool     btn_prev;
        bool btn = button_pressed();
        if (btn && !btn_prev) btn_down_ms = g_ms;
        if (!btn && btn_prev) {
            if (g_ms - btn_down_ms > 2000u) {
                pd_phy_send_hard_reset(&g_phy_src);
                pd_phy_send_hard_reset(&g_phy_snk);
                console_emit("evt hard-reset (button hold)");
                led_set(LED_FAULT);
                for (volatile int d=0; d<200000; d++);
                led_set(LED_SNIFF);
            } else {
                policy_id_t next = (policy_id_t)((g_policy.active + 1u) % POL_COUNT);
                policy_set(&g_policy, next);
                led_set((led_state_t)(next + LED_SNIFF));
            }
        }
        btn_prev = btn;

        /* Per-ms policy tick + thermal guard */
        if (g_ms != last_tick) {
            last_tick = g_ms;
            policy_dispatch_tick(&g_policy, g_ms);

            /* Thermal / eFuse safety: force-disarm on fault */
            uint8_t t = power_path_read_temp();
            bool flt = power_path_efuse_fault();
            if (t > TEMP_OVER_LIMIT_C || flt) {
                policy_disarm(&g_policy);
                power_path_source_enable(false);
                led_set(LED_FAULT);
                console_emit("FAULT temp=%uC efuse=%d — disarmed", t, (int)flt);
            }

            /* Update telemetry snapshot used by the policy context */
            power_path_read_src(&g_policy.vbus_src_mv, &g_policy.ma_src);
            power_path_read_snk(&g_policy.vbus_snk_mv, &g_policy.ma_snk);
            g_policy.temp_c = t;
            g_policy.efuse_fault = flt;
        }
    }

    return 0;   /* unreachable */
}