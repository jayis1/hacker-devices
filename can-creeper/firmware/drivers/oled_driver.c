/**
 * @file oled_driver.c
 * @brief SSD1306 OLED Display Driver Implementation
 *
 * 128x64 monochrome OLED over I²C (TWIM0).
 * Uses internal framebuffer, flushed to display on demand.
 */

#include "oled_driver.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* SSD1306 commands */
#define SSD1306_CMD_SET_CONTRAST      0x81
#define SSD1306_CMD_DISPLAY_ON        0xAF
#define SSD1306_CMD_DISPLAY_OFF       0xAE
#define SSD1306_CMD_DISPLAY_ALL_ON    0xA5
#define SSD1306_CMD_DISPLAY_NORMAL    0xA6
#define SSD1306_CMD_INVERT_DISPLAY    0xA7
#define SSD1306_CMD_SET_MUX_RATIO     0xA8
#define SSD1306_CMD_SET_DISPLAY_OFFSET 0xD3
#define SSD1306_CMD_SET_START_LINE    0x40
#define SSD1306_CMD_SEG_REMAP         0xA1
#define SSD1306_CMD_COM_SCAN_DEC      0xC8
#define SSD1306_CMD_SET_COM_PINS      0xDA
#define SSD1306_CMD_SET_DISPLAY_CLK   0xD5
#define SSD1306_CMD_SET_PRECHARGE     0xD9
#define SSD1306_CMD_SET_VCOM_DETECT   0xDB
#define SSD1306_CMD_CHARGE_PUMP       0x8D
#define SSD1306_CMD_MEMORY_MODE       0x20
#define SSD1306_CMD_SET_COL_ADDR      0x21
#define SSD1306_CMD_SET_PAGE_ADDR     0x22

static uint8_t g_framebuffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static bool g_initialized = false;

/* 5x7 font (ASCII 32-127, 5 bytes per char) */
static const uint8_t font5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* $ */
    {0x23,0x13,0x08,0x64,0x62}, /* % */
    {0x36,0x49,0x55,0x22,0x50}, /* & */
    {0x00,0x05,0x03,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* ( */
    {0x00,0x41,0x22,0x1C,0x00}, /* ) */
    {0x08,0x2A,0x1C,0x2A,0x08}, /* * */
    {0x08,0x08,0x3E,0x08,0x08}, /* + */
    {0x00,0x50,0x30,0x00,0x00}, /* , */
    {0x08,0x08,0x08,0x08,0x08}, /* - */
    {0x00,0x60,0x60,0x00,0x00}, /* . */
    {0x20,0x10,0x08,0x04,0x02}, /* / */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
    {0x00,0x36,0x36,0x00,0x00}, /* : */
    {0x00,0x56,0x36,0x00,0x00}, /* ; */
    {0x00,0x08,0x14,0x22,0x41}, /* < */
    {0x14,0x14,0x14,0x14,0x14}, /* = */
    {0x41,0x22,0x14,0x08,0x00}, /* > */
    {0x02,0x01,0x51,0x09,0x06}, /* ? */
    {0x32,0x49,0x79,0x41,0x3E}, /* @ */
    {0x7E,0x11,0x11,0x11,0x7E}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x01,0x01}, /* F */
    {0x3E,0x41,0x41,0x51,0x32}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x04,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */
    {0x7F,0x20,0x18,0x20,0x7F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x03,0x04,0x78,0x04,0x03}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
    {0x00,0x00,0x7F,0x41,0x41}, /* [ */
    {0x02,0x04,0x08,0x10,0x20}, /* \ */
    {0x41,0x41,0x7F,0x00,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04}, /* ^ */
    {0x40,0x40,0x40,0x40,0x40}, /* _ */
    {0x00,0x01,0x02,0x04,0x00}, /* ` */
    {0x20,0x54,0x54,0x54,0x78}, /* a */
    {0x7F,0x48,0x44,0x44,0x38}, /* b */
    {0x38,0x44,0x44,0x44,0x20}, /* c */
    {0x38,0x44,0x44,0x48,0x7F}, /* d */
    {0x38,0x54,0x54,0x54,0x18}, /* e */
    {0x08,0x7E,0x09,0x01,0x02}, /* f */
    {0x08,0x14,0x54,0x54,0x3C}, /* g */
    {0x7F,0x08,0x04,0x04,0x78}, /* h */
    {0x00,0x44,0x7D,0x40,0x00}, /* i */
    {0x20,0x40,0x44,0x3D,0x00}, /* j */
    {0x00,0x7F,0x10,0x28,0x44}, /* k */
    {0x00,0x41,0x7F,0x40,0x00}, /* l */
    {0x7C,0x04,0x18,0x04,0x78}, /* m */
    {0x7C,0x08,0x04,0x04,0x78}, /* n */
    {0x38,0x44,0x44,0x44,0x38}, /* o */
    {0x7C,0x14,0x14,0x14,0x08}, /* p */
    {0x08,0x14,0x14,0x18,0x7C}, /* q */
    {0x7C,0x08,0x04,0x04,0x08}, /* r */
    {0x48,0x54,0x54,0x54,0x20}, /* s */
    {0x04,0x3F,0x44,0x40,0x20}, /* t */
    {0x3C,0x40,0x40,0x20,0x7C}, /* u */
    {0x1C,0x20,0x40,0x20,0x1C}, /* v */
    {0x3C,0x40,0x30,0x40,0x3C}, /* w */
    {0x44,0x28,0x10,0x28,0x44}, /* x */
    {0x0C,0x50,0x50,0x50,0x3C}, /* y */
    {0x44,0x64,0x54,0x4C,0x44}, /* z */
    {0x00,0x08,0x36,0x41,0x00}, /* { */
    {0x00,0x00,0x7F,0x00,0x00}, /* | */
    {0x00,0x41,0x36,0x08,0x00}, /* } */
    {0x08,0x08,0x2A,0x1C,0x08}, /* ~ */
    {0x08,0x1C,0x2A,0x08,0x08}, /* ~ (duplicate) */
};

/* 8x16 font — simplified: just double-height 5x7 for now */
/* In production, use a proper 8x16 font table */

static int twi_write(uint8_t addr, const uint8_t *data, uint8_t len) {
    NRF_TWIM0->ADDRESS = addr;
    NRF_TWIM0->TXD_PTR = (uint32_t)data;
    NRF_TWIM0->TXD_MAXCNT = len;
    NRF_TWIM0->TASKS_STARTTX = 1;
    while (!NRF_TWIM0->EVENTS_TXDSENT && !NRF_TWIM0->EVENTS_ERROR) {}
    if (NRF_TWIM0->EVENTS_ERROR) { NRF_TWIM0->EVENTS_ERROR = 0; return -1; }
    NRF_TWIM0->EVENTS_TXDSENT = 0;
    NRF_TWIM0->TASKS_STOP = 1;
    while (!NRF_TWIM0->EVENTS_STOPPED) {}
    NRF_TWIM0->EVENTS_STOPPED = 0;
    return 0;
}

static int oled_write_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};  /* Co=0, D/C#=0 (command) */
    return twi_write(SSD1306_I2C_ADDR, buf, 2);
}

static int oled_write_data(const uint8_t *data, uint16_t len) {
    /* For bulk data, prepend 0x40 (Co=0, D/C#=1) */
    static uint8_t buf[129];  /* 1 control + 128 data */
    uint16_t pos = 0;
    while (pos < len) {
        uint16_t chunk = (len - pos > 128) ? 128 : (len - pos);
        buf[0] = 0x40;
        memcpy(&buf[1], &data[pos], chunk);
        int ret = twi_write(SSD1306_I2C_ADDR, buf, chunk + 1);
        if (ret != 0) return ret;
        pos += chunk;
    }
    return 0;
}

int oled_init(void) {
    /* Configure TWIM0 */
    NRF_TWIM0->PSEL_SDA = I2C_SDA_PIN;
    NRF_TWIM0->PSEL_SCL = I2C_SCL_PIN;
    NRF_TWIM0->FREQUENCY = TWIM_FREQ_400KBPS;
    NRF_TWIM0->ENABLE = 0x06;  /* TWIM ENABLE */

    /* SSD1306 init sequence */
    oled_write_cmd(SSD1306_CMD_DISPLAY_OFF);
    oled_write_cmd(SSD1306_CMD_SET_DISPLAY_CLK); oled_write_cmd(0x80);
    oled_write_cmd(SSD1306_CMD_SET_MUX_RATIO); oled_write_cmd(0x3F);
    oled_write_cmd(SSD1306_CMD_SET_DISPLAY_OFFSET); oled_write_cmd(0x00);
    oled_write_cmd(SSD1306_CMD_SET_START_LINE | 0x00);
    oled_write_cmd(SSD1306_CMD_CHARGE_PUMP); oled_write_cmd(0x14);
    oled_write_cmd(SSD1306_CMD_MEMORY_MODE); oled_write_cmd(0x00);
    oled_write_cmd(SSD1306_CMD_SEG_REMAP);
    oled_write_cmd(SSD1306_CMD_COM_SCAN_DEC);
    oled_write_cmd(SSD1306_CMD_SET_COM_PINS); oled_write_cmd(0x12);
    oled_write_cmd(SSD1306_CMD_SET_CONTRAST); oled_write_cmd(0x7F);
    oled_write_cmd(SSD1306_CMD_SET_PRECHARGE); oled_write_cmd(0xF1);
    oled_write_cmd(SSD1306_CMD_SET_VCOM_DETECT); oled_write_cmd(0x40);
    oled_write_cmd(SSD1306_CMD_DISPLAY_ALL_ON);
    oled_write_cmd(SSD1306_CMD_DISPLAY_NORMAL);
    oled_write_cmd(SSD1306_CMD_DISPLAY_ON);

    oled_clear();
    oled_display();
    g_initialized = true;
    return 0;
}

int oled_clear(void) {
    memset(g_framebuffer, 0, sizeof(g_framebuffer));
    return 0;
}

int oled_display(void) {
    oled_write_cmd(SSD1306_CMD_SET_COL_ADDR); oled_write_cmd(0); oled_write_cmd(127);
    oled_write_cmd(SSD1306_CMD_SET_PAGE_ADDR); oled_write_cmd(0); oled_write_cmd(7);
    return oled_write_data(g_framebuffer, sizeof(g_framebuffer));
}

int oled_set_pixel(uint8_t x, uint8_t y, bool on) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return -1;
    uint16_t idx = x + (y / 8) * SSD1306_WIDTH;
    if (on) g_framebuffer[idx] |= (1 << (y & 7));
    else    g_framebuffer[idx] &= ~(1 << (y & 7));
    return 0;
}

int oled_draw_char(uint8_t x, uint8_t y, char c, oled_font_t font) {
    if (c < 32 || c > 127) c = '?';
    uint8_t idx = (uint8_t)(c - 32);
    uint8_t char_w = (font == OLED_FONT_5X7) ? 5 : 8;
    uint8_t char_h = (font == OLED_FONT_5X7) ? 7 : 16;

    for (uint8_t col = 0; col < char_w; col++) {
        uint8_t col_data = font5x7[idx][col];
        for (uint8_t row = 0; row < char_h; row++) {
            bool pixel_on = (col_data >> row) & 0x01;
            oled_set_pixel(x + col, y + row, pixel_on);
        }
    }
    return 0;
}

int oled_draw_string(uint8_t x, uint8_t y, const char *str, oled_font_t font) {
    uint8_t char_w = (font == OLED_FONT_5X7) ? 6 : 8;  /* 5+1 spacing or 8 */
    uint8_t cx = x;
    while (*str) {
        oled_draw_char(cx, y, *str, font);
        cx += char_w;
        str++;
    }
    return 0;
}

int oled_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    int dx = (int)x1 - (int)x0, dy = (int)y1 - (int)y0;
    int steps = (abs(dx) > abs(dy)) ? abs(dx) : abs(dy);
    for (int i = 0; i <= steps; i++) {
        uint8_t x = (uint8_t)(x0 + dx * i / steps);
        uint8_t y = (uint8_t)(y0 + dy * i / steps);
        oled_set_pixel(x, y, true);
    }
    return 0;
}

int oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool fill) {
    if (fill) {
        for (uint8_t iy = y; iy < y + h; iy++)
            for (uint8_t ix = x; ix < x + w; ix++)
                oled_set_pixel(ix, iy, true);
    } else {
        oled_draw_line(x, y, x + w - 1, y);
        oled_draw_line(x, y + h - 1, x + w - 1, y + h - 1);
        oled_draw_line(x, y, x, y + h - 1);
        oled_draw_line(x + w - 1, y, x + w - 1, y + h - 1);
    }
    return 0;
}

int oled_set_contrast(uint8_t contrast) {
    oled_write_cmd(SSD1306_CMD_SET_CONTRAST);
    return oled_write_cmd(contrast);
}

int oled_power_on(void)  { return oled_write_cmd(SSD1306_CMD_DISPLAY_ON); }
int oled_power_off(void) { return oled_write_cmd(SSD1306_CMD_DISPLAY_OFF); }
int oled_invert(bool inv) { return oled_write_cmd(inv ? SSD1306_CMD_INVERT_DISPLAY : SSD1306_CMD_DISPLAY_NORMAL); }
