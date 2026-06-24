/*
 * lora-phantom / drivers/board_init.c
 * Clock tree, GPIO, SDRAM, SDMMC, AES HW, RNG, RTC init.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#include "../board.h"
#include "../registers.h"

extern volatile uint32_t g_ticks_ms;  /* defined in main.c */

/* ---- SysTick config for 1 ms tick at HCLK=240 MHz ---- */
static void systick_init(void) {
    SYSTICK->LOAD = (HCLK_HZ / 1000) - 1;  /* 240000 - 1 */
    SYSTICK->VAL  = 0;
    SYSTICK->CSR  = SYSTICK_CSR_CLKSRC | SYSTICK_CSR_TICKINT | SYSTICK_CSR_ENABLE;
}

/* ---- Clock tree: HSE 25 MHz → PLL1 → SYSCLK 480 MHz, HCLK 240 MHz ---- */
static void clock_init(void) {
    /* Enable HSE and wait for ready */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) { }

    /* Configure Flash latency for 240 MHz HCLK (VOS0) — 4 wait states */
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_DCEN | FLASH_ACR_ICEN | 4;

    /* PLL1: input = HSE 25 MHz, DIVM1 = 5 → 5 MHz, N1 = 96 → 480 MHz,
     * P1 = 2 → 240 MHz SYSCLK, Q1 = 4 → 120 MHz (for USB), R1 = 2 → 240 MHz */
    RCC->PLLCKSELR = RCC_PLLCKSELR_PLLSRC_HSE | RCC_PLLCKSELR_DIVM1(5);
    RCC->PLLCFGR   = (1U << 0);   /* PLL1 VCO range 1 (128-560 MHz) */
    RCC->PLL1DIVR  = RCC_PLL1DIVR_N1(96 - 1) | RCC_PLL1DIVR_P1(2 - 1) |
                     RCC_PLL1DIVR_Q1(4 - 1) | RCC_PLL1DIVR_R1(2 - 1);
    RCC->PLL1FRACR = 0;

    /* Enable PLL1 and wait for ready */
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY)) { }

    /* D1CFGR: SYSCLK / 2 = 240 MHz HCLK, APB3 = /2 = 120 MHz */
    RCC->D1CFGR = (1U << 0) | (1U << 4) | (1U << 8);  /* HPRE=/2, D1PPRE=/2 */
    /* D2CFGR: APB1 = HCLK/2 = 120 MHz, APB2 = HCLK/2 = 120 MHz */
    RCC->D2CFGR = (1U << 0) | (1U << 4) | (1U << 8) | (1U << 11);
    /* D3CFGR: APB4 = /2 */
    RCC->D3CFGR = (1U << 0);

    /* Switch SYSCLK to PLL1 */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SWS_MASK) | RCC_CFGR_SW_PLL1;
    while (((RCC->CFGR >> 3) & 3) != RCC_CFGR_SW_PLL1) { }
}

/* ---- Enable all GPIO clocks ---- */
static void gpio_clocks_init(void) {
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                    RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                    RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOFEN |
                    RCC_AHB4ENR_GPIOGEN | RCC_AHB4ENR_GPIOHEN;
}

/* ---- GPIO pin configuration ---- */
static void gpio_pins_init(void) {
    /* SX1262 SPI6: PG8 (NSS), PG9 (DIO1), PG10 (BUSY), PB2 (RESET), PB6 (ANT_SW)
     * SPI6 SCK = PG13, MISO = PG12, MOSI = PG14 (AF5) */
    gpio_set_mode(GPIOG, 8, GPIO_MODE_OUTPUT);     /* NSS manual */
    gpio_set_otype(GPIOG, 8, GPIO_OTYPE_PP);
    gpio_set_ospeed(GPIOG, 8, GPIO_OSPEED_VHIGH);
    gpio_write(GPIOG, 8, 1);                       /* NSS high (idle) */

    gpio_set_mode(GPIOG, 13, GPIO_MODE_AF);        /* SPI6_SCK */
    gpio_set_af(GPIOG, 13, 5);
    gpio_set_ospeed(GPIOG, 13, GPIO_OSPEED_VHIGH);

    gpio_set_mode(GPIOG, 12, GPIO_MODE_AF);        /* SPI6_MISO */
    gpio_set_af(GPIOG, 12, 5);

    gpio_set_mode(GPIOG, 14, GPIO_MODE_AF);        /* SPI6_MOSI */
    gpio_set_af(GPIOG, 14, 5);
    gpio_set_ospeed(GPIOG, 14, GPIO_OSPEED_VHIGH);

    gpio_set_mode(GPIOG, 10, GPIO_MODE_INPUT);     /* BUSY */
    gpio_set_mode(GPIOG, 9, GPIO_MODE_INPUT);      /* DIO1 (IRQ via EXTI later) */

    gpio_set_mode(GPIOB, 2, GPIO_MODE_OUTPUT);     /* RESET */
    gpio_set_otype(GPIOB, 2, GPIO_OTYPE_PP);
    gpio_write(GPIOB, 2, 1);                       /* NRST high (not in reset) */

    gpio_set_mode(GPIOB, 6, GPIO_MODE_OUTPUT);     /* ANT_SW */
    gpio_set_otype(GPIOB, 6, GPIO_OTYPE_PP);
    gpio_write(GPIOB, 6, 0);                       /* RX mode */

    /* BLE USART3: PB10 (TX), PB11 (RX), PB13 (CTS), PB14 (RTS) — AF7 */
    gpio_set_mode(GPIOB, 10, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 10, 7);
    gpio_set_ospeed(GPIOB, 10, GPIO_OSPEED_HIGH);
    gpio_set_mode(GPIOB, 11, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 11, 7);
    gpio_set_mode(GPIOB, 13, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 13, 7);
    gpio_set_mode(GPIOB, 14, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 14, 7);
    gpio_set_mode(GPIOB, 15, GPIO_MODE_OUTPUT);    /* BLE RESET */
    gpio_set_otype(GPIOB, 15, GPIO_OTYPE_PP);
    gpio_write(GPIOB, 15, 1);

    /* USB-C: PA11 (DM), PA12 (DP) — AF10 (OTG_HS) */
    gpio_set_mode(GPIOA, 11, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 11, 10);
    gpio_set_mode(GPIOA, 12, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 12, 10);
    gpio_set_mode(GPIOA, 9, GPIO_MODE_INPUT);      /* VBUS sense */
    gpio_set_mode(GPIOA, 10, GPIO_MODE_INPUT);     /* OTG ID */

    /* microSD SDMMC1: PC8-PC12 (AF12), PD2 (AF12) */
    for (uint8_t p = 8; p <= 12; p++) {
        gpio_set_mode(GPIOC, p, GPIO_MODE_AF);
        gpio_set_af(GPIOC, p, 12);
        gpio_set_ospeed(GPIOC, p, GPIO_OSPEED_VHIGH);
    }
    gpio_set_mode(GPIOD, 2, GPIO_MODE_AF);
    gpio_set_af(GPIOD, 2, 12);
    gpio_set_ospeed(GPIOD, 2, GPIO_OSPEED_VHIGH);
    gpio_set_mode(GPIOA, 5, GPIO_MODE_INPUT);      /* SD card detect */
    gpio_set_pupd(GPIOA, 5, GPIO_PUPD_PU);

    /* Buttons: PC0, PC1 (input, pull-up) */
    gpio_set_mode(GPIOC, 0, GPIO_MODE_INPUT);
    gpio_set_pupd(GPIOC, 0, GPIO_PUPD_PU);
    gpio_set_mode(GPIOC, 1, GPIO_MODE_INPUT);
    gpio_set_pupd(GPIOC, 1, GPIO_PUPD_PU);

    /* LEDs: PE0-PE3 (output, push-pull) */
    for (uint8_t p = 0; p <= 3; p++) {
        gpio_set_mode(GPIOE, p, GPIO_MODE_OUTPUT);
        gpio_set_otype(GPIOE, p, GPIO_OTYPE_PP);
        gpio_set_ospeed(GPIOE, p, GPIO_OSPEED_LOW);
        gpio_write(GPIOE, p, 0);
    }

    /* OLED I2C4: PD12 (SCL), PD13 (SDA) — AF4 */
    gpio_set_mode(GPIOD, 12, GPIO_MODE_AF);
    gpio_set_af(GPIOD, 12, 4);
    gpio_set_otype(GPIOD, 12, GPIO_OTYPE_OD);
    gpio_set_pupd(GPIOD, 12, GPIO_PUPD_PU);
    gpio_set_mode(GPIOD, 13, GPIO_MODE_AF);
    gpio_set_af(GPIOD, 13, 4);
    gpio_set_otype(GPIOD, 13, GPIO_OTYPE_OD);
    gpio_set_pupd(GPIOD, 13, GPIO_PUPD_PU);

    /* Battery ADC: PF0 (analog) */
    gpio_set_mode(GPIOF, 0, GPIO_MODE_ANALOG);

    /* FMC SDRAM pins: PD0,PD1,PD4,PD5,PD8,PD9,PD10,PD14,PD15,
     * PE7,PE8,PE9,PE10,PE11,PE12,PE13,PE14,PE15, plus PC0? no — use listed */
    /* (Simplified: configure D0..D15 + addr + ctrl as AF12) */
    uint8_t fmc_pd[] = {0, 1, 4, 5, 8, 9, 10, 14, 15};
    for (uint32_t i = 0; i < sizeof(fmc_pd); i++) {
        gpio_set_mode(GPIOD, fmc_pd[i], GPIO_MODE_AF);
        gpio_set_af(GPIOD, fmc_pd[i], 12);
        gpio_set_ospeed(GPIOD, fmc_pd[i], GPIO_OSPEED_VHIGH);
    }
    uint8_t fmc_pe[] = {7, 8, 9, 10, 11, 12, 13, 14, 15};
    for (uint32_t i = 0; i < sizeof(fmc_pe); i++) {
        gpio_set_mode(GPIOE, fmc_pe[i], GPIO_MODE_AF);
        gpio_set_af(GPIOE, fmc_pe[i], 12);
        gpio_set_ospeed(GPIOE, fmc_pe[i], GPIO_OSPEED_VHIGH);
    }
}

/* ---- SPI6 init for SX1262 (8-bit, master, CPOL=0 CPHA=0, 18 MHz) ---- */
static void spi6_init(void) {
    /* Enable SPI6 clock — RCC AHB2? On H7, SPI6 is on APB2 (RCC_APB2ENR bit 5) */
    RCC->APB2ENR |= (1U << 5);   /* SPI6EN */
    /* Wait a few cycles for clock to propagate */
    for (volatile int i = 0; i < 10; i++) { }

    SPI6->CR1 = 0;
    /* BR: APB2 = 120 MHz, /8 = 15 MHz (closest to 18) — use /8 (BR=2) */
    SPI6->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                SPI_CR1_BR_DIV8 | SPI_CR1_CPOL | SPI_CR1_CPHA;
    /* Wait: SX1262 uses SPI mode 0 (CPOL=0, CPHA=0). Set correctly: */
    SPI6->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_DIV8;
    SPI6->CR2 = SPI_CR2_DS_8BIT | (1U << 12);  /* DS=8-bit, FRXTH */
    SPI6->CR1 |= SPI_CR1_SPE;  /* enable */
}

/* ---- USART3 init for BLE nRF52840 ---- */
static void usart3_init(void) {
    /* Enable USART3 clock (APB1L bit 18) */
    RCC->APB1LENR |= RCC_APB1LENR_USART3EN;
    for (volatile int i = 0; i < 10; i++) { }

    USART3->CR1 = 0;
    /* BRR for 115200 baud at APB1=120 MHz: 120000000/115200 = 1041.67 → 1042 */
    USART3->BRR = 1042;
    USART3->CR2 = 0;  /* 8N1, no flow control (HW flow via CTS/RTS pins anyway) */
    USART3->CR3 = (1U << 8) | (1U << 9);  /* CTSE + RTSE (hardware flow control) */
    USART3->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE;
}

/* ---- AES2 hardware accelerator enable ---- */
static void aes2_clk_init(void) {
    /* AES2 is on AHB2 (RCC_AHB2ENR bit 17) */
    RCC->AHB2ENR |= (1U << 17);
    for (volatile int i = 0; i < 10; i++) { }
}

/* ---- RNG enable ---- */
static void rng_init(void) {
    /* RNG on AHB2 (RCC_AHB2ENR bit 18) */
    RCC->AHB2ENR |= (1U << 18);
    for (volatile int i = 0; i < 10; i++) { }
    RNG->CR = RNG_CR_RNGEN;
}

/* ---- CRC enable ---- */
static void crc_init(void) {
    /* CRC on AHB4 (RCC_AHB4ENR bit 19) */
    RCC->AHB4ENR |= (1U << 19);
    for (volatile int i = 0; i < 10; i++) { }
    CRC->CR = CRC_CR_RESET;
}

/* ---- USB OTG-HS (FS mode) enable ---- */
static void usb_init(void) {
    /* USB OTG-HS on AHB1 (RCC_AHB1ENR bit 25) */
    RCC->AHB1ENR |= RCC_AHB1ENR_USB1OTGHSEN;
    for (volatile int i = 0; i < 100; i++) { }
    /* The full USB stack lives in usb_cdc.c; here we just enable the clock. */
}

/* ---- FMC SDRAM init (IS42S16160G-6TLI, 16 MB, Bank 5/6) ---- */
static void fmc_sdram_init(void) {
    /* Enable FMC clock (AHB3) */
    RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;
    for (volatile int i = 0; i < 10; i++) { }

    /* SDCR1 (Bank 5 — unused as device, used for timing): */
    /* Column bits = 9 (x16), Rows = 13, CAS = 3, 4 banks, WR = 2 */
    FMC_SDRAM->SDCR1 = (0U << 0)   /* NR = 13-bit row (00=11,01=12,10=13) */
                     | (1U << 2)   /* MWID = 16-bit */
                     | (2U << 4)   /* NB = 4 banks */
                     | (3U << 6)   /* CAS = 3 */
                     | (1U << 8);  /* WR = 2 cycles */

    /* SDTR1 timing (simplified for 120 MHz SDCLK = HCLK/2 = 120 MHz) */
    FMC_SDRAM->SDTR1 = (1U << 0)   /* TRCD = 1 */
                     | (1U << 4)   /* TRP = 1 */
                     | (2U << 8)   /* TWR = 2 */
                     | (4U << 12)  /* TRAS = 4 */
                     | (5U << 16)  /* TRC = 5 */
                     | (1U << 24); /* TXSR = 1 */

    /* SDCR2 (Bank 6 — actual SDRAM): same params */
    FMC_SDRAM->SDCR2 = FMC_SDRAM->SDCR1;

    /* SDTR2 (Bank 6): same timing */
    FMC_SDRAM->SDTR2 = FMC_SDRAM->SDTR1;

    /* SDRAM initialization sequence via SDCMR */
    /* Step 1: clock config enable */
    FMC_SDRAM->SDCMR = (1U << 0) | (0U << 4) | (0U << 5);  /* MR=1, Bank sel = 00 */
    for (volatile int i = 0; i < 100000; i++) { }
    /* Step 2: PALL (precharge all) */
    FMC_SDRAM->SDCMR = (2U << 0) | (3U << 4);  /* MR=2, both banks */
    for (volatile int i = 0; i < 10000; i++) { }
    /* Step 3: auto-refresh x8 */
    FMC_SDRAM->SDCMR = (3U << 0) | (3U << 4) | (7U << 5);  /* MR=3, 8 refreshes */
    for (volatile int i = 0; i < 10000; i++) { }
    /* Step 4: load mode register */
    FMC_SDRAM->SDCMR = (4U << 0) | (2U << 4) | (0x23 << 9); /* MR=4, Bank6, burst=3,CAS3 */
    for (volatile int i = 0; i < 10000; i++) { }

    /* Set refresh rate: 64 ms / 4096 rows = 15.625 µs; at 120 MHz = 1875 cycles */
    FMC_SDRAM->SDRTR = (1875U << 1);
}

/* ---- RTC enable (for timestamping) ---- */
static void rtc_init(void) {
    /* Enable PWR clock + access to backup domain */
    RCC->APB1LENR |= (1U << 28);   /* PWR */
    PWR->CR1 |= PWR_CR1_DBP;
    /* Enable LSE (32.768 kHz) */
    RCC->BDCR |= (1U << 0);        /* LSEON */
    for (volatile int i = 0; i < 10000; i++) { }
    while (!(RCC->BDCR & (1U << 1))) { }  /* LSERDY */
    /* Select LSE as RTC clock */
    RCC->BDCR = (RCC->BDCR & ~(3U << 8)) | (1U << 8);  /* RTCSEL = LSE */
    /* Enable RTC */
    RCC->BDCR |= (1U << 15);       /* RTCEN */
}

/* ---- Public board init ---- */
void board_init(void) {
    /* Clocks first */
    clock_init();
    /* SysTick 1 ms */
    systick_init();
    /* Enable all peripheral clocks needed */
    gpio_clocks_init();
    /* GPIO pin config */
    gpio_pins_init();
    /* Peripherals */
    spi6_init();
    usart3_init();
    aes2_clk_init();
    rng_init();
    crc_init();
    usb_init();
    fmc_sdram_init();
    rtc_init();

    /* Enable SYSCFG for EXTI (if needed for DIO1 IRQ) */
    /* RCC APB2 SYSCFGEN */
    RCC->APB2ENR |= (1U << 0);
}

/* ---- AES HW init ---- */
void aes_hw_init(void) {
    AES2->CR = 0;
    AES2->CR = AES_CR_DATATYPE;  /* byte-swap for big-endian LoRaWAN */
    /* Key is loaded per-operation in aes_hw.c */
}

/* ---- Hardware RNG read ---- */
uint32_t hw_random(void) {
    while (!(RNG->SR & RNG_SR_DRDY)) { }
    return RNG->DR;
}