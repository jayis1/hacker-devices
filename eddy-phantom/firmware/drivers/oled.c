/*
 * eddy-phantom — oled.c
 * SSD1306 OLED display driver (128x64) via bit-banged I2C.
 * Also provides MAX17048 battery fuel gauge reading via shared I2C bus.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* ── I2C bit-bang implementation ──────────────────────────────── */
#define I2C_DELAY_COUNT   15   /* cycles for I2C timing (~100 kHz) */

static void i2c_delay(void)
{
    for (volatile int i = 0; i < I2C_DELAY_COUNT; i++);
}

static void i2c_scl_high(void)
{
    gpio_set(GPIO(OLED_SCL_PORT), OLED_SCL_PIN);
    i2c_delay();
}

static void i2c_scl_low(void)
{
    gpio_clr(GPIO(OLED_SCL_PORT), OLED_SCL_PIN);
    i2c_delay();
}

static void i2c_sda_high(void)
{
    gpio_set(GPIO(OLED_SDA_PORT), OLED_SDA_PIN);
    i2c_delay();
}

static void i2c_sda_low(void)
{
    gpio_clr(GPIO(OLED_SDA_PORT), OLED_SDA_PIN);
    i2c_delay();
}

static uint8_t i2c_sda_read(void)
{
    /* Set SDA as input (switch to input mode) */
    volatile gpio_regs_t *g = GPIO(B);
    g->MODER = (g->MODER & ~(3U << (OLED_SDA_PIN * 2))) |
               (GPIO_MODE_INPUT << (OLED_SDA_PIN * 2));
    g->PUPDR = (g->PUPDR & ~(3U << (OLED_SDA_PIN * 2))) |
               (GPIO_PUPD_PULLUP << (OLED_SDA_PIN * 2));

    i2c_delay();
    uint8_t val = (uint8_t)gpio_get(GPIO(OLED_SDA_PORT), OLED_SDA_PIN);

    /* Switch back to output (open-drain) */
    g->MODER = (g->MODER & ~(3U << (OLED_SDA_PIN * 2))) |
               (GPIO_MODE_OUTPUT << (OLED_SDA_PIN * 2));
    g->OTYPER = (g->OTYPER & ~(1U << OLED_SDA_PIN)) |
                (GPIO_OTYPE_OD << OLED_SDA_PIN);

    return val;
}

static void i2c_start(void)
{
    i2c_sda_high();
    i2c_scl_high();
    i2c_sda_low();
    i2c_scl_low();
}

static void i2c_stop(void)
{
    i2c_sda_low();
    i2c_scl_high();
    i2c_sda_high();
}

static uint8_t i2c_write_byte(uint8_t data)
{
    for (int i = 7; i >= 0; i--) {
        if (data & (1U << i))
            i2c_sda_high();
        else
            i2c_sda_low();
        i2c_scl_high();
        i2c_scl_low();
    }

    /* Read ACK */
    i2c_scl_high();
    uint8_t ack = i2c_sda_read();
    i2c_scl_low();

    return ack;  /* 0 = ACK, 1 = NACK */
}

static uint8_t i2c_read_byte(uint8_t ack)
{
    uint8_t data = 0;

    for (int i = 7; i >= 0; i--) {
        i2c_scl_high();
        if (i2c_sda_read())
            data |= (1U << i);
        i2c_scl_low();
    }

    /* Send ACK/NACK */
    if (ack)
        i2c_sda_low();
    else
        i2c_sda_high();
    i2c_scl_high();
    i2c_scl_low();
    i2c_sda_high();

    return data;
}

/* ── SSD1306 constants ────────────────────────────────────────── */
#define SSD1306_ADDR       0x3C   /* I2C address (SA0=0) */
#define SSD1306_CMD        0x00   /* Co=0, D/C=0 → command */
#define SSD1306_DATA       0x40   /* Co=0, D/C=1 → data */

/* SSD1306 commands */
#define SSD1306_DISPLAYOFF       0xAE
#define SSD1306_DISPLAYON        0xAF
#define SSD1306_SETCONTRAST      0x81
#define SSD1306_NORMALDISPLAY    0xA6
#define SSD1306_INVERTDISPLAY    0xA7
#define SSD1306_DISPLAYALLON     0xA5
#define SSD1306_DISPLAYALLON_R   0xA4
#define SSD1306_SETREMAP         0xA0
#define SSD1306_SETMULTIPLEX     0xA8
#define SSD1306_SETOFFSET        0xD3
#define SSD1306_SETSTARTLINE     0x40
#define SSD1306_SETCLKDIV        0xD5
#define SSD1306_SETPRECHARGE     0xD9
#define SSD1306_SETCOMPINS       0xDA
#define SSD1306_SETVCOMDETECT    0xDB
#define SSD1306_CHARGEPUMP       0x8D
#define SSD1306_MEMORYMODE       0x20
#define SSD1306_SETCOLADDR       0x21
#define SSD1306_SETPAGEADDR      0x22
#define SSD1306_SEGREMAP_127     0xA1
#define SSD1306_COMSCANDEC       0xC8

/* ── Display buffer (128x64 = 1024 bytes, 8 pages) ────────────── */
#define OLED_WIDTH    128
#define OLED_HEIGHT   64
#define OLED_PAGES    8

static uint8_t oled_buf[OLED_WIDTH * OLED_PAGES];
static int oled_dirty = 1;

/* ── Send command to SSD1306 ──────────────────────────────────── */
static void ssd1306_cmd(uint8_t cmd)
{
    i2c_start();
    i2c_write_byte(SSD1306_ADDR << 1);  /* write mode */
    i2c_write_byte(SSD1306_CMD);
    i2c_write_byte(cmd);
    i2c_stop();
}

static void ssd1306_cmd2(uint8_t cmd, uint8_t arg)
{
    ssd1306_cmd(cmd);
    ssd1306_cmd(arg);
}

/* ── Send data to SSD1306 ─────────────────────────────────────── */
static void ssd1306_data(const uint8_t *data, int len)
{
    i2c_start();
    i2c_write_byte(SSD1306_ADDR << 1);
    i2c_write_byte(SSD1306_DATA);
    for (int i = 0; i < len; i++) {
        i2c_write_byte(data[i]);
    }
    i2c_stop();
}

/* ── 5x7 font table (printable ASCII subset) ─────────────────── */
static const uint8_t font5x7[][5] = {
    [0x20] = {0x00,0x00,0x00,0x00,0x00},  /* space */
    [0x21] = {0x00,0x00,0x5F,0x00,0x00},  /* ! */
    [0x2D] = {0x00,0x20,0x20,0x20,0x00},  /* - */
    [0x2E] = {0x00,0x00,0x00,0x00,0x00},  /* . (modified) */
    [0x2F] = {0x20,0x10,0x08,0x04,0x02},  /* / */
    [0x30] = {0x3E,0x51,0x49,0x45,0x3E},  /* 0 */
    [0x31] = {0x00,0x42,0x7F,0x40,0x00},  /* 1 */
    [0x32] = {0x42,0x61,0x51,0x49,0x46},  /* 2 */
    [0x33] = {0x21,0x41,0x45,0x4B,0x31},  /* 3 */
    [0x34] = {0x18,0x14,0x12,0x7F,0x10},  /* 4 */
    [0x35] = {0x27,0x45,0x45,0x45,0x39},  /* 5 */
    [0x36] = {0x3C,0x4A,0x49,0x49,0x30},  /* 6 */
    [0x37] = {0x01,0x71,0x09,0x05,0x03},  /* 7 */
    [0x38] = {0x36,0x49,0x49,0x49,0x36},  /* 8 */
    [0x39] = {0x06,0x49,0x49,0x29,0x1E},  /* 9 */
    [0x3A] = {0x00,0x36,0x36,0x00,0x00},  /* : */
    [0x3D] = {0x08,0x08,0x3E,0x08,0x08},  /* = */
    [0x3F] = {0x00,0x41,0x45,0x43,0x00},  /* ? */
    [0x41] = {0x7E,0x11,0x11,0x11,0x7E},  /* A */
    [0x42] = {0x7F,0x49,0x49,0x49,0x36},  /* B */
    [0x43] = {0x3E,0x41,0x41,0x41,0x22},  /* C */
    [0x44] = {0x7F,0x41,0x41,0x22,0x1C},  /* D */
    [0x45] = {0x7F,0x49,0x49,0x49,0x41},  /* E */
    [0x46] = {0x7F,0x09,0x09,0x09,0x01},  /* F */
    [0x47] = {0x3E,0x41,0x49,0x49,0x7A},  /* G */
    [0x48] = {0x7F,0x08,0x08,0x08,0x7F},  /* H */
    [0x49] = {0x00,0x41,0x7F,0x41,0x00},  /* I */
    [0x4A] = {0x20,0x40,0x41,0x3F,0x01},  /* J */
    [0x4B] = {0x7F,0x08,0x14,0x22,0x41},  /* K */
    [0x4C] = {0x7F,0x40,0x40,0x40,0x40},  /* L */
    [0x4D] = {0x7F,0x02,0x0C,0x02,0x7F},  /* M */
    [0x4E] = {0x7F,0x04,0x08,0x10,0x7F},  /* N */
    [0x4F] = {0x3E,0x41,0x41,0x41,0x3E},  /* O */
    [0x50] = {0x7F,0x09,0x09,0x09,0x06},  /* P */
    [0x51] = {0x3E,0x41,0x51,0x21,0x5E},  /* Q */
    [0x52] = {0x7F,0x09,0x19,0x29,0x46},  /* R */
    [0x53] = {0x46,0x49,0x49,0x49,0x31},  /* S */
    [0x54] = {0x01,0x01,0x7F,0x01,0x01},  /* T */
    [0x55] = {0x3F,0x40,0x40,0x40,0x3F},  /* U */
    [0x56] = {0x1F,0x20,0x40,0x20,0x1F},  /* V */
    [0x57] = {0x3F,0x40,0x38,0x40,0x3F},  /* W */
    [0x58] = {0x63,0x14,0x08,0x14,0x63},  /* X */
    [0x59] = {0x07,0x08,0x70,0x08,0x07},  /* Y */
    [0x5A] = {0x61,0x51,0x49,0x45,0x43},  /* Z */
    [0x5F] = {0x40,0x40,0x40,0x40,0x40},  /* _ */
    [0x61] = {0x20,0x54,0x54,0x54,0x78},  /* a (small) */
    [0x62] = {0x7F,0x48,0x44,0x44,0x38},  /* b */
    [0x63] = {0x38,0x44,0x44,0x44,0x20},  /* c */
    [0x64] = {0x38,0x44,0x44,0x48,0x7F},  /* d */
    [0x65] = {0x38,0x54,0x54,0x54,0x18},  /* e */
    [0x66] = {0x08,0x7E,0x09,0x01,0x02},  /* f */
    [0x67] = {0x18,0xA4,0xA4,0xA4,0x7C},  /* g */
    [0x68] = {0x7F,0x08,0x04,0x04,0x78},  /* h */
    [0x69] = {0x00,0x44,0x7D,0x40,0x00},  /* i */
    [0x6A] = {0x40,0x80,0x84,0x7D,0x00},  /* j */
    [0x6B] = {0x7F,0x10,0x28,0x44,0x00},  /* k */
    [0x6C] = {0x00,0x41,0x7F,0x40,0x00},  /* l */
    [0x6D] = {0x7C,0x04,0x18,0x04,0x78},  /* m */
    [0x6E] = {0x7C,0x08,0x04,0x04,0x78},  /* n */
    [0x6F] = {0x38,0x44,0x44,0x44,0x38},  /* o */
    [0x70] = {0x7C,0x14,0x14,0x14,0x08},  /* p */
    [0x71] = {0x08,0x14,0x14,0x18,0x7C},  /* q */
    [0x72] = {0x7C,0x08,0x04,0x04,0x08},  /* r */
    [0x73] = {0x48,0x54,0x54,0x54,0x20},  /* s */
    [0x74] = {0x04,0x3F,0x44,0x40,0x20},  /* t */
    [0x75] = {0x3C,0x40,0x40,0x20,0x7C},  /* u */
    [0x76] = {0x1C,0x20,0x40,0x20,0x1C},  /* v */
    [0x77] = {0x3C,0x40,0x30,0x40,0x3C},  /* w */
    [0x78] = {0x44,0x28,0x10,0x28,0x44},  /* x */
    [0x79] = {0x1C,0xA0,0xA0,0xA0,0x7C},  /* y */
    [0x7A] = {0x44,0x64,0x54,0x4C,0x44},  /* z */
};

/* ── Draw a character at (x, page*8) ──────────────────────────── */
static void oled_draw_char(int x, int page, char c)
{
    if (c < 0x20 || c > 0x7A)
        c = '?';
    if (x < 0 || x + 5 > OLED_WIDTH) return;
    if (page < 0 || page >= OLED_PAGES) return;

    const uint8_t *glyph = font5x7[(int)c];
    for (int i = 0; i < 5; i++) {
        oled_buf[page * OLED_WIDTH + x + i] = glyph[i];
    }
    /* 1-pixel space after character */
    if (x + 5 < OLED_WIDTH)
        oled_buf[page * OLED_WIDTH + x + 5] = 0x00;

    oled_dirty = 1;
}

/* ── Public API ───────────────────────────────────────────────── */

void oled_init(void)
{
    /* Wait for display to be ready after power-on */
    board_delay_ms(50);

    /* Initialize SSD1306 */
    ssd1306_cmd(SSD1306_DISPLAYOFF);
    ssd1306_cmd2(SSD1306_SETCONTRAST, 0x7F);
    ssd1306_cmd2(SSD1306_SETMULTIPLEX, 0x3F);
    ssd1306_cmd2(SSD1306_SETOFFSET, 0x00);
    ssd1306_cmd(SSD1306_SETSTARTLINE | 0x00);
    ssd1306_cmd(SSD1306_SEGREMAP_127);   /* column address 127 mapped to SEG0 */
    ssd1306_cmd(SSD1306_COMSCANDEC);      /* COM scan direction remapped */
    ssd1306_cmd2(SSD1306_SETCLKDIV, 0x80);
    ssd1306_cmd2(SSD1306_SETPRECHARGE, 0x22);
    ssd1306_cmd2(SSD1306_SETCOMPINS, 0x12);
    ssd1306_cmd2(SSD1306_SETVCOMDETECT, 0x40);
    ssd1306_cmd2(SSD1306_MEMORYMODE, 0x00);  /* horizontal addressing */
    ssd1306_cmd2(SSD1306_CHARGEPUMP, 0x14);
    ssd1306_cmd(SSD1306_DISPLAYALLON_R);
    ssd1306_cmd(SSD1306_NORMALDISPLAY);
    ssd1306_cmd(SSD1306_DISPLAYON);

    oled_clear();
    oled_refresh();
}

void oled_clear(void)
{
    for (int i = 0; i < OLED_WIDTH * OLED_PAGES; i++) {
        oled_buf[i] = 0x00;
    }
    oled_dirty = 1;
}

void oled_draw_string(int x, int y, const char *str, int size)
{
    int page = y / 8;
    int cx = x;

    while (*str && cx < OLED_WIDTH) {
        char c = *str++;

        if (size == 2) {
            /* Large font: render at 2x scale (simplified — just wider spacing) */
            oled_draw_char(cx, page, c);
            oled_draw_char(cx + 1, page + 1, c);  /* duplicate on next page for height */
            cx += 7;
        } else {
            oled_draw_char(cx, page, c);
            cx += 6;
        }
    }
}

void oled_draw_status(int armed, int battery, const char *profile)
{
    oled_clear();

    /* Title */
    oled_draw_string(0, 0, "EDDY-PHANTOM", 2);

    /* Profile name */
    if (profile && profile[0]) {
        oled_draw_string(0, 16, profile, 1);
    } else {
        oled_draw_string(0, 16, "no profile", 1);
    }

    /* State */
    if (armed) {
        oled_draw_string(0, 28, "ARMED", 1);
    } else {
        oled_draw_string(0, 28, "IDLE", 1);
    }

    /* Battery indicator */
    if (battery > 0) {
        char batt_str[16];
        /* Simple itoa */
        int idx = 0;
        if (battery >= 100) {
            batt_str[idx++] = '1';
            batt_str[idx++] = '0';
            batt_str[idx++] = '0';
        } else {
            if (battery >= 10) batt_str[idx++] = '0' + (battery / 10);
            batt_str[idx++] = '0' + (battery % 10);
        }
        batt_str[idx++] = '%';
        batt_str[idx++] = '\0';
        oled_draw_string(100, 0, batt_str, 1);
    }

    oled_dirty = 1;
}

void oled_refresh(void)
{
    if (!oled_dirty) return;

    /* Set column address range: 0 to 127 */
    ssd1306_cmd(SSD1306_SETCOLADDR);
    ssd1306_cmd(0);
    ssd1306_cmd(OLED_WIDTH - 1);

    /* Set page address range: 0 to 7 */
    ssd1306_cmd(SSD1306_SETPAGEADDR);
    ssd1306_cmd(0);
    ssd1306_cmd(OLED_PAGES - 1);

    /* Send display buffer in chunks (I2C has max ~32 bytes per write) */
    int offset = 0;
    while (offset < OLED_WIDTH * OLED_PAGES) {
        int chunk = OLED_WIDTH * OLED_PAGES - offset;
        if (chunk > 16) chunk = 16;
        ssd1306_data(&oled_buf[offset], chunk);
        offset += chunk;
    }

    oled_dirty = 0;
}

/* ── MAX17048 battery fuel gauge ──────────────────────────────── */
uint8_t i2c_read_battery(void)
{
    /* MAX17048 I2C address: 0x36 (7-bit) → 0x6C (8-bit write) */
    /* Register 0x04 (SOC) returns state of charge as a percentage
     * in units of 1/256% (i.e., value * 100 / 256 ≈ percentage) */

    i2c_start();
    if (i2c_write_byte(0x6C) != 0) {  /* write address */
        i2c_stop();
        return 0;
    }
    i2c_write_byte(0x04);  /* register: SOC */
    i2c_stop();

    i2c_start();
    i2c_write_byte(0x6D);  /* read address */
    uint8_t hi = i2c_read_byte(1);  /* ACK for first byte */
    uint8_t lo = i2c_read_byte(0);  /* NACK for last byte */
    i2c_stop();

    /* SOC is stored as 16-bit, upper byte is integer percent */
    (void)lo;
    if (hi > 100) hi = 100;
    return hi;
}