/*
 * aperture-phantom / firmware / drivers / board_init.c
 *
 * STM32H743 clock-tree, GPIO, and peripheral bring-up for AperturePhantom.
 * Configures HSE→PLL1 for 480 MHz sysclk, enables all peripheral clocks,
 * configures GPIO modes/alternate functions, sets NVIC priorities, and
 * calls into the individual driver inits as needed.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include "board.h"

/* ---- Helper: wait for PLL lock bit ---- */
static void wait_pll_lock(volatile uint32_t *cr, uint32_t bit) {
    while (((*cr) & bit) == 0) { /* spin */ }
}

void board_init(void) {
    /* 1. Enable HSE and wait for it to be ready. */
    RCC_CR |= (1u << 16); /* HSEON */
    while ((RCC_CR & (1u << 17)) == 0) { } /* HSERDY */

    /* 2. Configure PLL1: HSE / 5 * 96 / 2 = 480 MHz. */
    RCC_PLLCFGR = (5u << 0)        /* M = 5 */
                | (96u << 8)       /* N = 96 */
                | (0u << 25)       /* PLL1REN off */
                | (0u << 29)       /* PLL1VCOSEL: 192-836 MHz */
                | (1u << 0);       /* SRC = HSE */
    RCC_PLLCFGR |= (1u << 24);      /* PLL1PEN */
    RCC_PLLCFGR &= ~(3u << 2);
    RCC_PLLCFGR |= (0u << 2);       /* P = 2 (divide by 2): 480/2=240? No — see below */
    /* For STM32H7 sysclk = PLL1_VCO / P. VCO = HSE/M * N = 25/5*96 = 480 MHz.
     * P=2 gives 240 MHz sysclk; we want 480 MHz so P=1. */
    RCC_PLLCFGR |= (1u << 2);       /* P = 1 → 480 MHz */

    /* 3. Enable PLL1 and wait for lock. */
    RCC_CR |= (1u << 24); /* PLL1ON */
    wait_pll_lock(&RCC_CR, (1u << 25)); /* PLL1RDY */

    /* 4. Set flash latency for 240 MHz HCLK (5 wait states). */
    *(volatile uint32_t *)(0x52002020u) = 5u; /* FLASH_ACR.LATENCY */
    *(volatile uint32_t *)(0x52002020u) |= (1u << 8); /* ARTEN */

    /* 5. Switch SYSCLK to PLL1. */
    RCC_CFGR &= ~(7u << 0);
    RCC_CFGR |= (3u << 0); /* SW = PLL1 */
    while (((RCC_CFGR >> 3) & 7u) != 3u) { } /* SWS */

    /* 6. Configure bus dividers: HCLK = 240, AHB/2 = APB1=APB2 = 120 MHz. */
    RCC_CFGR &= ~(0xFu << 4);   /* HPRE = /1 (HCLK = 240) */
    RCC_CFGR &= ~(7u << 10);    /* PPRE1 = /2 (120 MHz) */
    RCC_CFGR |=  (4u << 10);
    RCC_CFGR &= ~(7u << 13);    /* PPRE2 = /2 (120 MHz) */
    RCC_CFGR |=  (4u << 13);

    /* 7. Enable peripheral clocks. */
    RCC_AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN | RCC_AHB1ENR_MDMAEN;
    RCC_AHB3ENR |= RCC_AHB3ENR_FMCEN;
    RCC_AHB4ENR |= (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3)
                |  (1u << 4) | (1u << 7); /* GPIOA,B,C,D,E,H */
    RCC_APB1LENR |= RCC_APB1LENR_USART3EN | RCC_APB1LENR_I2C1EN | RCC_APB1LENR_I2C2EN;
    RCC_APB2ENR  |= RCC_APB2ENR_SPI1EN;

    /* 8. GPIO pin config. */

    /* USART3: PB6 TX, PB7 RX, AF7 */
    GPIO_PIN_AF(GPIOB_BASE, 6, AF_USART3_TX_PB6);
    GPIO_PIN_AF(GPIOB_BASE, 7, AF_USART3_RX_PB7);

    /* I2C1 side-A: PB8 SCL, PB9 SDA, AF4 */
    GPIO_PIN_AF(GPIOB_BASE, 8, AF_I2C1_SCL_PB8);
    GPIO_PIN_AF(GPIOB_BASE, 9, AF_I2C1_SDA_PB9);
    GPIO_OTYPER(GPIOB_BASE) |= (1u << 8) | (1u << 9); /* open-drain */

    /* I2C2 side-B: PA8 SCL, PC9 SDA, AF4 */
    GPIO_PIN_AF(GPIOA_BASE, 8, AF_I2C2_SCL_PA8);
    GPIO_PIN_AF(GPIOC_BASE, 9, AF_I2C2_SDA_PC9);
    GPIO_OTYPER(GPIOA_BASE) |= (1u << 8);
    GPIO_OTYPER(GPIOC_BASE) |= (1u << 9);

    /* SPI1 to FPGA: PA5 SCK, PA6 MISO, PA7 MOSI, AF5 */
    GPIO_PIN_AF(GPIOA_BASE, 5, AF_SPI1_PA5_7);
    GPIO_PIN_AF(GPIOA_BASE, 6, AF_SPI1_PA5_7);
    GPIO_PIN_AF(GPIOA_BASE, 7, AF_SPI1_PA5_7);

    /* FPGA control pins (PB0 RST_N, PB1 CS_N) output, default high. */
    GPIO_PIN_OUT_PP(GPIOB_BASE, PIN_FPGA_RST);
    GPIO_PIN_OUT_PP(GPIOB_BASE, PIN_FPGA_CS);
    GPIO_SET(GPIOB_BASE, PIN_FPGA_RST);
    GPIO_SET(GPIOB_BASE, PIN_FPGA_CS);

    /* FPGA IRQ input (PC4) pull-up. */
    GPIO_PIN_IN_PU(GPIOC_BASE, PIN_FPGA_IRQ);

    /* LEDs (PC5/6/7) output push-pull, off. */
    GPIO_PIN_OUT_PP(GPIOC_BASE, PIN_LED_R);
    GPIO_PIN_OUT_PP(GPIOC_BASE, PIN_LED_G);
    GPIO_PIN_OUT_PP(GPIOC_BASE, PIN_LED_B);
    GPIO_CLR(GPIOC_BASE, PIN_LED_R);
    GPIO_CLR(GPIOC_BASE, PIN_LED_G);
    GPIO_CLR(GPIOC_BASE, PIN_LED_B);

    /* Motor enable (PC8) output. */
    GPIO_PIN_OUT_PP(GPIOC_BASE, PIN_MOTOR_EN);
    GPIO_CLR(GPIOC_BASE, PIN_MOTOR_EN);

    /* Touch pads (PC10/11/12) input floating (TSC cap-sense handles drive). */
    GPIO_MODER(GPIOC_BASE) &= ~(3u << (2*PIN_TOUCH0));
    GPIO_MODER(GPIOC_BASE) &= ~(3u << (2*PIN_TOUCH1));
    GPIO_MODER(GPIOC_BASE) &= ~(3u << (2*PIN_TOUCH2));
    GPIO_PUPDR(GPIOC_BASE) &= ~(3u << (2*PIN_TOUCH0));
    GPIO_PUPDR(GPIOC_BASE) &= ~(3u << (2*PIN_TOUCH1));
    GPIO_PUPDR(GPIOC_BASE) &= ~(3u << (2*PIN_TOUCH2));

    /* SD card detect (PC13) input pull-up. */
    GPIO_PIN_IN_PU(GPIOC_BASE, PIN_SD_CD);

    /* Sensor MUX (PC14) output, default passthrough (high). */
    GPIO_PIN_OUT_PP(GPIOC_BASE, PIN_SENSOR_MUX);
    GPIO_SET(GPIOC_BASE, PIN_SENSOR_MUX);

    /* DS28E07 1-wire (PC1) open-drain. */
    GPIO_MODER(GPIOC_BASE) &= ~(3u << (2*PIN_DS28E07_DQ));
    GPIO_MODER(GPIOC_BASE) |=  (1u << (2*PIN_DS28E07_DQ)); /* output */
    GPIO_OTYPER(GPIOC_BASE) |= (1u << PIN_DS28E07_DQ);
    GPIO_SET(GPIOC_BASE, PIN_DS28E07_DQ); /* idle high */

    /* 9. NVIC priority: SysTick lowest, UART/USB higher. */
    SCB_SHPR3 = (0x40u << 24); /* SysTick = 0x40 */

    /* 10. Enable FMC for SDRAM. */
    *(volatile uint32_t *)(0x58021C00u + 0xA0) |= (1u << 12); /* SYSCFG FMC enable */

    /* 11. Cache: enable I-cache and D-cache. */
    SCB_CCR |= (1u << 17) | (1u << 16);
    __asm__("dsb; isb");
}

void board_led_set(uint8_t r, uint8_t g, uint8_t b) {
    if (r) GPIO_SET(GPIOC_BASE, PIN_LED_R); else GPIO_CLR(GPIOC_BASE, PIN_LED_R);
    if (g) GPIO_SET(GPIOC_BASE, PIN_LED_G); else GPIO_CLR(GPIOC_BASE, PIN_LED_G);
    if (b) GPIO_SET(GPIOC_BASE, PIN_LED_B); else GPIO_CLR(GPIOC_BASE, PIN_LED_B);
}

void board_motor_pulse(uint16_t ms) {
    GPIO_SET(GPIOC_BASE, PIN_MOTOR_EN);
    uint32_t t0 = g_ticks_ms_local();
    while ((g_ticks_ms_local() - t0) < ms) { }
    GPIO_CLR(GPIOC_BASE, PIN_MOTOR_EN);
}

uint8_t board_touch_read(void) {
    /* Simple read of touch pad GPIO levels. Real build uses TSC peripheral;
     * here we approximate as button-like input bits. */
    uint8_t r = 0;
    /* PC10/11/12 — when touched, cap sense pulls low; we approximate with IDR */
    if ((GPIO_IDR(GPIOC_BASE) >> PIN_TOUCH0) & 1u) r |= 0x01;
    if ((GPIO_IDR(GPIOC_BASE) >> PIN_TOUCH1) & 1u) r |= 0x02;
    if ((GPIO_IDR(GPIOC_BASE) >> PIN_TOUCH2) & 1u) r |= 0x04;
    return r;
}

uint8_t board_sd_present(void) {
    return (GPIO_IDR(GPIOC_BASE) >> PIN_SD_CD) & 1u ? 0 : 1; /* active low */
}

void board_sensor_mux(uint8_t passthrough) {
    if (passthrough) GPIO_SET(GPIOC_BASE, PIN_SENSOR_MUX);
    else             GPIO_CLR(GPIOC_BASE, PIN_SENSOR_MUX);
}

/* local ms tick accessor (defined in main.c as g_ticks_ms) */
extern volatile uint32_t g_ticks_ms;
static uint32_t g_ticks_ms_local(void) { return g_ticks_ms; }