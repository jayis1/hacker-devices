/*
 * TRACE-REAPER — local UI driver
 *
 * Buttons, status LEDs, and a 0.96" mono OLED over I2C1 (SSD1306). The OLED
 * is shared between tamper (interrupt-driven I2C) and UI; here we just draw
 * the local status line. Full UI lives in the companion app.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "ui.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

#define OLED_ADDR 0x3C
#define OLED_W 128
#define OLED_H 64

static volatile uint8_t g_btn_mode = 0;
static volatile uint8_t g_btn_arm = 0;
static volatile uint8_t g_btn_dump = 0;
static volatile uint32_t g_btn_debounce = 0;

/* ---- I2C write helpers (reuse pattern from tamper.c) ---- */
static void oled_cmd(uint8_t c)
{
    I2C1->CR2 = ((uint32_t)OLED_ADDR << 1) | (2U << 16) | (1U << 13);
    while ((I2C1->ISR & (1U << 0)) == 0) ;
    I2C1->TXDR = 0x00;       /* control = command */
    while ((I2C1->ISR & (1U << 0)) == 0) ;
    I2C1->TXDR = c;
    while ((I2C1->ISR & (1U << 5)) == 0) ;
    I2C1->ICR = (1U << 5);
}

static void oled_data(uint8_t d)
{
    I2C1->CR2 = ((uint32_t)OLED_ADDR << 1) | (2U << 16) | (1U << 13);
    while ((I2C1->ISR & (1U << 0)) == 0) ;
    I2C1->TXDR = 0x40;       /* control = data */
    while ((I2C1->ISR & (1U << 0)) == 0) ;
    I2C1->TXDR = d;
    while ((I2C1->ISR & (1U << 5)) == 0) ;
    I2C1->ICR = (1U << 5);
}

void ui_init(void)
{
    /* Buttons as inputs with pull-up; active low */
    gpio_mode(GPIOC, 13, 0); gpio_pupd(GPIOC, 13, 1);
    gpio_mode(GPIOC, 14, 0); gpio_pupd(GPIOC, 14, 1);
    gpio_mode(GPIOC, 15, 0); gpio_pupd(GPIOC, 15, 1);

    /* LEDs as outputs */
    gpio_mode(GPIOB, 0, 1); gpio_mode(GPIOB, 1, 1); gpio_mode(GPIOB, 2, 1);
    gpio_out(GPIOB, 0, 0); gpio_out(GPIOB, 1, 0); gpio_out(GPIOB, 2, 0);

    /* SSD1306 init sequence */
    oled_cmd(0xAE); /* display off */
    oled_cmd(0xD5); oled_cmd(0x80); /* clock divide */
    oled_cmd(0xA8); oled_cmd(0x3F); /* mux 1/64 */
    oled_cmd(0xD3); oled_cmd(0x00); /* display offset */
    oled_cmd(0x40); /* start line */
    oled_cmd(0x8D); oled_cmd(0x14); /* charge pump on */
    oled_cmd(0x20); oled_cmd(0x00); /* addressing mode horizontal */
    oled_cmd(0xA1); /* segment remap */
    oled_cmd(0xC8); /* COM scan dir */
    oled_cmd(0xDA); oled_cmd(0x12); /* COM pins */
    oled_cmd(0xD9); oled_cmd(0xF1); /* precharge */
    oled_cmd(0xDB); oled_cmd(0x40); /* VCOMH */
    oled_cmd(0xA4); /* display follows RAM */
    oled_cmd(0xA6); /* normal display */
    oled_cmd(0xAF); /* display on */

    ui_clear();
    ui_text(0, "TRACE-REAPER");
    ui_text(2, "idle");
}

void ui_clear(void)
{
    for (uint8_t p = 0; p < 8; p++) {
        oled_cmd(0xB0 + p);  /* page */
        oled_cmd(0x00);      /* col low */
        oled_cmd(0x10);      /* col high */
        for (uint8_t c = 0; c < OLED_W; c++) oled_data(0x00);
    }
}

/* 5x8 font subset: just spaces, digits, uppercase. We only need a status line. */
static const uint8_t font5x8[128][5] = {
    [0]=  {0x00,0x00,0x00,0x00,0x00},
    /* space at index 32 */
    [32]= {0x00,0x00,0x00,0x00,0x00},
    /* digits 48..57 */
    [48]= {0x3E,0x51,0x49,0x45,0x3E},
    [49]= {0x00,0x42,0x7F,0x40,0x00},
    [50]= {0x42,0x61,0x51,0x49,0x46},
    [51]= {0x21,0x41,0x45,0x4B,0x31},
    [52]= {0x18,0x14,0x12,0x7F,0x10},
    [53]= {0x27,0x45,0x45,0x45,0x39},
    [54]= {0x3C,0x4A,0x49,0x49,0x30},
    [55]= {0x01,0x71,0x09,0x05,0x03},
    [56]= {0x36,0x49,0x49,0x49,0x36},
    [57]= {0x06,0x49,0x49,0x29,0x1E},
    /* uppercase A..Z at 65..90 */
    [65]= {0x7E,0x11,0x11,0x11,0x7E},
    [66]= {0x7F,0x49,0x49,0x49,0x36},
    [67]= {0x3E,0x41,0x41,0x41,0x22},
    [68]= {0x7F,0x41,0x41,0x22,0x1C},
    [69]= {0x7F,0x49,0x49,0x49,0x41},
    [70]= {0x7F,0x09,0x09,0x09,0x01},
    [71]= {0x3E,0x41,0x49,0x49,0x7A},
    [72]= {0x7F,0x08,0x08,0x08,0x7F},
    [73]= {0x00,0x41,0x7F,0x41,0x00},
    [74]= {0x20,0x40,0x41,0x3F,0x01},
    [75]= {0x7F,0x08,0x14,0x22,0x41},
    [76]= {0x7F,0x40,0x40,0x40,0x40},
    [77]= {0x7F,0x02,0x0C,0x02,0x7F},
    [78]= {0x7F,0x04,0x08,0x10,0x7F},
    [79]= {0x3E,0x41,0x41,0x41,0x3E},
    [80]= {0x7F,0x09,0x09,0x09,0x06},
    [81]= {0x3E,0x41,0x51,0x21,0x5E},
    [82]= {0x7F,0x09,0x19,0x29,0x46},
    [83]= {0x46,0x49,0x49,0x49,0x31},
    [84]= {0x01,0x01,0x7F,0x01,0x01},
    [85]= {0x3F,0x40,0x40,0x40,0x3F},
    [86]= {0x1F,0x20,0x40,0x20,0x1F},
    [87]= {0x3F,0x40,0x38,0x40,0x3F},
    [88]= {0x63,0x14,0x08,0x14,0x63},
    [89]= {0x07,0x08,0x70,0x08,0x07},
    [90]= {0x61,0x51,0x49,0x45,0x43},
    /* - at 45 */
    [45]= {0x00,0x00,0x7F,0x00,0x00},
    [46]= {0x00,0x00,0x60,0x00,0x00},
    [58]= {0x00,0x36,0x36,0x00,0x00},
};

void ui_text(uint8_t row, const char *s)
{
    uint8_t page = row;
    oled_cmd(0xB0 + page);
    oled_cmd(0x00);
    oled_cmd(0x10);
    for (uint8_t i = 0; s[i] && i < 21; i++) {
        uint8_t ch = (uint8_t)s[i];
        if (ch >= 128) ch = 0;
        for (uint8_t b = 0; b < 5; b++) oled_data(font5x8[ch][b]);
        oled_data(0x00); /* column gap */
    }
}

void ui_led_status(uint8_t on) { gpio_out(GPIOB, 0, on ? 1 : 0); }
void ui_led_mode(uint8_t on)   { gpio_out(GPIOB, 1, on ? 1 : 0); }
void ui_led_arm(uint8_t on)    { gpio_out(GPIOB, 2, on ? 1 : 0); }

/* ---- Button polling ---- */
void ui_poll(uint32_t now_ms)
{
    if (now_ms - g_btn_debounce < 50) return;
    if (!gpio_in(GPIOC, 13)) { g_btn_mode = 1; g_btn_debounce = now_ms; }
    if (!gpio_in(GPIOC, 14)) { g_btn_arm = 1;  g_btn_debounce = now_ms; }
    if (!gpio_in(GPIOC, 15)) { g_btn_dump = 1; g_btn_debounce = now_ms; }
}

int ui_btn_mode_take(void) { int b = g_btn_mode; g_btn_mode = 0; return b; }
int ui_btn_arm_take(void)  { int b = g_btn_arm;  g_btn_arm = 0;  return b; }
int ui_btn_dump_take(void) { int b = g_btn_dump; g_btn_dump = 0; return b; }