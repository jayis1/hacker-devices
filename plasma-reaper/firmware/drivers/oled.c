/*
 * oled.c — SSD1306 OLED status display driver
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Drives a 0.96" SSD1306 OLED (128×64) over I2C1 to display device
 * status: current mode, shot count, success/failure counts, and
 * sweep progress.
 *
 * The SSD1306 has a 128×64 pixel display organized as 8 pages of
 * 128 columns each. Text rendering uses a simple 6×8 font.
 */

#include "oled.h"
#include "../registers.h"
#include "../board.h"

/* ---- Constants ----------------------------------------------------- */

#define OLED_I2C_ADDR       0x3C  /* 7-bit address, shifted left = 0x78 */
#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_PAGES          8
#define OLED_CHARS_PER_LINE 21    /* 128 / 6 */

/* ---- Private state ------------------------------------------------- */

static uint8_t g_cursor_row;
static uint8_t g_cursor_col;
static uint8_t g_framebuf[OLED_PAGES][OLED_WIDTH];

/* ---- Simple 6x8 font (printable ASCII subset) --------------------- */

/* Each character is 6 columns wide, 8 rows (1 page) tall.
 * We store only a minimal subset to save space. */
static const uint8_t font6x8[][6] = {
    /* ' ' */ {0x00,0x00,0x00,0x00,0x00,0x00},
    /* '!' */ {0x00,0x00,0x5F,0x00,0x00,0x00},
    /* '#' */ {0x7C,0x7C,0x7C,0x7C,0x7C,0x7C},
    /* '+' */ {0x00,0x04,0x07,0x04,0x00,0x00},
    /* '-' */ {0x00,0x04,0x04,0x04,0x00,0x00},
    /* '/' */ {0x20,0x10,0x08,0x04,0x02,0x00},
    /* '0' */ {0x3E,0x51,0x49,0x45,0x3E,0x00},
    /* '1' */ {0x00,0x42,0x7F,0x40,0x00,0x00},
    /* '2' */ {0x42,0x61,0x51,0x49,0x46,0x00},
    /* '3' */ {0x21,0x41,0x45,0x4B,0x31,0x00},
    /* '4' */ {0x18,0x14,0x12,0x7F,0x10,0x00},
    /* '5' */ {0x27,0x45,0x45,0x45,0x39,0x00},
    /* '6' */ {0x3C,0x4A,0x49,0x49,0x30,0x00},
    /* '7' */ {0x01,0x71,0x09,0x05,0x03,0x00},
    /* '8' */ {0x36,0x49,0x49,0x49,0x36,0x00},
    /* '9' */ {0x06,0x49,0x49,0x29,0x1E,0x00},
    /* ':' */ {0x00,0x36,0x36,0x00,0x00,0x00},
    /* 'A' */ {0x7E,0x11,0x11,0x11,0x7E,0x00},
    /* 'B' */ {0x7F,0x49,0x49,0x49,0x36,0x00},
    /* 'C' */ {0x3E,0x41,0x41,0x41,0x22,0x00},
    /* 'D' */ {0x7F,0x41,0x41,0x22,0x1C,0x00},
    /* 'E' */ {0x7F,0x49,0x49,0x49,0x41,0x00},
    /* 'F' */ {0x7F,0x09,0x09,0x09,0x01,0x00},
    /* 'G' */ {0x3E,0x41,0x49,0x49,0x7A,0x00},
    /* 'H' */ {0x7F,0x08,0x08,0x08,0x7F,0x00},
    /* 'I' */ {0x00,0x41,0x7F,0x41,0x00,0x00},
    /* 'J' */ {0x20,0x40,0x41,0x3F,0x01,0x00},
    /* 'K' */ {0x7F,0x08,0x14,0x22,0x41,0x00},
    /* 'L' */ {0x7F,0x40,0x40,0x40,0x40,0x00},
    /* 'M' */ {0x7F,0x02,0x0C,0x02,0x7F,0x00},
    /* 'N' */ {0x7F,0x04,0x08,0x10,0x7F,0x00},
    /* 'O' */ {0x3E,0x41,0x41,0x41,0x3E,0x00},
    /* 'P' */ {0x7F,0x09,0x09,0x09,0x06,0x00},
    /* 'R' */ {0x7F,0x09,0x19,0x29,0x46,0x00},
    /* 'S' */ {0x46,0x49,0x49,0x49,0x31,0x00},
    /* 'T' */ {0x01,0x01,0x7F,0x01,0x01,0x00},
    /* 'U' */ {0x3F,0x40,0x40,0x40,0x3F,0x00},
    /* 'V' */ {0x1F,0x20,0x40,0x20,0x1F,0x00},
    /* 'W' */ {0x3F,0x40,0x38,0x40,0x3F,0x00},
    /* 'X' */ {0x63,0x14,0x08,0x14,0x63,0x00},
    /* 'Y' */ {0x07,0x08,0x70,0x08,0x07,0x00},
    /* 'Z' */ {0x61,0x51,0x49,0x45,0x43,0x00},
    /* 'a' */ {0x20,0x54,0x54,0x54,0x78,0x00},
    /* 'b' */ {0x7F,0x48,0x44,0x44,0x38,0x00},
    /* 'c' */ {0x38,0x44,0x44,0x44,0x20,0x00},
    /* 'd' */ {0x38,0x44,0x44,0x48,0x7F,0x00},
    /* 'e' */ {0x38,0x54,0x54,0x54,0x18,0x00},
    /* 'f' */ {0x08,0x7E,0x09,0x01,0x02,0x00},
    /* 'g' */ {0x0C,0x52,0x52,0x52,0x3E,0x00},
    /* 'h' */ {0x7F,0x08,0x04,0x04,0x78,0x00},
    /* 'i' */ {0x00,0x44,0x7D,0x40,0x00,0x00},
    /* 'j' */ {0x20,0x40,0x44,0x3D,0x00,0x00},
    /* 'k' */ {0x7F,0x10,0x28,0x44,0x00,0x00},
    /* 'l' */ {0x41,0x7F,0x41,0x00,0x00,0x00},
    /* 'm' */ {0x7C,0x04,0x18,0x04,0x78,0x00},
    /* 'n' */ {0x7C,0x08,0x04,0x04,0x78,0x00},
    /* 'o' */ {0x38,0x44,0x44,0x44,0x38,0x00},
    /* 'p' */ {0x7C,0x14,0x14,0x14,0x08,0x00},
    /* 'r' */ {0x7C,0x08,0x04,0x04,0x08,0x00},
    /* 's' */ {0x48,0x54,0x54,0x54,0x20,0x00},
    /* 't' */ {0x04,0x7F,0x48,0x40,0x00,0x00},
    /* 'u' */ {0x7C,0x40,0x40,0x40,0x7C,0x00},
    /* 'v' */ {0x1C,0x20,0x40,0x20,0x1C,0x00},
    /* 'w' */ {0x3C,0x40,0x30,0x40,0x3C,0x00},
    /* 'x' */ {0x44,0x28,0x10,0x28,0x44,0x00},
    /* 'y' */ {0x0C,0x50,0x50,0x50,0x3C,0x00},
    /* 'z' */ {0x44,0x64,0x54,0x4C,0x44,0x00},
};

/* Character lookup: maps ASCII to font table index */
static int char_to_font_idx(char c)
{
    if (c == ' ') return 0;
    if (c == '!') return 1;
    if (c == '#') return 2;
    if (c == '+') return 3;
    if (c == '-') return 4;
    if (c == '/') return 5;
    if (c >= '0' && c <= '9') return 6 + (c - '0');
    if (c == ':') return 16;
    if (c >= 'A' && c <= 'Z') return 17 + (c - 'A');
    if (c >= 'a' && c <= 'z') return 43 + (c - 'a');
    return 0; /* default: space */
}

/* ---- I2C helpers --------------------------------------------------- */

static void i2c_start_write(uint8_t addr)
{
    I2C1->CR2 = ((uint32_t)addr << 1) | I2C_CR2_START;
}

static void i2c_send_byte(uint8_t byte)
{
    while (!(I2C1->ISR & I2C_ISR_TXE))
        ;
    I2C1->TXDR = byte;
}

static void i2c_stop(void)
{
    I2C1->CR2 |= I2C_CR2_STOP;
    while (!(I2C1->ISR & BIT(6))) /* wait TC */
        ;
}

static void oled_send_cmd(uint8_t cmd)
{
    i2c_start_write(OLED_I2C_ADDR);
    /* Set NBYTES = 2 (control byte + command) */
    I2C1->CR2 = ((uint32_t)OLED_I2C_ADDR << 1)
              | (2U << I2C_CR2_NBYTES_SHIFT)
              | I2C_CR2_START;
    i2c_send_byte(0x00); /* control byte: command */
    i2c_send_byte(cmd);
    i2c_stop();
}

/* ---- Initialization ------------------------------------------------ */

void oled_init(void)
{
    /* Configure I2C1 pins: PB8 (SCL), PB9 (SDA) as AF4 */
    gpio_config(GPIOB, I2C_SCL_PIN, GPIO_MODE_AF, GPIO_OSPEED_HIGH,
                GPIO_PUPD_PULLUP, 4);
    gpio_config(GPIOB, I2C_SDA_PIN, GPIO_MODE_AF, GPIO_OSPEED_HIGH,
                GPIO_PUPD_PULLUP, 4);

    /* Configure OLED reset pin (PE7) as output */
    gpio_config(GPIOE, OLED_RESET_PIN, GPIO_MODE_OUTPUT, GPIO_OSPEED_LOW,
                GPIO_PUPD_NONE, 0);

    /* Reset OLED (toggle reset pin) */
    GPIOE->BRR = BIT(OLED_RESET_PIN);
    board_delay_ms(10);
    GPIOE->BSRR = BIT(OLED_RESET_PIN);
    board_delay_ms(10);

    /* Configure I2C1: 400 kHz */
    I2C1->CR1 = 0;
    I2C1->TIMINGR = 0x10909CEC; /* 400 kHz at 138 MHz PCLK (approx) */
    I2C1->CR1 = I2C_CR1_PE;     /* enable peripheral */

    /* SSD1306 initialization sequence */
    oled_send_cmd(0xAE); /* display off */
    oled_send_cmd(0xD5); oled_send_cmd(0x80); /* set display clock */
    oled_send_cmd(0xA8); oled_send_cmd(0x3F); /* multiplex ratio 1/64 */
    oled_send_cmd(0xD3); oled_send_cmd(0x00); /* display offset */
    oled_send_cmd(0x40); /* set start line */
    oled_send_cmd(0x8D); oled_send_cmd(0x14); /* enable charge pump */
    oled_send_cmd(0x20); oled_send_cmd(0x00); /* horizontal addressing */
    oled_send_cmd(0xA1); /* segment remap */
    oled_send_cmd(0xC8); /* COM scan direction */
    oled_send_cmd(0xDA); oled_send_cmd(0x12); /* COM pins */
    oled_send_cmd(0x81); oled_send_cmd(0xCF); /* contrast */
    oled_send_cmd(0xD9); oled_send_cmd(0xF1); /* precharge */
    oled_send_cmd(0xDB); oled_send_cmd(0x40); /* VCOM deselect */
    oled_send_cmd(0xA4); /* display follows RAM */
    oled_send_cmd(0xA6); /* normal display (not inverted) */
    oled_send_cmd(0xAF); /* display ON */

    g_cursor_row = 0;
    g_cursor_col = 0;

    /* Clear framebuffer */
    for (int p = 0; p < OLED_PAGES; p++)
        for (int c = 0; c < OLED_WIDTH; c++)
            g_framebuf[p][c] = 0;

    oled_clear();
    oled_refresh();
}

/* ---- Clear / Refresh ----------------------------------------------- */

void oled_clear(void)
{
    for (int p = 0; p < OLED_PAGES; p++)
        for (int c = 0; c < OLED_WIDTH; c++)
            g_framebuf[p][c] = 0;
    g_cursor_row = 0;
    g_cursor_col = 0;
}

void oled_refresh(void)
{
    /* Send entire framebuffer to OLED via I2C */
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        /* Set page address */
        oled_send_cmd(0xB0 + page);
        /* Set column address (low, high) */
        oled_send_cmd(0x00); /* low nibble = 0 */
        oled_send_cmd(0x10); /* high nibble = 0 */

        /* Send 128 bytes of page data */
        I2C1->CR2 = ((uint32_t)OLED_I2C_ADDR << 1)
                  | ((OLED_WIDTH + 1) << I2C_CR2_NBYTES_SHIFT)
                  | I2C_CR2_START;
        i2c_send_byte(0x40); /* control byte: data */
        for (int c = 0; c < OLED_WIDTH; c++)
            i2c_send_byte(g_framebuf[page][c]);
        i2c_stop();
    }
}

/* ---- Text output --------------------------------------------------- */

void oled_set_cursor(uint8_t row, uint8_t col)
{
    g_cursor_row = row;
    g_cursor_col = col;
}

void oled_print(const char *str)
{
    while (*str) {
        if (g_cursor_col >= OLED_CHARS_PER_LINE) {
            g_cursor_col = 0;
            g_cursor_row++;
            if (g_cursor_row >= OLED_PAGES)
                g_cursor_row = 0;
        }

        int idx = char_to_font_idx(*str);
        int x = g_cursor_col * 6;
        int p = g_cursor_row;

        for (int i = 0; i < 6; i++) {
            if (x + i < OLED_WIDTH)
                g_framebuf[p][x + i] = font6x8[idx][i];
        }

        g_cursor_col++;
        str++;
    }
}

void oled_print_u32(uint32_t val)
{
    char tmp[11];
    int i = 0;
    if (val == 0) {
        oled_print("0");
        return;
    }
    while (val > 0) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (i > 0) {
        char s[2] = {tmp[--i], 0};
        oled_print(s);
    }
}

/* ---- End of file --------------------------------------------------- */