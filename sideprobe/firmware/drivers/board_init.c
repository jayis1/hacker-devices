/*
 * drivers/board_init.c — STM32H743 clock, GPIO, FMC SDRAM, SysTick setup
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include "../board.h"
#include "../registers.h"

static volatile uint32_t g_systick_count = 0;

void SysTick_Handler(void) __attribute__((interrupt("IRQ")));
void SysTick_Handler(void) {
    g_systick_count++;
}

uint32_t SysTick_read(void) { return g_systick_count; }

/* ---- GPIO helper ---- */
static void gpio_config(uint32_t base, uint8_t pin, uint8_t mode,
                         uint8_t af, uint8_t speed) {
    uint32_t moder = GPIO_MODER(base);
    moder &= ~(3u << (pin * 2));
    moder |= (mode & 3u) << (pin * 2);
    GPIO_MODER(base) = moder;

    if (mode == GPIO_MODE_AF) {
        if (pin < 8) {
            uint32_t v = GPIO_AFRL(base);
            v &= ~(0xFu << (pin * 4));
            v |= (af & 0xFu) << (pin * 4);
            GPIO_AFRL(base) = v;
        } else {
            uint32_t v = GPIO_AFRH(base);
            v &= ~(0xFu << ((pin - 8) * 4));
            v |= (af & 0xFu) << ((pin - 8) * 4);
            GPIO_AFRH(base) = v;
        }
    }
    uint32_t os = GPIO_OSPEEDR(base);
    os &= ~(3u << (pin * 2));
    os |= (speed & 3u) << (pin * 2);
    GPIO_OSPEEDR(base) = os;
}

/* ---- SDRAM init via FMC ---- */
static void sdram_init(void) {
    /* Enable FMC clock */
    RCC_AHB3ENR |= RCC_AHB3ENR_FMC;
    sp_delay_us(10);

    /* Configure GPIO alternate functions for FMC (SDIO/SDRAM pins).
       PD0..PD15 = D0..D15, plus addr/control pins on PE. In a real build
       these come from the KiCad netlist; here we set up the common ones. */
    /* D0..D15 on PD0..PD15, AF12 (FMC) */
    for (uint8_t p = 0; p < 16; p++) {
        gpio_config(GPIOD_BASE, p, GPIO_MODE_AF, 12, 3);
    }
    /* Address lines A0..A11 on PE0..PE11 (AF12), plus control: PE7=NBL1,
       PE8=NBL0 (byte lanes), PE13 (SDNCAS), PE14 (SDNWE), PE15 (SDNCAS)...
       Simplified: set a representative set. */
    for (uint8_t p = 0; p <= 11; p++) {
        gpio_config(GPIOE_BASE, p, GPIO_MODE_AF, 12, 3);
    }
    gpio_config(GPIOD_BASE, 12, GPIO_MODE_AF, 12, 3); /* BA0 */
    gpio_config(GPIOD_BASE, 13, GPIO_MODE_AF, 12, 3); /* BA1 */
    gpio_config(GPIOD_BASE, 14, GPIO_MODE_AF, 12, 3); /* SDNWE */
    gpio_config(GPIOD_BASE, 15, GPIO_MODE_AF, 12, 3); /* SDNCAS */
    gpio_config(GPIOE_BASE, 12, GPIO_MODE_AF, 12, 3); /* A12 */

    /* FMC SDRAM control register 1 (bank 5/6 used) */
    /* SDCLK=2, CAS=2, 16-bit, 4 banks, 13 row, 9 col */
    FMC_SDCR1 = (1u << 13)   /* SDCLK: 010 = 2 clocks */
               | (2u << 7)    /* CAS latency 2 */
               | (0u << 6)    /* 16-bit */
               | (3u << 4)    /* 4 banks */
               | (0u << 2)    /* 13 rows */
               | (0u << 0);   /* 9 cols */
    FMC_SDCR2 = 0;             /* bank 2 unused */

    /* Timing register (simplified, for 100 MHz SDCLK) */
    FMC_SDTR1 = (2u << 0)    /* TMRD = 2 */
              | (7u << 4)    /* TXSR = 7 */
              | (2u << 8)    /* TRAS = 2 */
              | (2u << 12)   /* TRC = 2 */
              | (2u << 16)   /* TWR = 2 */
              | (2u << 20)   /* TRP = 2 */
              | (2u << 24);  /* TRCD = 2 */

    /* SDRAM init sequence: enable clock -> precharge -> auto-refresh -> mode */
    FMC_SDCMR = (1u << 0) | (1u << 3);   /* START = 1, CMD = 1 (init) */
    sp_delay_us(100);
    FMC_SDCMR = (1u << 0) | (2u << 3) | (7u << 5);  /* PRECHARGE, count 7 */
    sp_delay_us(10);
    FMC_SDCMR = (1u << 0) | (3u << 3) | (8u << 5);  /* AUTO-REFRESH, 8 cycles */
    sp_delay_us(10);
    /* Load mode register: burst=1, CAS=2 */
    FMC_SDCMR = (1u << 0) | (4u << 3) | (0x230u << 9);
    sp_delay_us(10);
}

/* ---- System clock setup ---- */
static void clocks_init(void) {
    /* Enable HSE (25 MHz) */
    RCC_CR |= (1u << 16); /* HSEON */
    while (!(RCC_CR & (1u << 17))) { } /* wait HSERDY */

    /* Configure PWR supply: VOS1 for 480 MHz */
    PWR_CR3 |= (1u << 1); /* SDEN (switching regulator enable) */
    while (!(PWR_CR3 & PWR_CR3_SCUEN)) { }

    /* Flash latency for 480 MHz: 7 wait states, high-freq 1 */
    FLASH_ACR = FLASH_ACR_LATENCY(7) | (1u << 4);

    /* PLL1: HSE/5 * 96 /2 = 480 MHz (VCO 480*5=2400, /M=5, *N=96, /P=2) */
    RCC_PLLCKSELR = (5u << 0);            /* PLL1M = 5 (25/5=5 MHz ref) */
    RCC_PLL1CFGR  = (96u << 8)            /* PLL1N = 96 -> VCO 480 MHz */
                  | (0u << 0);            /* PLL1SRC = HSE */
    RCC_PLL1DIVR  = (1u << 0)             /* PLL1P = 1+1 = 2 -> 480 MHz */
                  | (3u << 8)             /* PLL1Q = 3+1 = 4 -> 240 MHz */
                  | (4u << 16);           /* PLL1R = 4+1 = 5 -> 192 MHz */
    RCC_CR |= (1u << 24);                 /* PLL1ON */
    while (!(RCC_CR & (1u << 25))) { }    /* PLL1RDY */

    /* Switch SYSCLK to PLL1 */
    RCC_CFGR = (3u << 0);  /* SW = PLL1 */
    while (((RCC_CFGR >> 3) & 3) != 3) { } /* wait SWS */

    /* AHB/APB dividers: AHB/2 = 240, APB1,APB2 /2 = 120 */
    RCC_CFGR |= (8u << 4)  /* HPRE = /2 (1008/2 = 504... adjusted for H7 = 240) */
             |  (4u << 10) /* PPRE1 = /2 -> 120 */
             |  (4u << 13);/* PPRE2 = /2 -> 120 */

    /* Enable peripheral clocks */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOA | RCC_AHB1ENR_GPIOB |
                   RCC_AHB1ENR_GPIOC | RCC_AHB1ENR_GPIOD |
                   RCC_AHB1ENR_GPIOE | RCC_AHB1ENR_DMA1  | RCC_AHB1ENR_DMA2;
    RCC_AHB2ENR |= RCC_AHB2ENR_OTG;
    RCC_AHB3ENR |= RCC_AHB3ENR_SDMMC1;
    RCC_APB1ENR |= RCC_APB1ENR_USART2 | RCC_APB1ENR_I2C1 | RCC_APB1ENR_SPI2;
    RCC_APB2ENR |= RCC_APB2ENR_USART1 | RCC_APB2ENR_SPI1;

    sp_delay_us(10);
}

void board_init(void) {
    clocks_init();

    /* SysTick 1 ms at 480 MHz -> reload 480 000-1 */
    *(volatile uint32_t *)(0xE000E010) = 0;          /* CTRL = 0 */
    *(volatile uint32_t *)(0xE000E014) = 479999u;    /* LOAD */
    *(volatile uint32_t *)(0xE000E018) = 0;          /* VAL */
    *(volatile uint32_t *)(0xE000E010) = 7;          /* ENABLE | TICKINT | CLKSOURCE */

    sdram_init();

    /* Configure button inputs (PA0..PA4) with pull-ups */
    gpio_config(GPIOA_BASE, 0, GPIO_MODE_IN, 0, 0);
    gpio_config(GPIOA_BASE, 1, GPIO_MODE_IN, 0, 0);
    gpio_config(GPIOA_BASE, 2, GPIO_MODE_IN, 0, 0);
    gpio_config(GPIOA_BASE, 3, GPIO_MODE_IN, 0, 0);
    gpio_config(GPIOA_BASE, 4, GPIO_MODE_IN, 0, 0);
    for (uint8_t p = 0; p < 5; p++) {
        uint32_t pup = GPIO_PUPDR(GPIOA_BASE);
        pup &= ~(3u << (p * 2));
        pup |= (1u << (p * 2)); /* pull-up */
        GPIO_PUPDR(GPIOA_BASE) = pup;
    }

    /* Trigger-out pin (PC14) as push-pull output */
    gpio_config(GPIOC_BASE, 14, GPIO_MODE_OUT, 0, 2);
    GPIO_BSRR(GPIOC_BASE) = (1u << (14 + 16)); /* set low */

    /* Trigger-in (PC13) as input with pull-down */
    gpio_config(GPIOC_BASE, 13, GPIO_MODE_IN, 0, 0);
    uint32_t pup = GPIO_PUPDR(GPIOC_BASE);
    pup &= ~(3u << 26);
    pup |= (2u << 26); /* pull-down */
    GPIO_PUPDR(GPIOC_BASE) = pup;

    /* Enable SYSCFG clock for EXTI */
    *(volatile uint32_t *)(RCC_BASE + 0x14) |= (1u << 0); /* APB4 SYSCFG */
}