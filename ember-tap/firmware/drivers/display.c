/*
 * display.c — SSD1306 OLED (128×64, I2C2) driver + minimal UI
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * I2C2 is on PB10 (SCL) / PB11 (SDA), AF4.
 * SSD1306 7-bit address: 0x3C.
 */

#include "display.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- Framebuffer: 128 × 64 / 8 = 1024 bytes ---- */
static uint8_t fb[BOARD_OLED_W * (BOARD_OLED_H / 8)];
static int dirty = 1;

/* ---- I2C2 low-level (simplified — mirrors I2C1 pattern) ---- */
static void i2c2_init(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C2EN;
    for (volatile int i = 0; i < 100; i++);

    /* PB10/PB11 AF4 open-drain pull-up */
    GPIOB->MODER &= ~(0x3U << (I2C2_SCL_PIN * 2));
    GPIOB->MODER |=  (GPIO_MODE_AF << (I2C2_SCL_PIN * 2));
    GPIOB->AFRL  &= ~(0xFU << (I2C2_SCL_PIN * 4));
    GPIOB->AFRL  |=  (0x4U << (I2C2_SCL_PIN * 4));

    GPIOB->MODER &= ~(0x3U << (I2C2_SDA_PIN * 2));
    GPIOB->MODER |=  (GPIO_MODE_AF << (I2C2_SDA_PIN * 2));
    GPIOB->AFRL  &= ~(0xFU << (I2C2_SDA_PIN * 4));
    GPIOB->AFRL  |=  (0x4U << (I2C2_SDA_PIN * 4));

    GPIOB->OTYPER |= (1U << I2C2_SCL_PIN) | (1U << I2C2_SDA_PIN);
    GPIOB->PUPDR  |= (1U << (I2C2_SCL_PIN * 2)) | (1U << (I2C2_SDA_PIN * 2));

    I2C2->CR1 = 0;
    I2C2->TIMINGR = 0x10B17DB5U;  /* 400 kHz @ 170 MHz */
    I2C2->CR1 = I2C_CR1_PE | I2C_CR1_NACKIE | I2C_CR1_STOPIE;
}

static int i2c2_write_buf(const uint8_t *data, uint8_t len) {
    uint32_t to;
    to = 0xFFFFF;
    while ((I2C2->ISR & I2C_ISR_BUSY) && to--) ;

    I2C2->CR2 = ((SSD1306_I2C_ADDR & 0x7F) << 1)
              | I2C_CR2_NBYTES(len)
              | I2C_CR2_AUTOEND
              | I2C_CR2_START;

    for (uint8_t i = 0; i < len; i++) {
        to = 0xFFFFF;
        while (!(I2C2->ISR & I2C_ISR_TXIS) && to--) {
            if (I2C2->ISR & I2C_ISR_NACKF) {
                I2C2->ICR = I2C_ICR_NACKCF;
                return -1;
            }
        }
        if (!to) return -1;
        I2C2->TXDR = data[i];
    }
    to = 0xFFFFF;
    while (!(I2C2->ISR & I2C_ISR_STOPF) && to--) ;
    I2C2->ICR = I2C_ICR_STOPCF;
    return 0;
}

/* SSD1306 control byte: Co=0, D/C=0 for commands, D/C=1 for data.
 * We send a leading control byte then payload. */
static void ssd1306_cmd(uint8_t c) {
    uint8_t buf[2] = { 0x00, c };
    i2c2_write_buf(buf, 2);
}

static void ssd1306_data(const uint8_t *d, int len) {
    /* Send in chunks of 31 (control + 31 data = 32 byte max per I2C burst) */
    int off = 0;
    while (off < len) {
        int chunk = len - off;
        if (chunk > 31) chunk = 31;
        uint8_t buf[32];
        buf[0] = 0x40;  /* data stream */
        memcpy(&buf[1], &d[off], chunk);
        i2c2_write_buf(buf, chunk + 1);
        off += chunk;
    }
}

/* ---- Minimal 6×8 font (printable ASCII subset) ---- */
static const uint8_t font6x8[96][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x4F,0x00,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14,0x00}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, /* $ */
    {0x23,0x13,0x08,0x64,0x72,0x00}, /* % */
    {0x36,0x49,0x55,0x22,0x50,0x00}, /* & */
    {0x00,0x05,0x03,0x00,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00,0x00}, /* ( */
    {0x00,0x41,0x22,0x1C,0x00,0x00}, /* ) */
    {0x14,0x08,0x3E,0x08,0x14,0x00}, /* * */
    {0x08,0x08,0x3E,0x08,0x08,0x00}, /* + */
    {0x00,0x50,0x30,0x00,0x00,0x00}, /* , */
    {0x08,0x08,0x08,0x08,0x08,0x00}, /* - */
    {0x00,0x60,0x60,0x00,0x00,0x00}, /* . */
    {0x20,0x10,0x08,0x04,0x02,0x00}, /* / */
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46,0x00}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31,0x00}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10,0x00}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39,0x00}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30,0x00}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03,0x00}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36,0x00}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E,0x00}, /* 9 */
    {0x00,0x36,0x36,0x00,0x00,0x00}, /* : */
    {0x00,0x56,0x36,0x00,0x00,0x00}, /* ; */
    {0x00,0x08,0x14,0x22,0x41,0x00}, /* < */
    {0x14,0x14,0x14,0x14,0x14,0x00}, /* = */
    {0x41,0x22,0x14,0x08,0x00,0x00}, /* > */
    {0x02,0x01,0x51,0x09,0x06,0x00}, /* ? */
    {0x32,0x49,0x79,0x41,0x3E,0x00}, /* @ */
    {0x7E,0x11,0x11,0x11,0x7E,0x00}, /* A */
    {0x7F,0x49,0x49,0x49,0x36,0x00}, /* B */
    {0x3E,0x41,0x41,0x41,0x22,0x00}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C,0x00}, /* D */
    {0x7F,0x49,0x49,0x49,0x41,0x00}, /* E */
    {0x7F,0x09,0x09,0x01,0x01,0x00}, /* F */
    {0x3E,0x41,0x41,0x51,0x32,0x00}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, /* H */
    {0x00,0x41,0x7F,0x41,0x00,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01,0x00}, /* J */
    {0x7F,0x08,0x14,0x22,0x41,0x00}, /* K */
    {0x7F,0x40,0x40,0x40,0x40,0x00}, /* L */
    {0x7F,0x02,0x04,0x02,0x7F,0x00}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E,0x00}, /* O */
    {0x7F,0x11,0x11,0x11,0x0E,0x00}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E,0x00}, /* Q */
    {0x7F,0x11,0x19,0x29,0x46,0x00}, /* R */
    {0x46,0x49,0x49,0x49,0x31,0x00}, /* S */
    {0x01,0x01,0x7F,0x01,0x01,0x00}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F,0x00}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F,0x00}, /* V */
    {0x7F,0x20,0x18,0x20,0x7F,0x00}, /* W */
    {0x63,0x14,0x08,0x14,0x63,0x00}, /* X */
    {0x03,0x04,0x78,0x04,0x03,0x00}, /* Y */
    {0x61,0x51,0x49,0x45,0x43,0x00}, /* Z */
    {0x00,0x7F,0x41,0x41,0x00,0x00}, /* [ */
    {0x02,0x04,0x08,0x10,0x20,0x00}, /* backslash */
    {0x00,0x41,0x41,0x7F,0x00,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04,0x00}, /* ^ */
    {0x40,0x40,0x40,0x40,0x40,0x00}, /* _ */
    {0x00,0x01,0x02,0x04,0x00,0x00}, /* ` */
    {0x20,0x54,0x54,0x54,0x78,0x00}, /* a */
    {0x7F,0x48,0x44,0x44,0x38,0x00}, /* b */
    {0x38,0x44,0x44,0x44,0x20,0x00}, /* c */
    {0x38,0x44,0x44,0x48,0x7F,0x00}, /* d */
    {0x38,0x54,0x54,0x54,0x18,0x00}, /* e */
    {0x08,0x7E,0x09,0x01,0x02,0x00}, /* f */
    {0x08,0x14,0x54,0x54,0x3C,0x00}, /* g */
    {0x7F,0x08,0x04,0x04,0x78,0x00}, /* h */
    {0x00,0x44,0x7D,0x40,0x00,0x00}, /* i */
    {0x20,0x40,0x44,0x3D,0x00,0x00}, /* j */
    {0x00,0x7F,0x10,0x28,0x44,0x00}, /* k */
    {0x00,0x41,0x7F,0x40,0x00,0x00}, /* l */
    {0x7C,0x04,0x18,0x04,0x78,0x00}, /* m */
    {0x7C,0x08,0x04,0x04,0x78,0x00}, /* n */
    {0x38,0x44,0x44,0x44,0x38,0x00}, /* o */
    {0x7C,0x14,0x14,0x14,0x08,0x00}, /* p */
    {0x08,0x14,0x14,0x18,0x7C,0x00}, /* q */
    {0x7C,0x08,0x04,0x04,0x08,0x00}, /* r */
    {0x48,0x54,0x54,0x24,0x00,0x00}, /* s */
    {0x04,0x3F,0x44,0x40,0x20,0x00}, /* t */
    {0x3C,0x40,0x40,0x20,0x7C,0x00}, /* u */
    {0x1C,0x20,0x40,0x20,0x1C,0x00}, /* v */
    {0x3C,0x40,0x30,0x40,0x3C,0x00}, /* w */
    {0x44,0x28,0x10,0x28,0x44,0x00}, /* x */
    {0x0C,0x50,0x50,0x50,0x3C,0x00}, /* y */
    {0x44,0x64,0x54,0x4C,0x44,0x00}, /* z */
    {0x00,0x08,0x36,0x41,0x00,0x00}, /* { */
    {0x00,0x00,0x7F,0x00,0x00,0x00}, /* | */
    {0x00,0x41,0x36,0x08,0x00,0x00}, /* } */
    {0x02,0x01,0x02,0x04,0x02,0x00}, /* ~ */
};

static void draw_char(int x, int y, char c, int inv) {
    if (c < 32 || c > 127) c = ' ';
    const uint8_t *glyph = font6x8[c - 32];
    for (int col = 0; col < 6; col++) {
        uint8_t bits = glyph[col];
        if (inv) bits = ~bits;
        for (int row = 0; row < 8; row++) {
            if (bits & (1 << row)) {
                int px = x + col;
                int py = y + row;
                if (px < 0 || px >= BOARD_OLED_W || py < 0 || py >= BOARD_OLED_H)
                    continue;
                fb[px + (py / 8) * BOARD_OLED_W] |= (1 << (py % 8));
            } else if (inv) {
                int px = x + col;
                int py = y + row;
                if (px < 0 || px >= BOARD_OLED_W || py < 0 || py >= BOARD_OLED_H)
                    continue;
                fb[px + (py / 8) * BOARD_OLED_W] &= ~(1 << (py % 8));
            }
        }
    }
    dirty = 1;
}

void display_text(int col, int row, const char *s) {
    int x = col * 6;
    int y = row * 8;
    while (*s) { draw_char(x, y, *s, 0); x += 6; s++; }
}

void display_text_inv(int col, int row, const char *s) {
    int x = col * 6;
    int y = row * 8;
    while (*s) { draw_char(x, y, *s, 1); x += 6; s++; }
}

void display_bar(int x, int y, int w, int h, uint8_t pct) {
    int fill = (w * pct) / 255;
    for (int i = 0; i < w; i++) {
        int active = (i < fill) ? 1 : 0;
        for (int j = 0; j < h; j++) {
            int px = x + i, py = y + j;
            if (px < 0 || px >= BOARD_OLED_W || py < 0 || py >= BOARD_OLED_H) continue;
            if (active) fb[px + (py/8)*BOARD_OLED_W] |= (1 << (py%8));
            else        fb[px + (py/8)*BOARD_OLED_W] &= ~(1 << (py%8));
        }
    }
    dirty = 1;
}

void display_clear(void) {
    memset(fb, 0, sizeof(fb));
    dirty = 1;
}

void display_flush(void) {
    if (!dirty) return;
    /* Set column and page ranges then stream data */
    ssd1306_cmd(0x21); ssd1306_cmd(0); ssd1306_cmd(127);
    ssd1306_cmd(0x22); ssd1306_cmd(0); ssd1306_cmd(7);
    ssd1306_data(fb, sizeof(fb));
    dirty = 0;
}

void display_init(void) {
    i2c2_init();
    /* SSD1306 init sequence (128×64) */
    ssd1306_cmd(0xAE);             /* display off */
    ssd1306_cmd(0xD5); ssd1306_cmd(0x80);  /* clock divide */
    ssd1306_cmd(0xA8); ssd1306_cmd(0x3F);  /* multiplex 1/64 */
    ssd1306_cmd(0xD3); ssd1306_cmd(0x00);  /* display offset 0 */
    ssd1306_cmd(0x40);             /* start line 0 */
    ssd1306_cmd(0x8D); ssd1306_cmd(0x14);  /* charge pump on */
    ssd1306_cmd(0x20); ssd1306_cmd(0x00);  /* horizontal addressing */
    ssd1306_cmd(0xA1);             /* segment remap */
    ssd1306_cmd(0xC8);             /* COM scan dir remap */
    ssd1306_cmd(0xDA); ssd1306_cmd(0x12);  /* COM pins */
    ssd1306_cmd(0x81); ssd1306_cmd(0xCF);  /* contrast */
    ssd1306_cmd(0xD9); ssd1306_cmd(0xF1);  /* precharge */
    ssd1306_cmd(0xDB); ssd1306_cmd(0x40);  /* VCOMH */
    ssd1306_cmd(0xA4);             /* resume RAM display */
    ssd1306_cmd(0xA6);             /* normal (not inverted) */
    ssd1306_cmd(0xAF);             /* display ON */
    display_clear();
}

void display_render_status(uint8_t mode, uint32_t vbus_mv, uint32_t vbus_ma,
                           int8_t temp_c, uint32_t fuzz_sent,
                           uint32_t fuzz_crash) {
    static const char *modenames[MODE_COUNT] = {
        "PASSIVE", "SPOOF-SRC", "SINK-MASQ", "FUZZ", "GLITCH"
    };
    display_clear();
    display_text(0, 0, "EMBER-TAP  jayis1");
    display_text_inv(0, 1, modenames[mode % MODE_COUNT]);

    char line[22];
    /* VBUS line */
    int v = vbus_mv;
    line[0] = 'V'; line[1] = ':'; line[2] = ' ';
    int p = 3;
    if (v >= 10000) { line[p++] = '0' + (v/10000); v %= 10000; }
    if (v >= 1000)  { line[p++] = '0' + (v/1000);  v %= 1000;  }
    line[p++] = '.';
    line[p++] = '0' + (v/100);
    line[p++] = 'V'; line[p++] = ' '; line[p++] = ' ';
    line[p++] = 'I'; line[p++] = ':'; line[p++] = ' ';
    int a = vbus_ma / 100;
    if (a >= 100) { line[p++] = '0' + (a/100); a %= 100; }
    line[p++] = '0' + (a/10);
    line[p++] = '.';
    line[p++] = '0' + (vbus_ma % 10);
    line[p++] = 'A';
    line[p] = 0;
    display_text(0, 3, line);

    /* Temperature */
    line[0] = 'T'; line[1] = ':'; line[2] = ' ';
    p = 3;
    if (temp_c < 0) { line[p++] = '-'; temp_c = -temp_c; }
    line[p++] = '0' + (temp_c / 10);
    line[p++] = '0' + (temp_c % 10);
    line[p++] = 'C'; line[p] = 0;
    display_text(0, 4, line);

    /* Fuzz counters */
    if (mode == MODE_FUZZ) {
        line[0] = 'F'; line[1] = ':'; line[2] = ' ';
        p = 3;
        uint32_t fs = fuzz_sent;
        char tmp[11]; int tl = 0;
        if (fs == 0) tmp[tl++] = '0';
        while (fs) { tmp[tl++] = '0' + (fs % 10); fs /= 10; }
        while (tl) line[p++] = tmp[--tl];
        line[p++] = ' '; line[p++] = 'X'; line[p++] = ':';
        uint32_t fc = fuzz_crash;
        tl = 0;
        if (fc == 0) tmp[tl++] = '0';
        while (fc) { tmp[tl++] = '0' + (fc % 10); fc /= 10; }
        while (tl) line[p++] = tmp[--tl];
        line[p] = 0;
        display_text(0, 5, line);
    }

    display_text(0, 7, "UP/DN/SEL to cfg");
    display_flush();
}

/* end of file — author: jayis1 */