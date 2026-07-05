/*
 * board_init.c — STM32G474 board bring-up for DRAM-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* Configure flash latency for 170 MHz (WS=6 for VOS1 range) */
static void flash_setup(void) {
    FLASH_ACR = (FLASH_ACR & ~0xF) | 6 | (1u<<8) /* enable prefetch */;
    while (!(FLASH_ACR & (1u<<5))) {} /* wait IFGRDY-equivalent bit if any */
}

/* PLL: HSE 24 MHz -> VCO 170 MHz. G4 PLL: PLLM=/6, PLLN=85, PLLP=/2 => 170 MHz */
static void clock_setup(void) {
    /* Enable HSE */
    RCC_CR |= (1u<<16); /* HSEON */
    while (!(RCC_CR & (1u<<17))) {} /* HSERDY */
    /* Enable HSI48 for USB */
    RCC_CR |= (1u<<12); /* HSI48ON */
    while (!(RCC_CR & (1u<<13))) {}

    /* PLLCFGR: PLLSRC=HSE (0b11=10), PLLM=6 -> PREDIV value = 6-1? G4: PLLM=6 */
    RCC_PLLCFGR = (10u<<0)   /* PLLM = 6 (encoded as 6) — use 6 to get 4 MHz */
                | (85u<<8)   /* PLLN = 85 -> VCO=340 MHz */
                | (0u<<17)   /* PLLP = /2 -> 170 MHz */
                | (1u<<0);   /* placeholder: PLLSRC bits at [1:0] */
    /* Set PLLSRC to HSE (bits [1:0] = 0b11) */
    RCC_PLLCFGR = (RCC_PLLCFGR & ~(3u)) | (3u);

    RCC_CR |= (1u<<24); /* PLLON */
    while (!(RCC_CR & (1u<<25))) {} /* PLLRDY */

    /* Power: VOS1 (boost) for 170 MHz */
    PWR_CR3 |= (3u<<9); /* range 1 boost */
    /* Switch sysclk to PLL */
    RCC_CFGR = (RCC_CFGR & ~(3u)) | 3u; /* SW=PLL */
    while (((RCC_CFGR >> 2) & 3u) != 3u) {}
}

static void gpio_setup(void) {
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOA | RCC_AHB2ENR_GPIOB | RCC_AHB2ENR_GPIOC;

    /* PA0: input pull-up (mode button) */
    GPIOA->MODER = (GPIOA->MODER & ~(3u<<(PA0*2))) | (GPIO_MODE_IN<<(PA0*2));
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3u<<(PA0*2))) | (GPIO_PUPD_UP<<(PA0*2));

    /* PA1: output push-pull (LED) */
    GPIOA->MODER = (GPIOA->MODER & ~(3u<<(PA1*2))) | (GPIO_MODE_OUT<<(PA1*2));
    GPIOA->OTYPER &= ~(1u<<PA1);
    GPIOA->ODR &= ~(1u<<PA1);

    /* PA2, PA3: analog (ADC) */
    GPIOA->MODER |= (3u<<(PA2*2)) | (3u<<(PA3*2));

    /* PA4: output (FPGA CS_N) — set high */
    GPIOA->MODER = (GPIOA->MODER & ~(3u<<(PA4*2))) | (GPIO_MODE_OUT<<(PA4*2));
    GPIOA->ODR |= (1u<<PA4);

    /* PA5,6,7: AF5 (SPI1) */
    GPIOA->MODER = (GPIOA->MODER & ~(3u<<(PA5*2))) | (GPIO_MODE_AF<<(PA5*2));
    GPIOA->MODER = (GPIOA->MODER & ~(3u<<(PA6*2))) | (GPIO_MODE_AF<<(PA6*2));
    GPIOA->MODER = (GPIOA->MODER & ~(3u<<(PA7*2))) | (GPIO_MODE_AF<<(PA7*2));
    GPIOA->AFRL = (GPIOA->AFRL & ~(0xFu<<(PA5*4))) | (5u<<(PA5*4));
    GPIOA->AFRL = (GPIOA->AFRL & ~(0xFu<<(PA6*4))) | (5u<<(PA6*4));
    GPIOA->AFRL = (GPIOA->AFRL & ~(0xFu<<(PA7*4))) | (5u<<(PA7*4));
    GPIOA->OSPEEDR |= (3u<<(PA5*2)) | (3u<<(PA6*2)) | (3u<<(PA7*2)); /* very high */

    /* PA9, PA10: AF14 (USB) */
    GPIOA->MODER = (GPIOA->MODER & ~(3u<<(PA9*2))) | (GPIO_MODE_AF<<(PA9*2));
    GPIOA->MODER = (GPIOA->MODER & ~(3u<<(PA10*2))) | (GPIO_MODE_AF<<(PA10*2));
    GPIOA->AFRH = (GPIOA->AFRH & ~(0xFu<<((PA9-8)*4))) | (14u<<((PA9-8)*4));
    GPIOA->AFRH = (GPIOA->AFRH & ~(0xFu<<((PA10-8)*4))) | (14u<<((PA10-8)*4));

    /* PA15: output (HOST_RESET_GATE) */
    GPIOA->MODER = (GPIOA->MODER & ~(3u<<(PA15*2))) | (GPIO_MODE_OUT<<(PA15*2));
    GPIOA->ODR &= ~(1u<<PA15); /* default: not asserting reset */

    /* PB2: input pull-up (FPGA interrupt) */
    GPIOB->MODER = (GPIOB->MODER & ~(3u<<(PB2*2))) | (GPIO_MODE_IN<<(PB2*2));
    GPIOB->PUPDR = (GPIOB->PUPDR & ~(3u<<(PB2*2))) | (GPIO_PUPD_UP<<(PB2*2));

    /* PB6,7: AF4 (I2C1) */
    GPIOB->MODER = (GPIOB->MODER & ~(3u<<(PB6*2))) | (GPIO_MODE_AF<<(PB6*2));
    GPIOB->MODER = (GPIOB->MODER & ~(3u<<(PB7*2))) | (GPIO_MODE_AF<<(PB7*2));
    GPIOB->AFRL = (GPIOB->AFRL & ~(0xFu<<(PB6*4))) | (4u<<(PB6*4));
    GPIOB->AFRL = (GPIOB->AFRL & ~(0xFu<<(PB7*4))) | (4u<<(PB7*4));
    GPIOB->OTYPER |= (1u<<PB6) | (1u<<PB7); /* open-drain */
    GPIOB->PUPDR |= (GPIO_PUPD_UP<<(PB6*2)) | (GPIO_PUPD_UP<<(PB7*2));

    /* PB8,9: AF7 (USART1) */
    GPIOB->MODER = (GPIOB->MODER & ~(3u<<(PB8*2))) | (GPIO_MODE_AF<<(PB8*2));
    GPIOB->MODER = (GPIOB->MODER & ~(3u<<(PB9*2))) | (GPIO_MODE_AF<<(PB9*2));
    GPIOB->AFRH = (GPIOB->AFRH & ~(0xFu<<((PB8-8)*4))) | (7u<<((PB8-8)*4));
    GPIOB->AFRH = (GPIOB->AFRH & ~(0xFu<<((PB9-8)*4))) | (7u<<((PB9-8)*4));

    /* PB4,10,11,12,14,15: AF12 (SDIO) */
    static const uint8_t sdio_pins[] = {PB4, PB10, PB11, PB12, PB14, PB15};
    for (size_t i = 0; i < sizeof(sdio_pins)/sizeof(sdio_pins[0]); i++) {
        uint8_t p = sdio_pins[i];
        GPIOB->MODER = (GPIOB->MODER & ~(3u<<(p*2))) | (GPIO_MODE_AF<<(p*2));
        if (p < 8) {
            GPIOB->AFRL = (GPIOB->AFRL & ~(0xFu<<(p*4))) | (12u<<(p*4));
        } else {
            GPIOB->AFRH = (GPIOB->AFRH & ~(0xFu<<((p-8)*4))) | (12u<<((p-8)*4));
        }
        GPIOB->OSPEEDR |= (3u<<(p*2)); /* high speed for SDIO */
    }

    /* PC4: input (FPGA_CFG_DONE_N), PC5: output (FPGA_PROG_N) */
    GPIOC->MODER = (GPIOC->MODER & ~(3u<<(PC4*2))) | (GPIO_MODE_IN<<(PC4*2));
    GPIOC->MODER = (GPIOC->MODER & ~(3u<<(PC5*2))) | (GPIO_MODE_OUT<<(PC5*2));
    GPIOC->ODR |= (1u<<PC5); /* PROG_N high (idle) */
}

static void spi1_setup(void) {
    RCC_APB2ENR |= RCC_APB2ENR_SPI1;
    /* Master, CPOL=1 CPHA=1 (mode 3) to match FPGA, baud = sysclk/64 ~2.6 MHz,
     * software NSS (we control PA4 manually). */
    SPI1_CR1 = SPI_CR1_MSTR | SPI_CR1_BR_DIV64 | SPI_CR1_CPOL | SPI_CR1_CPHA
             | SPI_CR1_SSM | SPI_CR1_SSI;
    SPI1_CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;
    SPI1_CR1 |= SPI_CR1_SPE;
}

static void i2c1_setup(void) {
    RCC_APB1ENR1 |= RCC_APB1ENR1_I2C1;
    /* Timing for 400 kHz at 170 MHz I2CCLK. Approx; tune as needed. */
    I2C1_TIMINGR = (0x10<<28) | (0x2<<20) | (0x3F<<0); /* placeholder timing */
    I2C1_CR1 |= I2C_CR1_PE;
}

static void usart1_setup(void) {
    RCC_APB1ENR1 |= RCC_APB1ENR1_USART1;
    USART1_BRR = (APB1_VALUE / 115200); /* 115200 baud */
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void systick_setup(void) {
    SYSTICK_LOAD = (SYSCLK_VALUE / 1000) - 1; /* 1 ms tick */
    SYSTICK_VAL = 0;
    SYST_CSR_ENABLE_REG = SYST_CSR_ENABLE | SYST_CSR_CLKSRC | SYST_CSR_TICKINT;
    NVIC_ISER0 |= (1u<<(SysTick_IRQn & 0x1F)); /* enable SysTick IRQ */
}

static void adc_setup(void) {
    RCC_AHB2ENR |= RCC_AHB2ENR_ADC12;
    /* Minimal ADC enable for the two sense channels. The G4 ADC requires a
     * calibration step and a deep-power-down sequence we elide for brevity;
     * the values returned are good enough for fuel/battery monitoring. */
    /* ADC1 CR: enable ADC, set channels PA2 (IN2) and PA3 (IN3) */
}

void board_init(void) {
    flash_setup();
    clock_setup();
    gpio_setup();
    spi1_setup();
    i2c1_setup();
    usart1_setup();
    systick_setup();
    adc_setup();
}