/*
 * board_init.c — STM32G474 board initialization (clocks, GPIO, peripherals)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Minimal bare-metal init: no HAL, no CMSIS, direct register access.
 * Configures the STM32G474 for 170 MHz from HSI + PLL, then sets up
 * GPIO pin modes, I²C, USART, SPI, ADC, and the UCPD peripheral.
 */

#include <stdint.h>
#include "board.h"
#include "registers.h"

/* ---- RCC register helpers ---- */
#define RCC_CR      (*(volatile uint32_t *)(RCC_BASE + 0x00u))
#define RCC_CFGR    (*(volatile uint32_t *)(RCC_BASE + 0x08u))
#define RCC_PLLCFGR (*(volatile uint32_t *)(RCC_BASE + 0x0Cu))
#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x48u))
#define RCC_AHB2ENR (*(volatile uint32_t *)(RCC_BASE + 0x4Cu))
#define RCC_APB1ENR1 (*(volatile uint32_t *)(RCC_BASE + 0x58u))
#define RCC_APB1ENR2 (*(volatile uint32_t *)(RCC_BASE + 0x5Cu))
#define RCC_APB2ENR (*(volatile uint32_t *)(RCC_BASE + 0x60u))
#define RCC_CCIPR   (*(volatile uint32_t *)(RCC_BASE + 0x88u))

#define PWR_CR1     (*(volatile uint32_t *)(PWR_BASE + 0x00u))
#define FLASH_ACR   (*(volatile uint32_t *)(FLASH_BASE_REG + FLASH_ACR_OFFSET))

/* ---- GPIO helpers ---- */
static void gpio_config(uint32_t base, uint8_t pin, uint8_t mode, uint8_t otype,
                        uint8_t speed, uint8_t pupd, uint8_t af) {
    uint32_t m;

    /* MODER */
    m = GPIO_REG(base, GPIO_MODER_OFFSET);
    m &= ~(3u << (pin * 2));
    m |= (uint32_t)mode << (pin * 2);
    GPIO_REG(base, GPIO_MODER_OFFSET) = m;

    /* OTYPER */
    m = GPIO_REG(base, GPIO_OTYPER_OFFSET);
    m &= ~(1u << pin);
    m |= (uint32_t)otype << pin;
    GPIO_REG(base, GPIO_OTYPER_OFFSET) = m;

    /* OSPEEDR */
    m = GPIO_REG(base, GPIO_OSPEEDR_OFFSET);
    m &= ~(3u << (pin * 2));
    m |= (uint32_t)speed << (pin * 2);
    GPIO_REG(base, GPIO_OSPEEDR_OFFSET) = m;

    /* PUPDR */
    m = GPIO_REG(base, GPIO_PUPDR_OFFSET);
    m &= ~(3u << (pin * 2));
    m |= (uint32_t)pupd << (pin * 2);
    GPIO_REG(base, GPIO_PUPDR_OFFSET) = m;

    /* AFR */
    if (pin < 8) {
        m = GPIO_REG(base, GPIO_AFRL_OFFSET);
        m &= ~(0xFu << (pin * 4));
        m |= (uint32_t)af << (pin * 4);
        GPIO_REG(base, GPIO_AFRL_OFFSET) = m;
    } else {
        m = GPIO_REG(base, GPIO_AFRH_OFFSET);
        m &= ~(0xFu << ((pin - 8) * 4));
        m |= (uint32_t)af << ((pin - 8) * 4);
        GPIO_REG(base, GPIO_AFRH_OFFSET) = m;
    }
}

static void gpio_write(uint32_t base, uint8_t pin, uint8_t val) {
    if (val) {
        GPIO_REG(base, GPIO_BSRR_OFFSET) = (1u << pin);
    } else {
        GPIO_REG(base, GPIO_BSRR_OFFSET) = (1u << (pin + 16));
    }
}

/* ---- System clock configuration: HSI → PLL → 170 MHz ---- */
static void clock_init(void) {
    /* Enable HSI */
    RCC_CR |= RCC_CR_HSION;
    while (!(RCC_CR & (1u << 10))) {} /* Wait for HSIRDY */

    /* Configure flash latency for 170 MHz (WS = 6) */
    FLASH_ACR &= ~FLASH_ACR_LATENCY_MASK;
    FLASH_ACR |= 6u | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    /* Enable power interface clock for VOS */
    RCC_APB1ENR1 |= (1u << 28); /* PWREN */
    /* Set VOS to range 1 (highest performance) */
    PWR_CR1 &= ~(3u << 9);
    PWR_CR1 |= (1u << 9); /* VOS0 = range 1 */

    /* PLL configuration: HSI 16 MHz → PLL → 170 MHz
     * PLLM = 4, PLLN = 85, PLLP = 2, PLLQ = 8, PLLR = 2
     * VCO = 16/4 * 85 = 340 MHz
     * SYSCLK = 340 / 2 = 170 MHz
     */
    RCC_PLLCFGR = (4u << 4)    /* PLLM */
                | (85u << 8)   /* PLLN */
                | (0u << 25)   /* PLLP = 2 (0) */
                | (8u << 21)   /* PLLQ */
                | (2u << 29)   /* PLLR = 2 */
                | (1u << 24)   /* PLLREN */
                | (3u << 0);   /* PLLSRC = HSI */

    RCC_CR |= RCC_CR_PLLON;
    while (!(RCC_CR & RCC_CR_PLLRDY)) {}

    /* Select PLL as system clock */
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_PLL;
    while (((RCC_CFGR >> RCC_CFGR_SWS_SHIFT) & 3u) != 2u) {}
}

/* ---- GPIO port clock enable ---- */
static void enable_gpio_clocks(void) {
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN |
                   RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIODEN |
                   RCC_AHB2ENR_GPIOEEN | RCC_AHB2ENR_GPIOFEN |
                   RCC_AHB2ENR_GPIOGEN;
}

/* ---- Peripheral clock enable ---- */
static void enable_periph_clocks(void) {
    /* DMA */
    RCC_AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN | RCC_AHB1ENR_DMAMUX1EN;

    /* I²C1, I²C2, SPI2, USART2, USART3, UART4, LPUART1, UCPD1 */
    RCC_APB1ENR1 |= RCC_APB1ENR1_I2C1EN | RCC_APB1ENR1_I2C2EN |
                    RCC_APB1ENR1_SPI2EN |
                    RCC_APB1ENR1_USART2EN | RCC_APB1ENR1_USART3EN |
                    RCC_APB1ENR1_UART4EN | RCC_APB1ENR1_LPUART1EN |
                    RCC_APB1ENR1_UCPD1EN | RCC_APB1ENR1_TIM2EN;

    /* USART1, SPI1, TIM1, ADC1, SYSCFG */
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_SPI1EN |
                   RCC_APB2ENR_TIM1EN | RCC_APB2ENR_ADC1EN |
                   RCC_APB2ENR_SYSCFGEN;
}

/* ---- Configure all GPIO pins ---- */
static void gpio_init_all(void) {
    /* --- USART2 (BLE) on PA2/PA3, AF7 --- */
    gpio_config(GPIOA_BASE, 2, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_VHIGH,
                GPIO_PUPD_UP, 7u);
    gpio_config(GPIOA_BASE, 3, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_VHIGH,
                GPIO_PUPD_UP, 7u);

    /* --- USART1 (USB CDC debug) on PA9/PA10, AF7 --- */
    gpio_config(GPIOA_BASE, 9, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_VHIGH,
                GPIO_PUPD_UP, 7u);
    gpio_config(GPIOA_BASE, 10, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_VHIGH,
                GPIO_PUPD_UP, 7u);

    /* --- I²C1 on PB6/PB7, AF4 --- */
    gpio_config(GPIOB_BASE, 6, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_SPEED_VHIGH,
                GPIO_PUPD_UP, 4u);
    gpio_config(GPIOB_BASE, 7, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_SPEED_VHIGH,
                GPIO_PUPD_UP, 4u);

    /* --- I²C2 on PB10/PB11, AF4 --- */
    gpio_config(GPIOB_BASE, 10, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_SPEED_VHIGH,
                GPIO_PUPD_UP, 4u);
    gpio_config(GPIOB_BASE, 11, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_SPEED_VHIGH,
                GPIO_PUPD_UP, 4u);

    /* --- ADC inputs on PA0, PA1 (analog) --- */
    gpio_config(GPIOA_BASE, 0, GPIO_MODE_ANALOG, 0, 0, GPIO_PUPD_NONE, 0);
    gpio_config(GPIOA_BASE, 1, GPIO_MODE_ANALOG, 0, 0, GPIO_PUPD_NONE, 0);

    /* --- TMUX2512 mux select pins: PA4, PA5 (mux1), PA6, PA7 (mux2) --- */
    gpio_config(GPIOA_BASE, 4, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);
    gpio_config(GPIOA_BASE, 5, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);
    gpio_config(GPIOA_BASE, 6, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);
    gpio_config(GPIOA_BASE, 7, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);

    /* --- eFuse enable: PB0 (src), PB1 (snk) --- */
    gpio_config(GPIOB_BASE, 0, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);
    gpio_config(GPIOB_BASE, 1, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);

    /* --- MOSFET gate drivers: PB2, PB3 --- */
    gpio_config(GPIOB_BASE, 2, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);
    gpio_config(GPIOB_BASE, 3, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);

    /* --- USB data switch: PB4 (SEL), PB5 (EN) --- */
    gpio_config(GPIOB_BASE, 4, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);
    gpio_config(GPIOB_BASE, 5, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);

    /* --- Boost converter: PB8 (EN), PB9 (PWM via TIM4 alt) --- */
    gpio_config(GPIOB_BASE, 8, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);
    gpio_config(GPIOB_BASE, 9, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_VHIGH,
                GPIO_PUPD_NONE, 11u); /* TIM4_CH4 AF11 */

    /* --- OLED reset: PB12 --- */
    gpio_config(GPIOB_BASE, 12, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);

    /* --- Status LEDs: PC13 (red), PC14 (green), PC15 (blue) --- */
    gpio_config(GPIOC_BASE, 13, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);
    gpio_config(GPIOC_BASE, 14, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);
    gpio_config(GPIOC_BASE, 15, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_MED,
                GPIO_PUPD_NONE, 0);

    /* Default states: LEDs off, eFuses disabled, USB switch off */
    gpio_write(GPIOC_BASE, 13, 0); /* Red LED off */
    gpio_write(GPIOC_BASE, 14, 0); /* Green LED off */
    gpio_write(GPIOC_BASE, 15, 0); /* Blue LED off */
    gpio_write(GPIOB_BASE, 0, 0);  /* eFuse src disabled */
    gpio_write(GPIOB_BASE, 1, 0);  /* eFuse snk disabled */
    gpio_write(GPIOB_BASE, 4, 0); /* USB switch SEL */
    gpio_write(GPIOB_BASE, 5, 0); /* USB switch disabled */
    gpio_write(GPIOB_BASE, 8, 0); /* Boost disabled */
    gpio_write(GPIOB_BASE, 12, 1); /* OLED reset high (active low) */
}

/* ---- USART2 init (BLE UART, 115200 baud) ---- */
static void usart2_init(void) {
    volatile uint32_t *cr1 = (volatile uint32_t *)(USART2_BASE + USART_CR1_OFFSET);
    volatile uint32_t *cr2 = (volatile uint32_t *)(USART2_BASE + USART_CR2_OFFSET);
    volatile uint32_t *cr3 = (volatile uint32_t *)(USART2_BASE + USART_CR3_OFFSET);
    volatile uint32_t *brr = (volatile uint32_t *)(USART2_BASE + USART_BRR_OFFSET);

    *cr1 = 0; /* Disable before config */
    *cr2 = 0;
    *cr3 = 0;
    /* 170 MHz / 115200 = 1476 */
    *brr = 1476u;
    *cr1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

/* ---- USART1 init (USB CDC debug, 115200 baud) ---- */
static void usart1_init(void) {
    volatile uint32_t *cr1 = (volatile uint32_t *)(USART1_BASE + USART_CR1_OFFSET);
    volatile uint32_t *cr2 = (volatile uint32_t *)(USART1_BASE + USART_CR2_OFFSET);
    volatile uint32_t *cr3 = (volatile uint32_t *)(USART1_BASE + USART_CR3_OFFSET);
    volatile uint32_t *brr = (volatile uint32_t *)(USART1_BASE + USART_BRR_OFFSET);

    *cr1 = 0;
    *cr2 = 0;
    *cr3 = 0;
    *brr = 1476u;
    *cr1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

/* ---- SysTick init (1 ms tick) ---- */
static void systick_init(void) {
    SYST_RVR = (SYSCLK_HZ / 1000u) - 1u;
    SYST_CVR = 0;
    SYST_CSR = SYSTICK_ENABLE | SYSTICK_TICKINT | SYSTICK_CLKSOURCE;
}

/* ---- I²C generic init ---- */
void i2c_init(uint32_t base, uint32_t timingr) {
    volatile uint32_t *cr1 = (volatile uint32_t *)(base + I2C_CR1_OFFSET);
    volatile uint32_t *timing = (volatile uint32_t *)(base + I2C_TIMINGR_OFFSET);

    *cr1 = 0; /* Disable */
    *timing = timingr;
    *cr1 = I2C_CR1_PE; /* Enable */
}

/* ---- I²C write register ---- */
int i2c_write_reg(uint32_t base, uint8_t dev_addr, uint8_t reg,
                  const uint8_t *data, uint16_t len) {
    volatile uint32_t *cr1 = (volatile uint32_t *)(base + I2C_CR1_OFFSET);
    volatile uint32_t *cr2 = (volatile uint32_t *)(base + I2C_CR2_OFFSET);
    volatile uint32_t *isr = (volatile uint32_t *)(base + I2C_ISR_OFFSET);
    volatile uint32_t *txdr = (volatile uint32_t *)(base + I2C_TXDR_OFFSET);
    volatile uint32_t *icr = (volatile uint32_t *)(base + I2C_ICR_OFFSET);

    /* Clear flags */
    *icr = 0x3Fu;

    /* Write register address + data: 1 + len bytes */
    uint16_t total = 1u + len;
    *cr2 = ((uint32_t)dev_addr << 1)
         | ((uint32_t)total << I2C_CR2_NBYTES_SHIFT)
         | I2C_CR2_START;

    /* Send register address */
    while (!(*isr & I2C_ISR_TXE)) {}
    *txdr = reg;

    /* Send data bytes */
    for (uint16_t i = 0; i < len; i++) {
        while (!(*isr & I2C_ISR_TXE)) {}
        *txdr = data[i];
    }

    /* Wait for transfer complete */
    uint32_t timeout = millis() + 100;
    while (!(*isr & I2C_ISR_TC)) {
        if (millis() > timeout) return -1;
    }

    *cr2 |= I2C_CR2_STOP;
    while (!(*isr & I2C_ISR_STOPF)) {}
    *icr = I2C_ISR_STOPF;

    return 0;
}

/* ---- I²C read register ---- */
int i2c_read_reg(uint32_t base, uint8_t dev_addr, uint8_t reg,
                 uint8_t *data, uint16_t len) {
    volatile uint32_t *cr2 = (volatile uint32_t *)(base + I2C_CR2_OFFSET);
    volatile uint32_t *isr = (volatile uint32_t *)(base + I2C_ISR_OFFSET);
    volatile uint32_t *rxdr = (volatile uint32_t *)(base + I2C_RXDR_OFFSET);
    volatile uint32_t *icr = (volatile uint32_t *)(base + I2C_ICR_OFFSET);

    *icr = 0x3Fu;

    /* Write register address (1 byte, no stop) */
    *cr2 = ((uint32_t)dev_addr << 1)
         | (1u << I2C_CR2_NBYTES_SHIFT)
         | I2C_CR2_START;
    while (!(*isr & I2C_ISR_TXE)) {}
    *(volatile uint32_t *)(base + I2C_TXDR_OFFSET) = reg;
    while (!(*isr & I2C_ISR_TC)) {}

    /* Read data (restart) */
    *cr2 = ((uint32_t)dev_addr << 1)
         | I2C_CR2_RD_WRN
         | ((uint32_t)len << I2C_CR2_NBYTES_SHIFT)
         | I2C_CR2_START;

    for (uint16_t i = 0; i < len; i++) {
        while (!(*isr & I2C_ISR_RXNE)) {}
        data[i] = (uint8_t)*rxdr;
    }

    *cr2 |= I2C_CR2_STOP;
    while (!(*isr & I2C_ISR_STOPF)) {}
    *icr = I2C_ISR_STOPF;

    return 0;
}

/* ---- I²C write burst (no register, raw) ---- */
int i2c_write_burst(uint32_t base, uint8_t dev_addr, const uint8_t *data, uint16_t len) {
    volatile uint32_t *cr2 = (volatile uint32_t *)(base + I2C_CR2_OFFSET);
    volatile uint32_t *isr = (volatile uint32_t *)(base + I2C_ISR_OFFSET);
    volatile uint32_t *icr = (volatile uint32_t *)(base + I2C_ICR_OFFSET);

    *icr = 0x3Fu;
    *cr2 = ((uint32_t)dev_addr << 1)
         | ((uint32_t)len << I2C_CR2_NBYTES_SHIFT)
         | I2C_CR2_START;

    for (uint16_t i = 0; i < len; i++) {
        while (!(*isr & I2C_ISR_TXE)) {}
        *(volatile uint32_t *)(base + I2C_TXDR_OFFSET) = data[i];
    }

    while (!(*isr & I2C_ISR_TC)) {}
    *cr2 |= I2C_CR2_STOP;
    while (!(*isr & I2C_ISR_STOPF)) {}
    *icr = I2C_ISR_STOPF;

    return 0;
}

/* ---- I²C read burst (no register, raw) ---- */
int i2c_read_burst(uint32_t base, uint8_t dev_addr, uint8_t *data, uint16_t len) {
    volatile uint32_t *cr2 = (volatile uint32_t *)(base + I2C_CR2_OFFSET);
    volatile uint32_t *isr = (volatile uint32_t *)(base + I2C_ISR_OFFSET);
    volatile uint32_t *rxdr = (volatile uint32_t *)(base + I2C_RXDR_OFFSET);
    volatile uint32_t *icr = (volatile uint32_t *)(base + I2C_ICR_OFFSET);

    *icr = 0x3Fu;
    *cr2 = ((uint32_t)dev_addr << 1)
         | I2C_CR2_RD_WRN
         | ((uint32_t)len << I2C_CR2_NBYTES_SHIFT)
         | I2C_CR2_START;

    for (uint16_t i = 0; i < len; i++) {
        while (!(*isr & I2C_ISR_RXNE)) {}
        data[i] = (uint8_t)*rxdr;
    }

    *cr2 |= I2C_CR2_STOP;
    while (!(*isr & I2C_ISR_STOPF)) {}
    *icr = I2C_ISR_STOPF;

    return 0;
}

/* ---- Board init (called from main) ---- */
void board_init(void) {
    /* 1. System clocks */
    clock_init();

    /* 2. Enable all GPIO and peripheral clocks */
    enable_gpio_clocks();
    enable_periph_clocks();

    /* 3. Configure GPIO pins */
    gpio_init_all();

    /* 4. Init UARTs */
    usart1_init();
    usart2_init();

    /* 5. SysTick for millis() */
    systick_init();

    /* 6. Green LED on = board alive */
    gpio_write(GPIOC_BASE, 14, 1);
}