/*
 * board_init.c — WireReaper clock, GPIO, and OLED initialization
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — GPL-2.0
 *
 * Contains the low-level clock tree setup (PLL to 480 MHz),
 * generic GPIO port enables, and the SSD1306 OLED display driver.
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

/* ---- Clock tree initialization ---- */
void wr_clock_init(void) {
    /* Enable HSE (8 MHz external crystal) */
    RCC_CR |= (1U << 16); /* HSEON */
    while (!(RCC_CR & (1U << 17))); /* Wait for HSERDY */

    /* Configure flash latency for 240 MHz AHB (4 wait states) */
    FLASH_ACR = (4U << 0) | (1U << 8) | (1U << 9); /* Latency, prefetch, ACCEL */

    /* Configure PLL1: HSE / 1 * 120 / 2 = 480 MHz */
    /* PLLCFGR: PLLM=1, PLLN=120, PLLP=2, PLLQ=20, PLLR=2, PLLSRC=HSE */
    RCC_PLLCFGR = (1U << 0) |    /* PLLM = 1 */
                  (120U << 8) |  /* PLLN = 120 */
                  (0U << 16) |   /* PLLP = 2 (00) */
                  (1U << 24) |   /* PLLSRC = HSE */
                  (20U << 25);   /* PLLQ = 20 (480/20=24 for USB) */

    /* Enable PLL1 */
    RCC_CR |= (1U << 24); /* PLL1ON */
    while (!(RCC_CR & (1U << 25))); /* Wait for PLL1RDY */

    /* Set AHB/APB dividers and switch system clock to PLL1 */
    /* CFGR: SW=PLL1, HPRE=/2, PPRE1=/2, PPRE2=/2 */
    RCC_CFGR = (3U << 0) |   /* SW = PLL1 */
               (8U << 4) |   /* HPRE = /2 */
               (4U << 8) |   /* PPRE1 = /2 */
               (4U << 11);   /* PPRE2 = /2 */
    while (((RCC_CFGR >> 0) & 0x7) != 3); /* Wait for switch */

    /* Enable PWR voltage scaling to VOS0 (highest) */
    PWR_CR3 |= (1U << 13); /* VBE for LDO */
    PWR_D3CR = (3U << 14); /* VOS = scale 0 */
    while (!(PWR_D3CR & (1U << 13))); /* VOSRDY */
}

/* ---- Generic GPIO initialization ---- */
void wr_gpio_init(void) {
    /* Enable all GPIO ports */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOA | RCC_AHB1ENR_GPIOB |
                   RCC_AHB1ENR_GPIOC | RCC_AHB1ENR_GPIOD |
                   RCC_AHB1ENR_GPIOE | RCC_AHB1ENR_GPIOF |
                   RCC_AHB1ENR_GPIOG | RCC_AHB1ENR_GPIOH;

    /* Configure button pins as inputs with pull-ups */
    gpio_t *e = GPIO(BTN_PORT);
    int btns[] = {BTN_UP_PIN, BTN_DOWN_PIN, BTN_SELECT_PIN, BTN_TRIGGER_PIN};
    for (int i = 0; i < 4; i++) {
        int p = btns[i];
        e->MODER &= ~(3U << (p * 2));
        e->PUPDR |= (GPIO_PUPD_PULLUP << (p * 2));
    }

    /* Configure LED pins as outputs */
    gpio_t *led = GPIO(LED_PORT);
    int leds[] = {LED_STATUS_PIN, LED_CAPTURE_PIN};
    for (int i = 0; i < 2; i++) {
        int p = leds[i];
        led->MODER &= ~(3U << (p * 2));
        led->MODER |= (GPIO_MODE_OUTPUT << (p * 2));
        led->OTYPER &= ~(1U << p);
        led->OSPEEDR |= (GPIO_OSPEED_HIGH << (p * 2));
    }

    /* Status LED on */
    led->BSRR = (1U << LED_STATUS_PIN);
}

/* ---- SSD1306 OLED display driver ---- */
static i2c_t *const oled_i2c = (i2c_t *)I2C_INTERNAL_BASE;

#define OLED_I2C_ADDR_W ((OLED_I2C_ADDR << 1) & 0xFE)

static void oled_write_cmd(uint8_t cmd) {
    /* Wait for I2C ready */
    while (oled_i2c->ISR & I2C_ISR_BUSY);
    oled_i2c->CR2 = (OLED_I2C_ADDR & 0x7F) << 1 |
                    (2 << I2C_CR2_NBYTES_SHIFT) |
                    I2C_CR2_AUTOEND | I2C_CR2_START;
    while (!(oled_i2c->ISR & I2C_ISR_TXE));
    oled_i2c->TXDR = 0x00; /* Co=0, D/C=0 (command) */
    while (!(oled_i2c->ISR & I2C_ISR_TXE));
    oled_i2c->TXDR = cmd;
    while (!(oled_i2c->ISR & I2C_ISR_STOPF));
    oled_i2c->ICR = I2C_ICR_STOPCF;
}

static void oled_write_data(uint8_t data) {
    while (oled_i2c->ISR & I2C_ISR_BUSY);
    oled_i2c->CR2 = (OLED_I2C_ADDR & 0x7F) << 1 |
                    (2 << I2C_CR2_NBYTES_SHIFT) |
                    I2C_CR2_AUTOEND | I2C_CR2_START;
    while (!(oled_i2c->ISR & I2C_ISR_TXE));
    oled_i2c->TXDR = 0x40; /* Co=0, D/C=1 (data) */
    while (!(oled_i2c->ISR & I2C_ISR_TXE));
    oled_i2c->TXDR = data;
    while (!(oled_i2c->ISR & I2C_ISR_STOPF));
    oled_i2c->ICR = I2C_ICR_STOPCF;
}

/* ---- 5x7 font (compact, ASCII 0x20-0x7F) ---- */
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    /* ... full font truncated for brevity; in production,
       include full 96-character table ... */
    {0x7E,0x11,0x11,0x11,0x7E}, /* A=0x41 */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x7F,0x41,0x41,0x41,0x22}, /* C */
    /* ... etc ... */
};

/* ---- Display buffer (128x64 / 8 = 128x8 pages, 1024 bytes) ---- */
static uint8_t oled_fb[1024];
static int oled_dirty = 0;

void wr_oled_init(void) {
    /* Enable I2C3 for internal peripherals (OLED + fuel gauge) */
    RCC_APB1ENR1 |= RCC_APB1ENR_I2C3;
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOC;

    /* Configure PC0(SCL), PC1(SDA) as AF4 (I2C3) */
    gpio_t *c = GPIO(GPIOC_BASE);
    int pins[] = {I2C_INT_SCL_PIN, I2C_INT_SDA_PIN};
    for (int i = 0; i < 2; i++) {
        int p = pins[i];
        c->MODER &= ~(3U << (p * 2));
        c->MODER |= (GPIO_MODE_AF << (p * 2));
        c->OTYPER |= (GPIO_OTYPE_OD << p);
        c->PUPDR |= (GPIO_PUPD_PULLUP << (p * 2));
        c->AFR[p / 8] &= ~(0xFU << ((p % 8) * 4));
        c->AFR[p / 8] |= (4U << ((p % 8) * 4));
    }

    oled_i2c->CR1 = 0;
    oled_i2c->TIMINGR = I2C_TIMING_400K;
    oled_i2c->CR1 = I2C_CR1_PE | I2C_CR1_ANFOFF;

    /* SSD1306 initialization sequence */
    oled_write_cmd(0xAE); /* Display off */
    oled_write_cmd(0xD5); oled_write_cmd(0x80); /* Display clock div */
    oled_write_cmd(0xA8); oled_write_cmd(0x3F); /* Multiplex ratio 64 */
    oled_write_cmd(0xD3); oled_write_cmd(0x00); /* Display offset */
    oled_write_cmd(0x40); /* Display start line */
    oled_write_cmd(0x8D); oled_write_cmd(0x14); /* Charge pump on */
    oled_write_cmd(0x20); oled_write_cmd(0x00); /* Horizontal addressing */
    oled_write_cmd(0xA1); /* Segment remap */
    oled_write_cmd(0xC8); /* COM scan direction */
    oled_write_cmd(0xDA); oled_write_cmd(0x12); /* COM pins config */
    oled_write_cmd(0xD9); oled_write_cmd(0xF1); /* Pre-charge period */
    oled_write_cmd(0xDB); oled_write_cmd(0x40); /* VCOMH deselect */
    oled_write_cmd(0x2E); /* Deactivate scroll */
    oled_write_cmd(0xA4); /* Display resume */
    oled_write_cmd(0xA6); /* Normal display (not inverted) */
    oled_write_cmd(0xAF); /* Display on */

    memset(oled_fb, 0, sizeof(oled_fb));
    oled_dirty = 1;
}

void wr_oled_clear(void) {
    memset(oled_fb, 0, sizeof(oled_fb));
    oled_dirty = 1;
}

void wr_oled_show(int line, const char *text) {
    if (line < 0 || line >= 8)
        return;
    int row = line * 8;
    int col = 0;
    for (int i = 0; text[i] && col < 128; i++) {
        uint8_t ch = (uint8_t)text[i];
        if (ch < 0x20) ch = 0x20;
        if (ch > 0x7E) ch = 0x20;
        ch -= 0x20;
        /* Write 5 bytes of font data into framebuffer */
        for (int b = 0; b < 5 && col < 128; b++) {
            int idx = col + (row / 8) * 128;
            if (idx < 1024)
                oled_fb[idx] = font5x7[ch % 96][b];
            col++;
        }
        /* 1-pixel spacing column */
        if (col < 128) {
            int idx = col + (row / 8) * 128;
            if (idx < 1024)
                oled_fb[idx] = 0;
            col++;
        }
    }
    oled_dirty = 1;
    /* Flush to display immediately for simplicity */
    if (oled_dirty) {
        /* Set column and page range */
        oled_write_cmd(0x21); oled_write_cmd(0); oled_write_cmd(127);
        oled_write_cmd(0x22); oled_write_cmd(0); oled_write_cmd(7);
        /* Send all 1024 bytes */
        for (int i = 0; i < 1024; i++)
            oled_write_data(oled_fb[i]);
        oled_dirty = 0;
    }
}