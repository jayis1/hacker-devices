/*
 * board_init.c — early hardware bring-up for CoilGrip
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Configures the STM32H743 clocks, GPIOs, and the high-resolution timer
 * (HRTIM) that generates the 140 kHz Qi PWM. Other peripherals are
 * initialised lazily by their drivers on first use.
 */

#include "board.h"
#include "registers.h"

/* ---- Clocks ------------------------------------------------------------ */
/* HSE 25 MHz crystal -> PLL1 -> 480 MHz core, 240 MHz APX clocks.
 * The Qi PWM and ADC derive from the high-resolution timer running at
 * 8x the timer clock (≈ 3.844 GHz effective), giving sub-ns jitter. */

/* ---- Clocks ------------------------------------------------------------ */

static void rcc_setup(void)
{
    /* Enable HSE and wait for it */
    RCC->CR |= BIT(16);  /* HSEON */
    while (!(RCC->CR & BIT(17))) { }  /* HSERDY */

    /* Enable HSI as backup; we already have it running */
    RCC->CR |= BIT(0);
    while (!(RCC->CR & BIT(1))) { }

    /* Voltage scale 0 (VOS0) for 480 MHz core */
    PWR->VOSR |= BIT(0) | BIT(1);
    while (!(PWR->VOSR & BIT(15))) { }  /* VOS0RDY */

    /* PLL1: HSE / 5 * 192 / 2 = 25/5 * 192 / 2 = 960 MHz VCO / 2 = 480 MHz */
    RCC->CKCFGR = (5U << 0)    /* PLL1M = 5 */
                | (192U << 8)  /* PLL1N = 192 */
                | (1U << 24);  /* PLL1P EN */
    REG32((uint32_t)RCC + 0x28) = 0;  /* PLL1Q/R off */
    RCC->CR |= BIT(24);  /* PLL1ON */
    while (!(RCC->CR & BIT(25))) { }  /* PLL1RDY */

    /* D1CFGR: HCLK = SYSCLK / 2 = 240, APB1/2 = HCLK / 2 = 120,
     * APB3/4 = HCLK / 2. */
    RCC->D1CFGR = (8U << 0)   /* HPRE /2 (D1CPRE = 0b1000 = /2) */
                | (4U << 4)   /* D1PPRE /2 (APB3) */
                | (4U << 8);  /* D1PPRE /2 (APB1) */

    /* D2CFGR: D2PPRE1 (APB1 alt) /2, D2PPRE2 (APB2) /2 */
    RCC->D2CFGR = (4U << 0)
                | (4U << 8);

    /* D3CFGR: D3PPRE (APB4) /2 */
    RCC->D3CFGR = (4U << 0);

    /* CFGR.SW = PLL1 */
    RCC->CFGR = (RCC->CFGR & ~0x3U) | 2U;
    while (((RCC->CFGR >> 3) & 0x7U) != 2U) { }
}

/* ---- Peripheral enables ------------------------------------------------- */

static void rcc_enable_periphs(void)
{
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN
                  | RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN
                  | RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOHEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    RCC->AHB3ENR |= RCC_AHB3ENR_QSPIEN;
    RCC->APB1LENR |= RCC_APB1LENR_SPI2EN | RCC_APB1LENR_I2C1EN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_TIM2EN
                  | RCC_APB2ENR_HRTIM1EN;
    RCC->APB1HENR |= RCC_APB1HENR_USART3EN;
    RCC->AHB4ENR |= RCC_AHB4ENR_ADC12EN;
}

/* ---- GPIO configuration ------------------------------------------------- */

static void gpio_setup(void)
{
    /* PA0 — LOAD_MOD_DRV, output */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_PA0 * 2))) | (GPIO_MODE_OUTPUT_PP << (PIN_PA0 * 2));
    GPIOA->BSRR = BIT(PIN_PA0 + 16);  /* set low (load mod off) */

    /* PA1 — GLITCH_FET_EN, output (H-bridge enable override) */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_PA1 * 2))) | (GPIO_MODE_OUTPUT_PP << (PIN_PA1 * 2));
    GPIOA->BSRR = BIT(PIN_PA1 + 16);

    /* PA2 — TRIG_IN0 input; PA3 — TRIG_OUT0 output; PA4 — TRIG_IN1; PA5 — TRIG_OUT1 */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_PA2 * 2))) | (GPIO_MODE_INPUT << (PIN_PA2 * 2));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_PA3 * 2))) | (GPIO_MODE_OUTPUT_PP << (PIN_PA3 * 2));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_PA4 * 2))) | (GPIO_MODE_INPUT << (PIN_PA4 * 2));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_PA5 * 2))) | (GPIO_MODE_OUTPUT_PP << (PIN_PA5 * 2));

    /* PA6 — TMP117_ALERT, input pull-up */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_PA6 * 2))) | (GPIO_MODE_INPUT << (PIN_PA6 * 2));
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3U << (PIN_PA6 * 2))) | (1U << (PIN_PA6 * 2));

    /* PA8 — HRTIM1 Master ChA output (AF13 on H7) */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_PA8 * 2))) | (GPIO_MODE_AF << (PIN_PA8 * 2));
    GPIOA->AFRH = (GPIOA->AFRH & ~(0xFU << ((PIN_PA8 - 8) * 4))) | (13U << ((PIN_PA8 - 8) * 4));
    GPIOA->OSPEEDR |= (3U << (PIN_PA8 * 2));

    /* PA9/PA10 — USART3 debug (AF7) */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_PA9 * 2))) | (GPIO_MODE_AF << (PIN_PA9 * 2));
    GPIOA->AFRH = (GPIOA->AFRH & ~(0xFU << ((PIN_PA9 - 8) * 4))) | (7U << ((PIN_PA9 - 8) * 4));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_PA10 * 2))) | (GPIO_MODE_AF << (PIN_PA10 * 2));
    GPIOA->AFRH = (GPIOA->AFRH & ~(0xFU << ((PIN_PA10 - 8) * 4))) | (7U << ((PIN_PA10 - 8) * 4));

    /* PB6/PB7 — I2C1 to BQ51013B (AF4) */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (PIN_PB6 * 2))) | (GPIO_MODE_AF << (PIN_PB6 * 2));
    GPIOB->AFRL = (GPIOB->AFRL & ~(0xFU << (PIN_PB6 * 4))) | (4U << (PIN_PB6 * 4));
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (PIN_PB7 * 2))) | (GPIO_MODE_AF << (PIN_PB7 * 2));
    GPIOB->AFRL = (GPIOB->AFRL & ~(0xFU << (PIN_PB7 * 4))) | (4U << (PIN_PB7 * 4));
    GPIOB->OTYPER |= BIT(PIN_PB6) | BIT(PIN_PB7);  /* open-drain */
    GPIOB->PUPDR |= (1U << (PIN_PB6 * 2)) | (1U << (PIN_PB7 * 2));

    /* PB10/PB12/PB14/PB15 — SPI2 to MWCT1011 (AF5) */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (PIN_PB10 * 2))) | (GPIO_MODE_AF << (PIN_PB10 * 2));
    GPIOB->AFRL = (GPIOB->AFRL & ~(0xFU << (PIN_PB10 * 4))) | (5U << (PIN_PB10 * 4));
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (PIN_PB12 * 2))) | (GPIO_MODE_OUTPUT_PP << (PIN_PB12 * 2));  /* NSS manual */
    GPIOB->BSRR = BIT(PIN_PB12);  /* NSS high (deselected) */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (PIN_PB14 * 2))) | (GPIO_MODE_AF << (PIN_PB14 * 2));
    GPIOB->AFRL = (GPIOB->AFRL & ~(0xFU << (PIN_PB14 * 4))) | (5U << (PIN_PB14 * 4));
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (PIN_PB15 * 2))) | (GPIO_MODE_AF << (PIN_PB15 * 2));
    GPIOB->AFRL = (GPIOB->AFRL & ~(0xFU << (PIN_PB15 * 4))) | (5U << (PIN_PB15 * 4));

    /* PC4/PC5 — ADC1 inputs (analog) */
    GPIOC->MODER = (GPIOC->MODER & ~(3U << (PIN_PC4 * 2))) | (GPIO_MODE_ANALOG << (PIN_PC4 * 2));
    GPIOC->MODER = (GPIOC->MODER & ~(3U << (PIN_PC5 * 2))) | (GPIO_MODE_ANALOG << (PIN_PC5 * 2));

    /* PE0 — FPGA_INT, input; PE1 — FPGA_CS, output (SPI1 NSS manual); PE2 — FPGA_RESET, output */
    GPIOE->MODER = (GPIOE->MODER & ~(3U << (PIN_PE0 * 2))) | (GPIO_MODE_INPUT << (PIN_PE0 * 2));
    GPIOE->PUPDR |= (1U << (PIN_PE0 * 2));  /* pull-up */
    GPIOE->MODER = (GPIOE->MODER & ~(3U << (PIN_PE1 * 2))) | (GPIO_MODE_OUTPUT_PP << (PIN_PE1 * 2));
    GPIOE->BSRR = BIT(PIN_PE1);  /* CS high (deselected) */
    GPIOE->MODER = (GPIOE->MODER & ~(3U << (PIN_PE2 * 2))) | (GPIO_MODE_OUTPUT_PP << (PIN_PE2 * 2));
    GPIOE->BSRR = BIT(PIN_PE2 + 16);  /* reset low */

    /* PE5/PE6 — USART2 (BLE), AF7 */
    GPIOE->MODER = (GPIOE->MODER & ~(3U << (PIN_PE5 * 2))) | (GPIO_MODE_AF << (PIN_PE5 * 2));
    GPIOE->AFRL = (GPIOE->AFRL & ~(0xFU << (PIN_PE5 * 4))) | (7U << (PIN_PE5 * 4));
    GPIOE->MODER = (GPIOE->MODER & ~(3U << (PIN_PE6 * 2))) | (GPIO_MODE_AF << (PIN_PE6 * 2));
    GPIOE->AFRL = (GPIOE->AFRL & ~(0xFU << (PIN_PE6 * 4))) | (7U << (PIN_PE6 * 4));

    /* PB5 — OLED_CS, output (uses SPI3 bit-banged SDA on PB8/PB9) */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (PIN_PB5 * 2))) | (GPIO_MODE_OUTPUT_PP << (PIN_PB5 * 2));
    GPIOB->BSRR = BIT(PIN_PB5);  /* CS high */
    /* PA15 — OLED_DC, output */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_PA15 * 2))) | (GPIO_MODE_OUTPUT_PP << (PIN_PA15 * 2));

    /* PB8 — OLED_SCL (bit-banged), output; PB9 — OLED_SDA (bit-banged), output */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (PIN_PB8 * 2))) | (GPIO_MODE_OUTPUT_PP << (PIN_PB8 * 2));
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (PIN_PB9 * 2))) | (GPIO_MODE_OUTPUT_PP << (PIN_PB9 * 2));
}

/* ---- USART3 debug console ----------------------------------------------- */

static void usart3_setup(void)
{
    USART3->BRR = (APB_HZ / 115200U);  /* 1042 ≈ 120 MHz / 115200 */
    USART3->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

/* ---- HRTIM PWM for Qi TX (140 kHz, 50% duty) ---------------------------- */

static void hrtim_setup(void)
{
    /* Master counter clocked at HRTIM_CLK_HZ * 8 = 1.92 GHz effective
     * (8x high-resolution multiplier). Period for 140 kHz:
     *   PER = (1.92e9 / 140e3) = 13714 counts
     * Compare at 50% duty: CMP1 = 6857. */
    uint32_t per = (HRTIM_CLK_HZ * 8U) / 140000U;  /* 13714 */
    HRTIM1->PERER = per;
    HRTIM1->CMP1ER = per / 2U;
    HRTIM1->CMP2ER = per / 4U;  /* not used; placeholder for deadtime */
    /* Enable master counter */
    HRTIM1->MCR = HRTIM_MCR_MCEN | HRTIM_MCR_TBDMEN;
}

/* ---- ADC1 for current/voltage sensing (polling) ------------------------ */

static void adc1_setup(void)
{
    /* Deep power-down exit */
    ADC1->CR &= ~ADC_CR_ADEN;
    /* Calibration (single-ended) */
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) { }
    /* Enable */
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }
    /* Configure: 8-bit right-aligned, single channel, continuous off,
     * channel 4 (PC4) as injected for now; sampling time 64.5 cycles. */
    ADC1->CFGR = 0;  /* 12-bit, right-aligned, single conversion, SW trigger */
    ADC1->SMPR1 = (5U << 12) | (5U << 15);  /* 64.5 cycles for ch4, ch5 */
}

/* ---- Public entry ------------------------------------------------------- */

void board_init(void)
{
    /* 1. exception/vector table is set by startup; nothing to do here. */
    /* 2. clocks */
    rcc_setup();
    /* 3. enable peripheral clocks */
    rcc_enable_periphs();
    /* 4. pin mux */
    gpio_setup();
    /* 5. USART3 for debug log */
    usart3_setup();
    /* 6. HRTIM for Qi PWM (not enabled until qi_tx_start) */
    hrtim_setup();
    /* 7. ADC1 for sensing */
    adc1_setup();
    /* 8. Vector table is already at 0x08000000 by default. */
}

/* ---- Small utility: blocking debug print ------------------------------- */
/* USART3 is used as a printf-like debug console. We avoid pulling in
 * the full C library printf to keep the firmware deterministic. */

void debug_putc(char c)
{
    while (!(USART3->ISR & USART_ISR_TXE)) { }
    USART3->TDR = (uint32_t)c;
}

void debug_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            debug_putc('\r');
        debug_putc(*s++);
    }
}

/* Minimal hex/decimal formatters for log output */
void debug_put_u(uint32_t v)
{
    char buf[12];
    int i = 0;
    if (v == 0) { debug_putc('0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i) debug_putc(buf[--i]);
}

void debug_put_x(uint32_t v)
{
    char buf[12];
    const char hex[] = "0123456789ABCDEF";
    int i = 0;
    if (v == 0) { debug_putc('0'); return; }
    while (v) { buf[i++] = hex[v & 0xF]; v >>= 4; }
    while (i) debug_putc(buf[--i]);
}