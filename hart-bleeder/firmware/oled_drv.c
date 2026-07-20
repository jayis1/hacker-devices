/*
 * hart-bleeder — oled_drv.c
 * SSD1306 128x64 OLED driver over I2C2 for status display on the
 * HART Fieldbus Covert In-Line Attack Implant.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"
#include "oled_drv.h"
#include "modem_drv.h"
#include <string.h>

static uint8_t s_fb[OLED_WIDTH * OLED_HEIGHT / 8];

/* ── I2C2 bit-banged (PB10/PB11) for portability ────────────── */
#define I2C_SCL_HI()  gpio_write(GPIO(B), 10, 1)
#define I2C_SCL_LO()  gpio_write(GPIO(B), 10, 0)
#define I2C_SDA_HI()  gpio_write(GPIO(B), 11, 1)
#define I2C_SDA_LO()  gpio_write(GPIO(B), 11, 0)
#define I2C_SDA_RD()  gpio_read(GPIO(B), 11)

static void i2c_halfclock(void) { system_delay_us(5); }

static void i2c_start(void) {
    I2C_SDA_HI(); I2C_SCL_HI(); i2c_halfclock();
    I2C_SDA_LO(); i2c_halfclock();
    I2C_SCL_LO(); i2c_halfclock();
}

static void i2c_stop(void) {
    I2C_SDA_LO(); I2C_SCL_HI(); i2c_halfclock();
    I2C_SDA_HI(); i2c_halfclock();
}

static int i2c_write_byte(uint8_t b) {
    for (int i = 0; i < 8; i++) {
        if (b & 0x80) I2C_SDA_HI(); else I2C_SDA_LO();
        b <<= 1;
        i2c_halfclock();
        I2C_SCL_HI(); i2c_halfclock();
        I2C_SCL_LO();
    }
    I2C_SDA_HI(); i2c_halfclock();
    I2C_SCL_HI(); i2c_halfclock();
    int ack = (I2C_SDA_RD() == 0) ? 1 : 0;
    I2C_SCL_LO();
    return ack;
}

static void oled_cmd(uint8_t c) {
    i2c_start();
    i2c_write_byte(OLED_I2C_ADDR << 1);
    i2c_write_byte(0x00);   /* Co=0, D/C#=0 (command) */
    i2c_write_byte(c);
    i2c_stop();
}

static void oled_data(uint8_t d) {
    i2c_start();
    i2c_write_byte(OLED_I2C_ADDR << 1);
    i2c_write_byte(0x40);   /* D/C#=1 (data) */
    i2c_write_byte(d);
    i2c_stop();
}

/* ── 5x7 font (printable ASCII subset) ──────────────────────── */
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00}, /* " */
    /* ... abbreviated font; full set omitted for brevity */
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
    {0x7C,0x12,0x11,0x12,0x7C}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x09,0x01}, /* F */
    {0x3E,0x41,0x49,0x49,0x7A}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */
    {0x3F,0x40,0x38,0x40,0x3F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x07,0x08,0x70,0x08,0x07}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
    {0x00,0x00,0x00,0x00,0x00}, /* space (index 27) */
};

static int char_index(char c) {
    if (c == ' ') return 27;
    if (c >= '0' && c <= '9') return 3 + (c - '0');
    if (c >= 'A' && c <= 'Z') return 13 + (c - 'A');
    if (c >= 'a' && c <= 'z') return 13 + (c - 'a'); /* upper-case */
    return 27;
}

/* ── Init ────────────────────────────────────────────────────── */
int oled_init(void) {
    /* Configure PB10 (SCL), PB11 (SDA) as open-drain outputs */
    gpio_set_mode(GPIO(B), 10, GPIO_MODE_OUTPUT);
    gpio_set_mode(GPIO(B), 11, GPIO_MODE_OUTPUT);
    gpio_set_otyper(GPIO(B), 10, 1);
    gpio_set_otyper(GPIO(B), 11, 1);
    gpio_set_pupd(GPIO(B), 10, GPIO_PUPD_PU);
    gpio_set_pupd(GPIO(B), 11, GPIO_PUPD_PU);
    gpio_set_speed(GPIO(B), 10, GPIO_SPEED_HIGH);
    gpio_set_speed(GPIO(B), 11, GPIO_SPEED_HIGH);

    system_delay_ms(50);
    oled_cmd(0xAE);             /* display off */
    oled_cmd(0xD5); oled_cmd(0x80);   /* clock divide */
    oled_cmd(0xA8); oled_cmd(0x3F);   /* mux ratio 64 */
    oled_cmd(0xD3); oled_cmd(0x00);   /* display offset */
    oled_cmd(0x40);             /* start line 0 */
    oled_cmd(0x8D); oled_cmd(0x14);   /* charge pump on */
    oled_cmd(0x20); oled_cmd(0x00);   /* horizontal addressing */
    oled_cmd(0xA1);             /* segment remap */
    oled_cmd(0xC8);             /* COM scan direction */
    oled_cmd(0xDA); oled_cmd(0x12);   /* COM pins */
    oled_cmd(0x81); oled_cmd(0xCF);   /* contrast */
    oled_cmd(0xD9); oled_cmd(0xF1);   /* precharge */
    oled_cmd(0xDB); oled_cmd(0x40);   /* VCOMH deselect */
    oled_cmd(0xA4);             /* display RAM */
    oled_cmd(0xA6);             /* normal display */
    oled_cmd(0xAF);             /* display on */
    oled_clear();
    oled_refresh();
    return 0;
}

void oled_clear(void) {
    memset(s_fb, 0, sizeof(s_fb));
}

void oled_set_contrast(uint8_t val) {
    oled_cmd(0x81);
    oled_cmd(val);
}

void oled_draw_string(int x, int y, const char *s, int size) {
    while (*s && x < OLED_WIDTH - 5 * size) {
        int idx = char_index(*s);
        const uint8_t *glyph = font5x7[idx];
        for (int c = 0; c < 5; c++) {
            uint8_t col = glyph[c];
            for (int r = 0; r < 7; r++) {
                if (col & (1 << r)) {
                    for (int sx = 0; sx < size; sx++)
                        for (int sy = 0; sy < size; sy++) {
                            int px = x + c * size + sx;
                            int py = y + r * size + sy;
                            if (px < OLED_WIDTH && py < OLED_HEIGHT)
                                s_fb[(py / 8) * OLED_WIDTH + px] |= (1 << (py & 7));
                        }
                }
            }
        }
        x += 6 * size;
        s++;
    }
}

void oled_draw_status(uint8_t mode, uint8_t battery, uint8_t n_dev, float i_ma) {
    char line[20];
    oled_clear();
    oled_draw_string(0, 0, "HART-BLEEDER", 2);
    const char *m = "PASS";
    if (mode == 1) m = "INJ ";
    else if (mode == 2) m = "CLMP";
    else if (mode == 3) m = "SAG ";
    else if (mode == 4) m = "OPEN";
    else if (mode == 5) m = "COVT";
    oled_draw_string(0, 24, "MODE:", 1);
    oled_draw_string(36, 24, m, 1);
    oled_draw_string(0, 36, "BAT:", 1);
    /* battery as 0-9 digit */
    line[0] = '0' + (battery / 10 > 9 ? 9 : battery / 10);
    line[1] = 0;
    oled_draw_string(28, 36, line, 1);
    oled_draw_string(0, 48, "DEV:", 1);
    line[0] = '0' + (n_dev > 9 ? 9 : n_dev);
    line[1] = 0;
    oled_draw_string(28, 48, line, 1);
    /* Loop current — simplified 2-digit display */
    int i_int = (int)(i_ma * 100);
    line[0] = '0' + (i_int / 1000);
    line[1] = '.';
    line[2] = '0' + ((i_int / 100) % 10);
    line[3] = '0' + ((i_int / 10) % 10);
    line[4] = 'm';
    line[5] = 'A';
    line[6] = 0;
    oled_draw_string(60, 48, line, 1);
    oled_refresh();
}

void oled_refresh(void) {
    /* Set column and page address range then write framebuffer */
    oled_cmd(0x21); oled_cmd(0); oled_cmd(OLED_WIDTH - 1);
    oled_cmd(0x22); oled_cmd(0); oled_cmd((OLED_HEIGHT / 8) - 1);
    for (uint16_t i = 0; i < sizeof(s_fb); i++) oled_data(s_fb[i]);
}

void oled_off(void) { oled_cmd(0xAE); }
void oled_on(void)  { oled_cmd(0xAF); }