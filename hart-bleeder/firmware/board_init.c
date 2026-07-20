/*
 * hart-bleeder — board_init.c
 * MCU clock tree, GPIO, ADC, DAC, timers and interrupt controller
 * setup for the HART Fieldbus Covert In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"
#include "modem_drv.h"
#include "oled_drv.h"

/* ── External symbol for system tick handler ────────────────── */
extern volatile uint32_t s_tick;
volatile uint32_t s_tick;

/* ── Clock tree: HSE 24 MHz -> PLL -> 170 MHz ────────────────── */
static void clock_init(void) {
    /* Enable HSE and wait for ready */
    RCC_CR |= RCC_CR_HSEON;
    while (!(RCC_CR & RCC_CR_HSERDY)) { }

    /* Configure PLL: source = HSE, M=6, N=85, R=2 => 24/6*85/2 = 170 MHz */
    RCC_PLLCFGR = (1U << 0)            /* PLLSRC = HSE */
                | (6U << 4)            /* M = 6 */
                | (85U << 8)           /* N = 85 */
                | (0U << 16)           /* P = 2 (00) */
                | (7U << 24)           /* Q = 7 (USB) */
                | (0U << 25);          /* R = 2 (00) */
    RCC_CR |= RCC_CR_PLLON;
    while (!(RCC_CR & RCC_CR_PLLRDY)) { }

    /* Flash latency for 170 MHz: 4 wait states + prefetch + icache */
    FLASH_ACR = 4 | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    /* Switch system clock to PLL */
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SWS_MSK) | RCC_CFGR_SW_PLL;
    while ((RCC_CFGR & RCC_CFGR_SWS_MSK) != (2U << 2)) { }

    /* Update s_tick reference: core is now 170 MHz */
    BOARD_MCU_CORE_CLK;  /* documented; unused at runtime */
}

/* ── SysTick: 1 ms tick at 170 MHz ──────────────────────────── */
static void systick_init(void) {
    /* Use TIM7 as 1 ms timer (simpler than SysTick for portability) */
    RCC_APB1ENR1 |= RCC_APB1ENR1_TIM7;
    tim_regs_t *t = TIM(7);
    t->PSC = (BOARD_MCU_CORE_CLK / 1000UL) - 1;   /* 1 kHz */
    t->ARR = 1000 - 1;                             /* 1 ms reload */
    t->DIER = TIM_DIER_UIE;
    t->CR1 = TIM_CR1_CEN;
    /* Enable TIM7 IRQ in NVIC (line 55 on STM32G4) */
    *(volatile uint32_t *)0xE000E100 = (1U << 55);
}

/* ── GPIO configuration ─────────────────────────────────────── */
static void gpio_init_all(void) {
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOA | RCC_AHB2ENR_GPIOB |
                   RCC_AHB2ENR_GPIOC | RCC_AHB2ENR_GPIOF;

    /* HART modem control pins */
    gpio_set_mode(GPIO(A), PIN_HART_RTS, GPIO_MODE_OUTPUT);
    gpio_set_mode(GPIO(A), PIN_HART_CD, GPIO_MODE_INPUT);
    gpio_set_pupd(GPIO(A), PIN_HART_CD, GPIO_PUPD_PU);
    gpio_set_mode(GPIO(A), PIN_HART_SHDN, GPIO_MODE_OUTPUT);

    /* USART2 TX/RX -> AF7 */
    gpio_set_mode(GPIO(A), PIN_USART2_TX, GPIO_MODE_AF);
    gpio_set_af(GPIO(A), PIN_USART2_TX, GPIO_AF7_USART12);
    gpio_set_mode(GPIO(A), PIN_USART2_RX, GPIO_MODE_AF);
    gpio_set_af(GPIO(A), PIN_USART2_RX, GPIO_AF7_USART12);

    /* USART1 TX/RX (console) -> AF7 */
    gpio_set_mode(GPIO(A), PIN_UART1_TX, GPIO_MODE_AF);
    gpio_set_af(GPIO(A), PIN_UART1_TX, GPIO_AF7_USART12);
    gpio_set_mode(GPIO(A), PIN_UART1_RX, GPIO_MODE_AF);
    gpio_set_af(GPIO(A), PIN_UART1_RX, GPIO_AF7_USART12);

    /* USB FS pins */
    gpio_set_mode(GPIO(A), PIN_USBDM, GPIO_MODE_AF);
    gpio_set_af(GPIO(A), PIN_USBDM, 10);
    gpio_set_mode(GPIO(A), PIN_USBDP, GPIO_MODE_AF);
    gpio_set_af(GPIO(A), PIN_USBDP, 10);

    /* DAC1 OUT1 (PA4) */
    gpio_set_mode(GPIO(A), 4, GPIO_MODE_ANALOG);

    /* ADC channels (PA0, PA1, PB0, PB1) as analog */
    gpio_set_mode(GPIO(A), 0, GPIO_MODE_ANALOG);
    gpio_set_mode(GPIO(A), 1, GPIO_MODE_ANALOG);
    gpio_set_mode(GPIO(B), 0, GPIO_MODE_ANALOG);
    gpio_set_mode(GPIO(B), 1, GPIO_MODE_ANALOG);

    /* Bypass relay drive (PA8) */
    gpio_set_mode(GPIO(A), PIN_BYPASS_RELAY, GPIO_MODE_OUTPUT);

    /* SPI1 pins for microSD (PB3/PB4/PB5) -> AF5 */
    gpio_set_mode(GPIO(B), 3, GPIO_MODE_AF);
    gpio_set_af(GPIO(B), 3, GPIO_AF5_SPI1);
    gpio_set_mode(GPIO(B), 4, GPIO_MODE_AF);
    gpio_set_af(GPIO(B), 4, GPIO_AF5_SPI1);
    gpio_set_mode(GPIO(B), 5, GPIO_MODE_AF);
    gpio_set_af(GPIO(B), 5, GPIO_AF5_SPI1);
    /* SD CS (PB6), CD (PB7) */
    gpio_set_mode(GPIO(B), PIN_SD_CS, GPIO_MODE_OUTPUT);
    gpio_set_mode(GPIO(B), PIN_SD_CD, GPIO_MODE_INPUT);
    gpio_set_pupd(GPIO(B), PIN_SD_CD, GPIO_PUPD_PU);

    /* OLED DC/RES (PB8/PB9) */
    gpio_set_mode(GPIO(B), PIN_OLED_DC, GPIO_MODE_OUTPUT);
    gpio_set_mode(GPIO(B), PIN_OLED_RES, GPIO_MODE_OUTPUT);

    /* LEDs (PB12/PB13/PB14) */
    gpio_set_mode(GPIO(B), PIN_LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_mode(GPIO(B), PIN_LED_GREEN, GPIO_MODE_OUTPUT);
    gpio_set_mode(GPIO(B), PIN_LED_BLUE, GPIO_MODE_OUTPUT);

    /* Isolation bypass (PB15) */
    gpio_set_mode(GPIO(B), PIN_ISO_BYPASS, GPIO_MODE_OUTPUT);

    /* User button (PC13, active low) */
    gpio_set_mode(GPIO(C), PIN_USER_BUTTON, GPIO_MODE_INPUT);
    gpio_set_pupd(GPIO(C), PIN_USER_BUTTON, GPIO_PUPD_PU);

    /* UART4 for BLE (PA0/PA1 alternate use on G4: actually PC10/PC11) */
    gpio_set_mode(GPIO(C), 10, GPIO_MODE_AF);
    gpio_set_af(GPIO(C), 10, 5);   /* UART4_TX AF5 */
    gpio_set_mode(GPIO(C), 11, GPIO_MODE_AF);
    gpio_set_af(GPIO(C), 11, 5);   /* UART4_RX AF5 */
}

/* ── ADC init (ADC1, 12-bit, single-ended, software trigger) ── */
static void adc_init_all(void) {
    RCC_AHB2ENR |= RCC_AHB2ENR_ADC12;
    /* Common for ADC1/2 — set CKMODE = synchronous /2 */
    *(volatile uint32_t *)(ADC1_BASE + 0x300) = 0;   /* SYSCFG/CRR default */

    adc_regs_t *a = ADC(1);
    /* Exit deep power-down, enable voltage regulator */
    a->CR = 0;
    a->CR |= (1U << 28);   /* ADVREGEN */
    system_delay_ms(1);
    /* Calibrate (single-ended) */
    a->CR |= ADC_CR_ADCAL | (1U << 30);   /* ALD=1 (single) */
    while (a->CR & ADC_CR_ADCAL) { }
    /* Enable */
    a->ISR = ADC_ISR_ADRDY;
    a->CR |= ADC_CR_ADEN;
    while (!(a->ISR & ADC_ISR_ADRDY)) { }
    /* Configure: 12-bit, right-align, single conversion */
    a->CFGR = 0;   /* CONT=0, DISCEN=0, 12-bit */
}

/* ── DAC init ────────────────────────────────────────────────── */
static void dac_init(void) {
    RCC_AHB1ENR |= RCC_AHB2ENR_DAC1EN;
    /* Trigger software, buffer enabled */
    DAC_CR = 0;
    system_delay_us(10);
}

/* ── Public board init ───────────────────────────────────────── */
void board_init_clock(void)   { clock_init(); }
void board_init_gpio(void)    { gpio_init_all(); }
void board_init_adc(void)     { adc_init_all(); }
void board_init_dac(void)     { dac_init(); }
void board_init_systick(void) { systick_init(); }

/* ── EXTI for user button ────────────────────────────────────── */
void board_init_exti(void) {
    RCC_APB2ENR |= RCC_APB2ENR_SYSCFG;
    /* Route PC13 to EXTI13 */
    *(volatile uint32_t *)(SYSCFG_BASE + 0x08 + (13 / 4) * 4) = 2;  /* port C = 2 */
    /* Falling edge, interrupt enable */
    *(volatile uint32_t *)(EXTI_BASE + 0x00) |= (1U << 13);   /* IMR13 */
    *(volatile uint32_t *)(EXTI_BASE + 0x0C) |= (1U << 13);   /* FTSR13 */
    *(volatile uint32_t *)0xE000E100 = (1U << 40);   /* EXTI15_10 IRQ */
}

/* ── NVIC system reset ───────────────────────────────────────── */
void NVIC_SystemReset(void) {
    *(volatile uint32_t *)0xE000ED0C = 0x05FA0004;
    while (1) { }
}

/* ── TIM7 IRQ handler ────────────────────────────────────────── */
void TIM7_DAC_IRQHandler(void) {
    tim_regs_t *t = TIM(7);
    if (t->SR & TIM_SR_UIF) {
        t->SR = 0;
        s_tick++;
    }
}