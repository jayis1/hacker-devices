/*
 * drivers/oled.c — OLED Status Display for Prism-Tap
 *
 * SSD1306 OLED driver over I2C3 (PC10/PC11). 128x64 monochrome display.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "oled.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- Display buffer (128x64 = 1024 pixels = 128 bytes) ---- */
static uint8_t display_buffer[OLED_HEIGHT / 8][OLED_WIDTH];
static uint8_t needs_update = 0;

/* ---- I2C3 helpers (similar to bridge I2C but on I2C3) ---- */

#define I2C3_TIMING_400K  0x20B15A55U

static void oled_i2c_write_cmd(uint8_t cmd)
{
    /* Control byte: 0x80 (Co=1, D/C=0) then command byte */
    uint8_t addr7 = (OLED_I2C_ADDR << 1);
    uint32_t timeout;

    while ((I2C_ISR(I2C3_BASE) & I2C_ISR_BUSY))
        ;

    /* Write 2 bytes: 0x00 (Co=0, D/C=0) + cmd */
    I2C_CR2(I2C3_BASE) = ((uint32_t)addr7) | ((uint32_t)2 << 16) | I2C_CR2_START;

    timeout = 10000;
    while (!(I2C_ISR(I2C3_BASE) & I2C_ISR_TXE) && timeout--)
        ;
    I2C_TXDR(I2C3_BASE) = 0x00;  /* Co=0, D/C#=0 (command) */

    timeout = 10000;
    while (!(I2C_ISR(I2C3_BASE) & I2C_ISR_TXE) && timeout--)
        ;
    I2C_TXDR(I2C3_BASE) = cmd;

    timeout = 10000;
    while (!(I2C_ISR(I2C3_BASE) & I2C_ISR_TC) && timeout--)
        ;
    I2C_CR2(I2C3_BASE) |= I2C_CR2_STOP;
}

static void oled_i2c_write_data(const uint8_t *data, uint32_t len)
{
    uint8_t addr7 = (OLED_I2C_ADDR << 1) | 0;  /* write mode */
    uint32_t timeout;

    while ((I2C_ISR(I2C3_BASE) & I2C_ISR_BUSY))
        ;

    /* Write len+1 bytes: 0x40 (Co=0, D/C=1) + data */
    I2C_CR2(I2C3_BASE) = ((uint32_t)addr7) | ((uint32_t)(len + 1) << 16) |
                         I2C_CR2_START;

    timeout = 10000;
    while (!(I2C_ISR(I2C3_BASE) & I2C_ISR_TXE) && timeout--)
        ;
    I2C_TXDR(I2C3_BASE) = 0x40;  /* Co=0, D/C#=1 (data) */

    for (uint32_t i = 0; i < len; i++) {
        timeout = 10000;
        while (!(I2C_ISR(I2C3_BASE) & I2C_ISR_TXE) && timeout--)
            ;
        I2C_TXDR(I2C3_BASE) = data[i];
    }

    timeout = 10000;
    while (!(I2C_ISR(I2C3_BASE) & I2C_ISR_TC) && timeout--)
        ;
    I2C_CR2(I2C3_BASE) |= I2C_CR2_STOP;
}

/* ---- SSD1306 init sequence ---- */

int oled_init(void)
{
    /* Enable GPIOC and I2C3 clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
    RCC_APB1LENR |= RCC_APB1LENR_I2C3EN;

    /* Configure PC10 (SCL) and PC11 (SDA) as AF4 (I2C3) */
    uint32_t moder = GPIO_REG(GPIOC_BASE, GPIO_MODER_OFFSET);
    moder &= ~(3U << (OLED_I2C_PIN_SCL * 2));
    moder &= ~(3U << (OLED_I2C_PIN_SDA * 2));
    moder |= (GPIO_MODE_AF << (OLED_I2C_PIN_SCL * 2));
    moder |= (GPIO_MODE_AF << (OLED_I2C_PIN_SDA * 2));
    GPIO_REG(GPIOC_BASE, GPIO_MODER_OFFSET) = moder;

    /* AF4 for I2C3 (PC10=AFRL[10], PC11=AFRL[11]) */
    uint32_t afrl = GPIO_REG(GPIOC_BASE, GPIO_AFRL_OFFSET);
    afrl &= ~(0xFU << (OLED_I2C_PIN_SCL * 4));
    afrl |= (AF_I2C3_SCL_PC10 << (OLED_I2C_PIN_SCL * 4));
    afrl &= ~(0xFU << (OLED_I2C_PIN_SDA * 4));
    afrl |= (AF_I2C3_SDA_PC11 << (OLED_I2C_PIN_SDA * 4));
    GPIO_REG(GPIOC_BASE, GPIO_AFRL_OFFSET) = afrl;

    /* Open-drain */
    uint32_t otyper = GPIO_REG(GPIOC_BASE, GPIO_OTYPER_OFFSET);
    otyper |= (GPIO_OTYPE_OD << OLED_I2C_PIN_SCL);
    otyper |= (GPIO_OTYPE_OD << OLED_I2C_PIN_SDA);
    GPIO_REG(GPIOC_BASE, GPIO_OTYPER_OFFSET) = otyper;

    /* Pull-up */
    uint32_t pupdr = GPIO_REG(GPIOC_BASE, GPIO_PUPDR_OFFSET);
    pupdr &= ~(3U << (OLED_I2C_PIN_SCL * 2));
    pupdr |= (GPIO_PUPD_UP << (OLED_I2C_PIN_SCL * 2));
    pupdr &= ~(3U << (OLED_I2C_PIN_SDA * 2));
    pupdr |= (GPIO_PUPD_UP << (OLED_I2C_PIN_SDA * 2));
    GPIO_REG(GPIOC_BASE, GPIO_PUPDR_OFFSET) = pupdr;

    /* Configure I2C3 */
    I2C_CR1(I2C3_BASE) = 0;
    I2C_TIMING(I2C3_BASE) = I2C3_TIMING_400K;
    I2C_CR1(I2C3_BASE) = I2C_CR1_PE;

    /* SSD1306 init sequence */
    oled_i2c_write_cmd(0xAE);  /* Display off */
    oled_i2c_write_cmd(0xD5);  /* Set display clock divide */
    oled_i2c_write_cmd(0x80);
    oled_i2c_write_cmd(0xA8);  /* Set multiplex ratio */
    oled_i2c_write_cmd(0x3F);  /* 1/64 duty */
    oled_i2c_write_cmd(0xD3);  /* Set display offset */
    oled_i2c_write_cmd(0x00);
    oled_i2c_write_cmd(0x40);  /* Set start line */
    oled_i2c_write_cmd(0x8D);  /* Charge pump */
    oled_i2c_write_cmd(0x14);  /* Enable */
    oled_i2c_write_cmd(0x20);  /* Memory addressing mode */
    oled_i2c_write_cmd(0x00);  /* Horizontal */
    oled_i2c_write_cmd(0xA1);  /* Segment remap */
    oled_i2c_write_cmd(0xC8);  /* COM scan direction */
    oled_i2c_write_cmd(0xDA);  /* COM pins config */
    oled_i2c_write_cmd(0x12);
    oled_i2c_write_cmd(0x81);  /* Set contrast */
    oled_i2c_write_cmd(0xCF);
    oled_i2c_write_cmd(0xD9);  /* Set pre-charge period */
    oled_i2c_write_cmd(0xF1);
    oled_i2c_write_cmd(0xDB);  /* Set VCOMH deselect */
    oled_i2c_write_cmd(0x40);
    oled_i2c_write_cmd(0xA4);  /* Display follows RAM */
    oled_i2c_write_cmd(0xA6);  /* Normal display (not inverted) */
    oled_i2c_write_cmd(0xAF);  /* Display on */

    oled_clear();
    oled_update();

    return 0;
}

void oled_clear(void)
{
    memset(display_buffer, 0, sizeof(display_buffer));
    needs_update = 1;
}

void oled_set_pixel(uint8_t x, uint8_t y, uint8_t on)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return;
    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    if (on)
        display_buffer[page][x] |= (1U << bit);
    else
        display_buffer[page][x] &= ~(1U << bit);
    needs_update = 1;
}

/* ---- 5x7 font (ASCII 32-127) ---- */
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x24}, /* $ */
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
    {0x00,0x36,0x36,0x00,0x00}, /* : (approx) */
    /* ... more characters would be added for full font */
    {0x7C,0x12,0x11,0x12,0x7C}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x41,0x3E}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x01,0x01}, /* F */
    {0x3E,0x41,0x41,0x49,0x7A}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x04,0x02,0x7F}, /* M */
    {0x7F,0x02,0x04,0x08,0x7F}, /* N */
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
};

void oled_draw_text(uint8_t x, uint8_t y, const char *text)
{
    if (!text)
        return;

    uint8_t col = x;
    for (const char *p = text; *p && col < OLED_WIDTH - 5; p++) {
        uint8_t ch = (uint8_t)*p;
        uint8_t idx = 0;

        if (ch >= 'A' && ch <= 'Z')
            idx = ch - 'A' + 40;  /* offset to A in font table */
        else if (ch >= 'a' && ch <= 'z')
            idx = ch - 'a' + 40;  /* uppercase for now */
        else if (ch >= '0' && ch <= '9')
            idx = ch - '0' + 16;
        else if (ch == ' ')
            idx = 0;
        else if (ch == ':')
            idx = 37;
        else
            idx = 0;

        if (idx < (uint8_t)(sizeof(font5x7) / sizeof(font5x7[0]))) {
            for (int i = 0; i < 5; i++) {
                uint8_t col_data = font5x7[idx][i];
                for (int j = 0; j < 7; j++) {
                    if (col_data & (1U << j))
                        oled_set_pixel(col + i, y + j, 1);
                    else
                        oled_set_pixel(col + i, y + j, 0);
                }
            }
        }
        col += 6;  /* 5 pixels + 1 space */
    }
    needs_update = 1;
}

void oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    /* Bresenham's line algorithm */
    int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        oled_set_pixel(x1, y1, 1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
    needs_update = 1;
}

void oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    oled_draw_line(x, y, x + w - 1, y);           /* top    */
    oled_draw_line(x, y + h - 1, x + w - 1, y + h - 1); /* bottom */
    oled_draw_line(x, y, x, y + h - 1);           /* left   */
    oled_draw_line(x + w - 1, y, x + w - 1, y + h - 1); /* right */
}

void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for (uint8_t i = x; i < x + w && i < OLED_WIDTH; i++)
        for (uint8_t j = y; j < y + h && j < OLED_HEIGHT; j++)
            oled_set_pixel(i, j, 1);
    needs_update = 1;
}

void oled_update(void)
{
    if (!needs_update)
        return;

    /* Set column range */
    oled_i2c_write_cmd(0x21);
    oled_i2c_write_cmd(0x00);
    oled_i2c_write_cmd(0x7F);

    /* Set page range */
    oled_i2c_write_cmd(0x22);
    oled_i2c_write_cmd(0x00);
    oled_i2c_write_cmd(0x07);

    /* Send all display data */
    for (int page = 0; page < 8; page++) {
        oled_i2c_write_data(display_buffer[page], OLED_WIDTH);
    }

    needs_update = 0;
}

/* ---- Status display ---- */

static const char *mode_names[] = {
    "IDLE", "PASS", "CAPT", "INJ", "MITM", "LO-P", "DIAG",
};

void oled_draw_status(const char *mode, const char *link, uint8_t batt_pct,
                      uint32_t frames, uint8_t sd_present)
{
    oled_clear();

    /* Title */
    oled_draw_text(0, 0, "PRISM-TAP");

    /* Mode */
    oled_draw_text(0, 12, "MODE:");
    oled_draw_text(36, 12, mode ? mode : "----");

    /* Link status */
    oled_draw_text(0, 22, "LINK:");
    oled_draw_text(36, 22, link ? link : "----");

    /* Battery */
    char batt_str[16];
    batt_str[0] = 'B';
    batt_str[1] = 'A';
    batt_str[2] = 'T';
    batt_str[3] = ':';
    /* Convert batt_pct to string */
    uint8_t bp = batt_pct;
    batt_str[4] = (bp >= 100) ? '1' : (bp / 10) + '0';
    batt_str[5] = (bp >= 100) ? ((bp / 10) % 10) + '0' : (bp % 10) + '0';
    batt_str[6] = (bp >= 100) ? (bp % 10) + '0' : '%';
    batt_str[7] = (bp >= 100) ? '%' : '\0';
    if (bp >= 100) batt_str[8] = '\0';
    oled_draw_text(0, 32, batt_str);

    /* SD card */
    oled_draw_text(0, 42, "SD:");
    oled_draw_text(24, 42, sd_present ? "YES" : "NO");

    /* Frame count */
    char frame_str[16];
    frame_str[0] = 'F';
    frame_str[1] = 'R';
    frame_str[2] = 'M';
    frame_str[3] = ':';
    uint32_t f = frames;
    int fpos = 4;
    if (f == 0) {
        frame_str[fpos++] = '0';
    } else {
        char tmp[12];
        int tlen = 0;
        while (f > 0 && tlen < 11) {
            tmp[tlen++] = '0' + (f % 10);
            f /= 10;
        }
        for (int j = tlen - 1; j >= 0 && fpos < 15; j--)
            frame_str[fpos++] = tmp[j];
    }
    frame_str[fpos] = '\0';
    oled_draw_text(0, 54, frame_str);

    oled_update();
}

/* ---- End of oled.c ----
 * Author: jayis1
 */