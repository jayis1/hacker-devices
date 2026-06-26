/*
 * drivers/oled.c — SH1106 OLED (128x64) driver for SideProbe
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Bit-banged SPI on PD12(DC)/PD13(SCK)/PD15(MOSI)/PD11(RST). 4-wire SPI mode.
 * Provides text + bar-chart drawing for the live CPA correlation display.
 */

#include <stdint.h>
#include "../board.h"
#include "../registers.h"

extern void sp_delay_us(volatile uint32_t us);
extern void sp_delay_ms(volatile uint32_t ms);

#define OLED_DC   12  /* PD12 */
#define OLED_SCK  13  /* PD13 */
#define OLED_MOSI 15  /* PD15 */
#define OLED_RST  11  /* PD11 */

static void oled_pin_init(void) {
    /* Set PD11, PD12, PD13, PD15 as push-pull outputs, speed high */
    uint32_t moder = GPIO_MODER(GPIOD_BASE);
    uint32_t mask = (3u << (OLED_RST*2)) | (3u << (OLED_DC*2))
                  | (3u << (OLED_SCK*2)) | (3u << (OLED_MOSI*2));
    moder &= ~mask;
    moder |= (1u << (OLED_RST*2)) | (1u << (OLED_DC*2))
           | (1u << (OLED_SCK*2)) | (1u << (OLED_MOSI*2));
    GPIO_MODER(GPIOD_BASE) = moder;
    GPIO_OSPEEDR(GPIOD_BASE) |= mask; /* high speed */
}

static void oled_dc(int v) {
    if (v) GPIO_BSRR(GPIOD_BASE) = (1u << OLED_DC);
    else   GPIO_BSRR(GPIOD_BASE) = (1u << (OLED_DC + 16));
}

static void oled_sck(int v) {
    if (v) GPIO_BSRR(GPIOD_BASE) = (1u << OLED_SCK);
    else   GPIO_BSRR(GPIOD_BASE) = (1u << (OLED_SCK + 16));
}

static void oled_mosi(int v) {
    if (v) GPIO_BSRR(GPIOD_BASE) = (1u << OLED_MOSI);
    else   GPIO_BSRR(GPIOD_BASE) = (1u << (OLED_MOSI + 16));
}

static void oled_rst(int v) {
    if (v) GPIO_BSRR(GPIOD_BASE) = (1u << OLED_RST);
    else   GPIO_BSRR(GPIOD_BASE) = (1u << (OLED_RST + 16));
}

static void oled_spi_write(uint8_t b) {
    for (int i = 7; i >= 0; i--) {
        oled_sck(0);
        oled_mosi((b >> i) & 1);
        sp_delay_us(1);
        oled_sck(1);
        sp_delay_us(1);
    }
}

static void oled_cmd(uint8_t c) { oled_dc(0); oled_spi_write(c); }
static void oled_data(uint8_t d) { oled_dc(1); oled_spi_write(d); }

void oled_init(void) {
    oled_pin_init();
    oled_rst(0);
    sp_delay_ms(50);
    oled_rst(1);
    sp_delay_ms(50);
    oled_cmd(0xAE); /* display off */
    oled_cmd(0xD5); oled_cmd(0x80); /* clock divide */
    oled_cmd(0xA8); oled_cmd(0x3F); /* multiplex 1/64 */
    oled_cmd(0xD3); oled_cmd(0x00); /* display offset */
    oled_cmd(0x40); /* start line */
    oled_cmd(0x8D); oled_cmd(0x14); /* charge pump on */
    oled_cmd(0x20); oled_cmd(0x00); /* horizontal addressing */
    oled_cmd(0xA1); /* seg remap */
    oled_cmd(0xC8); /* com scan dir */
    oled_cmd(0xDA); oled_cmd(0x12); /* com pins */
    oled_cmd(0x81); oled_cmd(0xCF); /* contrast */
    oled_cmd(0xD9); oled_cmd(0xF1); /* pre-charge */
    oled_cmd(0xDB); oled_cmd(0x40); /* VCOM deselect */
    oled_cmd(0xA4); oled_cmd(0xA6);
    oled_cmd(0xAF); /* display on */
}

/* Simple 5x7 font (printable ASCII subset). */
static const uint8_t FONT5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* space (index 0) */
    /* ... full font omitted for brevity; placeholder glyphs ... */
};
#define FONT_GLYPH_COUNT (sizeof(FONT5x7)/sizeof(FONT5x7[0]))

static uint8_t g_framebuf[128*64/8]; /* 1024 bytes, 1 bpp horizontal pages */

void oled_clear(void) { memset(g_framebuf, 0, sizeof(g_framebuf)); }

void oled_refresh(void) {
    for (uint8_t page = 0; page < 8; page++) {
        oled_cmd(0xB0u + page);     /* page address */
        oled_cmd(0x02);             /* low col */
        oled_cmd(0x10);             /* high col */
        for (uint8_t col = 0; col < 128; col++) {
            oled_data(g_framebuf[page * 128 + col]);
        }
    }
}

void oled_draw_text(uint8_t x, uint8_t y, const char *s) {
    /* x in pixels, y in pixel rows (0..63). Each glyph is 5x7. */
    uint8_t page = y / 8;
    while (*s) {
        uint8_t ch = (uint8_t)*s++;
        if (ch < 32 || ch > 127) ch = 32;
        uint8_t idx = (ch - 32) % FONT_GLYPH_COUNT;
        for (int c = 0; c < 5; c++) {
            uint8_t coldata = FONT5x7[idx][c];
            if (x < 128) g_framebuf[page * 128 + x] = coldata << (y % 8);
            x++;
        }
        x++; /* 1px gap */
    }
}

void oled_draw_bar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t fill) {
    /* Vertical bar from (x,y) width w, height h, filled to `fill` (0..h) */
    uint8_t filled = fill;
    if (filled > h) filled = h;
    for (uint8_t dx = 0; dx < w; dx++) {
        for (uint8_t dy = 0; dy < h; dy++) {
            uint8_t px = x + dx;
            uint8_t py = y + dy;
            uint8_t page = py / 8;
            uint8_t bit = py % 8;
            if (dy < filled) g_framebuf[page * 128 + px] |= (1u << bit);
            else             g_framebuf[page * 128 + px] &= ~(1u << bit);
        }
    }
}