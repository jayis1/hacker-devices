/*
 * drivers/oled.c — SH1107 OLED driver + menu rendering
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Drives a 1.3" 128x96 monochrome OLED (SH1107) over SPI1. Provides a
 * tiny framebuffer, font, and a 4-line menu renderer for on-device UI.
 */
#include "../board.h"
#include "../registers.h"

/* ---- Commands ----------------------------------------------------- */
#define OLED_CMD_SET_CONTRAST    0x81
#define OLED_CMD_DISPLAY_ON      0xAF
#define OLED_CMD_DISPLAY_OFF     0xAE
#define OLED_CMD_SEG_REMAP       0xA1
#define OLED_CMD_SCAN_REV        0xC8
#define OLED_CMD_COL_OFFSET      0x00

/* ---- Framebuffer --------------------------------------------------- */
/* 128x96, 1 bpp, page-addressed: 96/8 = 12 pages, 128 columns */
#define FB_PAGES   12
#define FB_COLS    128
static uint8_t fb[FB_PAGES][FB_COLS];

static void oled_cmd(uint8_t c)
{
    spi_cs_low(OLED_CS_PORT, OLED_CS_PIN);
    GPIO_BSRR(OLED_DC_PORT) = GPIO_BR(OLED_DC_PIN); /* cmd */
    spi_xfer(SPI1_BASE, c);
    spi_cs_high(OLED_CS_PORT, OLED_CS_PIN);
}

static void oled_data(uint8_t d)
{
    spi_cs_low(OLED_CS_PORT, OLED_CS_PIN);
    GPIO_BSRR(OLED_DC_PORT) = GPIO_BS(OLED_DC_PIN); /* data */
    spi_xfer(SPI1_BASE, d);
    spi_cs_high(OLED_CS_PORT, OLED_CS_PIN);
}

void oled_init(void)
{
    /* Reset */
    GPIO_BSRR(OLED_RST_PORT) = GPIO_BR(OLED_RST_PIN);
    board_delay_ms(20);
    GPIO_BSRR(OLED_RST_PORT) = GPIO_BS(OLED_RST_PIN);
    board_delay_ms(100);

    oled_cmd(OLED_CMD_DISPLAY_OFF);
    oled_cmd(0x40);                 /* set display start line */
    oled_cmd(OLED_CMD_SEG_REMAP);
    oled_cmd(OLED_CMD_SCAN_REV);
    oled_cmd(0xA4);                 /* normal display */
    oled_cmd(0xA6);                 /* normal (not inverted) */
    oled_cmd(0xA8); oled_cmd(0x5F); /* multiplex = 96-1 */
    oled_cmd(0xD3); oled_cmd(0x00); /* display offset */
    oled_cmd(0xD5); oled_cmd(0x50); /* clock divide */
    oled_cmd(0xD9); oled_cmd(0x22); /* pre-charge period */
    oled_cmd(0xDA); oled_cmd(0x12); /* seg pins hardware config */
    oled_cmd(0xDB); oled_cmd(0x20); /* VCOM deselect */
    oled_cmd(0x20); oled_cmd(0x00); /* page addressing mode */
    oled_cmd(OLED_CMD_SET_CONTRAST); oled_cmd(0x7F);
    oled_cmd(0x8D); oled_cmd(0x14); /* charge pump on */
    oled_cmd(OLED_CMD_DISPLAY_ON);
}

void oled_clear(void)
{
    for (uint8_t p = 0; p < FB_PAGES; p++)
        for (uint8_t c = 0; c < FB_COLS; c++) fb[p][c] = 0;
}

void oled_flush(void)
{
    for (uint8_t p = 0; p < FB_PAGES; p++) {
        oled_cmd(0xB0 + p);        /* page */
        oled_cmd(0x00);            /* col low */
        oled_cmd(0x10);            /* col high */
        for (uint8_t c = 0; c < FB_COLS; c++) oled_data(fb[p][c]);
    }
}

/* ---- Tiny 5x7 font (printable ASCII subset) ----------------------- */
static const uint8_t font5x7[][5] = {
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
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    {0x7F,0x09,0x09,0x09,0x7F}, /* A */
    {0x7F,0x49,0x49,0x49,0x41}, /* B */
    {0x3F,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x48,0x48,0x48,0x40}, /* F */
    {0x3E,0x41,0x41,0x49,0x4A}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x48,0x48,0x48,0x30}, /* P */
    {0x3E,0x41,0x49,0x45,0x3E}, /* Q */
    {0x7F,0x48,0x4C,0x4A,0x31}, /* R */
    {0x31,0x45,0x45,0x45,0x42}, /* S */
    {0x40,0x40,0x7F,0x40,0x40}, /* T */
    {0x3E,0x40,0x40,0x40,0x3E}, /* U */
    {0x1E,0x20,0x40,0x20,0x1E}, /* V */
    {0x7F,0x10,0x08,0x10,0x7F}, /* W */
    {0x41,0x22,0x1C,0x22,0x41}, /* X */
    {0x43,0x40,0x40,0x40,0x3F}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
    {0x00,0x60,0x60,0x00,0x00}, /* . */
    {0x44,0x44,0x7C,0x44,0x44}, /* + */
    {0x00,0x40,0x40,0x00,0x00}, /* - */
    {0x44,0x44,0x5C,0x44,0x44}, /* /  approx */
    {0x3E,0x45,0x49,0x31,0x00}, /* ? */
    {0x7C,0x10,0x10,0x10,0x7C}, /* [ */
    {0x08,0x08,0x08,0x08,0x08}, /* _ */
};

static uint8_t char_index(char c)
{
    switch (c) {
    case '0'...'9': return (uint8_t)(c - '0');
    case ' ': return 10;
    case '!': return 11;
    case 'A'...'Z': return (uint8_t)(c - 'A' + 12);
    case '.': return 38;
    case '+': return 39;
    case '-': return 40;
    case '/': return 41;
    case '?': return 42;
    case '[': return 43;
    case '_': return 44;
    default: return 10;
    }
}

void oled_draw_char(uint8_t x, uint8_t page, char c)
{
    uint8_t idx = char_index(c);
    const uint8_t *glyph = font5x7[idx];
    for (uint8_t i = 0; i < 5; i++) {
        if (x + i < FB_COLS) fb[page][x + i] = glyph[i];
    }
}

void oled_draw_string(uint8_t x, uint8_t page, const char *s)
{
    while (*s && x < FB_COLS - 5) {
        oled_draw_char(x, page, *s);
        x += 6;   /* 5 px + 1 px spacing */
        s++;
    }
}

void oled_invert_line(uint8_t page)
{
    for (uint8_t c = 0; c < FB_COLS; c++) fb[page][c] ^= 0xFF;
}

/* ---- Menu renderer ------------------------------------------------- */
typedef struct {
    const char *title;
    const char *items[8];
    uint8_t n_items;
    uint8_t selected;
} oled_menu_t;

void oled_render_menu(const oled_menu_t *m)
{
    oled_clear();
    oled_draw_string(0, 0, m->title);
    /* separator line in page 1 */
    for (uint8_t c = 0; c < FB_COLS; c++) fb[1][c] = 0x01;
    for (uint8_t i = 0; i < m->n_items && i < 7; i++) {
        uint8_t pg = 2 + i;
        oled_draw_string(2, pg, m->items[i]);
        if (i == m->selected) oled_invert_line(pg);
    }
    oled_flush();
}

void oled_render_status(uint8_t bat_pct, const char *mode,
                         uint32_t range_cm, int32_t vel_mmps,
                         int16_t rcs_qdb, uint8_t taps, uint8_t armed)
{
    char line[22];
    oled_clear();
    /* Line 0: device + battery */
    oled_draw_string(0, 0, "RADARPHANTOM");
    int32_t l = 0;
    /* format battery */
    line[0] = 'b'; line[1] = 'a'; line[2] = 't';
    line[3] = ' '; line[4] = ':';
    line[5] = '0' + (bat_pct / 100) % 10;
    line[6] = '0' + (bat_pct / 10) % 10;
    line[7] = '0' + bat_pct % 10;
    line[8] = '%';
    line[9] = 0;
    oled_draw_string(80, 0, line);
    /* separator */
    for (uint8_t c = 0; c < FB_COLS; c++) fb[1][c] = 0x01;
    /* Line 2: mode + armed */
    oled_draw_string(0, 2, "MODE:");
    oled_draw_string(30, 2, mode);
    if (armed) oled_draw_string(100, 2, "ARM");
    /* Line 4: range */
    oled_draw_string(0, 4, "RNG");
    /* range_cm to string "xxx.x m" simplified to integer meters */
    uint32_t m = range_cm / 100;
    l = 0;
    if (m >= 100) line[l++] = '0' + (m / 100) % 10;
    if (m >= 10)  line[l++] = '0' + (m / 10) % 10;
    line[l++] = '0' + m % 10;
    line[l++] = ' '; line[l++] = 'M'; line[l] = 0;
    oled_draw_string(30, 4, line);
    /* Line 6: velocity km/h */
    int32_t kmh = vel_mmps / 278;     /* mm/s -> km/h approx (/3.6/1000) */
    oled_draw_string(0, 6, "VEL");
    l = 0;
    if (kmh < 0) { line[l++] = '-'; kmh = -kmh; }
    if (kmh >= 100) line[l++] = '0' + (kmh / 100) % 10;
    if (kmh >= 10)  line[l++] = '0' + (kmh / 10) % 10;
    line[l++] = '0' + kmh % 10;
    line[l++] = ' '; line[l++] = 'K'; line[l++] = 'H'; line[l] = 0;
    oled_draw_string(30, 6, line);
    /* Line 8: RCS + taps */
    oled_draw_string(0, 8, "RCS");
    l = 0;
    if (rcs_qdb < 0) { line[l++] = '-'; rcs_qdb = -rcs_qdb; }
    line[l++] = '0' + (rcs_qdb / 100) % 10;
    line[l++] = '0' + (rcs_qdb / 10) % 10;
    line[l++] = '0' + rcs_qdb % 10;
    line[l] = 0;
    oled_draw_string(30, 8, line);
    oled_draw_string(70, 8, "TAP");
    oled_draw_char(100, 8, '0' + taps);
    /* Line 10: hints */
    oled_draw_string(0, 10, "OK MENU L/R VAL");
    oled_flush();
}