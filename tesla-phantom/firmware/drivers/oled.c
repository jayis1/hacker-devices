/*
 * tesla-phantom — drivers/oled.c
 * SSD1306 OLED display driver (128×64, I2C1).
 * Provides status display for the TeslaPhantom device.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* SSD1306 I2C address: 0x3C (8-bit: 0x78) */
#define SSD1306_ADDR      0x3C
#define SSD1306_WIDTH     128
#define SSD1306_HEIGHT    64

/* Display buffer (128 × 64 / 8 = 1024 bytes) */
static uint8_t oled_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static uint8_t oled_initialized = 0;

/* ── I2C1 helpers ─────────────────────────────────────────────────── */
static void i2c_wait_idle(void) {
    while (I2C1->ISR & I2C_ISR_BUSY) { }
}

static int i2c_write(uint8_t addr, const uint8_t *data, uint16_t len) {
    i2c_wait_idle();

    /* Configure transfer: write, addr, nbytes, autoend */
    I2C1->CR2 = ((uint32_t)addr << 1)           /* 7-bit addr, write */
              | ((uint32_t)len << 16)           /* NBYTES */
              | I2C_CR2_AUTOEND;                /* auto-end */

    /* Start transfer */
    I2C1->CR2 |= I2C_CR2_START;

    for (uint16_t i = 0; i < len; i++) {
        /* Wait for TXIS (transmit buffer empty) */
        uint32_t timeout = millis();
        while (!(I2C1->ISR & I2C_ISR_TXIS)) {
            if (I2C1->ISR & I2C_ISR_NACKF) {
                I2C1->ICR = I2C_ISR_NACKF;
                return -1;
            }
            if ((millis() - timeout) > 100) return -1;
        }
        I2C1->TXDR = data[i];
    }

    return 0;
}

/* ── SSD1306 commands ─────────────────────────────────────────────── */
static void ssd1306_command(uint8_t cmd) {
    uint8_t buf[2] = { 0x00, cmd };  /* Co=0, D/C#=0 → command */
    i2c_write(SSD1306_ADDR, buf, 2);
}

static void ssd1306_data(const uint8_t *data, uint16_t len) {
    /* Send data in chunks (max 128 per I2C transaction) */
    uint16_t offset = 0;
    while (offset < len) {
        uint16_t chunk = (len - offset > 127) ? 127 : (len - offset);
        uint8_t buf[128];
        buf[0] = 0x40;  /* Co=0, D/C#=1 → data */
        for (uint16_t i = 0; i < chunk; i++)
            buf[1 + i] = data[offset + i];
        i2c_write(SSD1306_ADDR, buf, chunk + 1);
        offset += chunk;
    }
}

/* ── Font (5×7, minimal ASCII subset) ─────────────────────────────── */
static const uint8_t font5x7[][5] = {
    /* Space (0x20) */ {0x00, 0x00, 0x00, 0x00, 0x00},
    /* ! (0x21) */     {0x00, 0x00, 0x5F, 0x00, 0x00},
    /* " (0x22) */     {0x00, 0x07, 0x00, 0x07, 0x00},
    /* # (0x23) */     {0x14, 0x7F, 0x14, 0x7F, 0x14},
    /* ... abbreviated — full font in production ... */
    /* 0 (0x30) */     {0x3E, 0x51, 0x49, 0x45, 0x3E},
    /* 1 (0x31) */     {0x00, 0x42, 0x7F, 0x40, 0x00},
    /* 2 (0x32) */     {0x42, 0x61, 0x51, 0x49, 0x46},
    /* 3 (0x33) */     {0x21, 0x41, 0x45, 0x4B, 0x31},
    /* 4 (0x34) */     {0x18, 0x14, 0x12, 0x7F, 0x10},
    /* 5 (0x35) */     {0x27, 0x45, 0x45, 0x45, 0x39},
    /* 6 (0x36) */     {0x3C, 0x4A, 0x49, 0x49, 0x30},
    /* 7 (0x37) */     {0x01, 0x71, 0x09, 0x05, 0x03},
    /* 8 (0x38) */     {0x36, 0x49, 0x49, 0x49, 0x36},
    /* 9 (0x39) */     {0x06, 0x49, 0x49, 0x29, 0x1E},
    /* A (0x41) */     {0x7E, 0x11, 0x11, 0x11, 0x7E},
    /* B (0x42) */     {0x7F, 0x49, 0x49, 0x49, 0x36},
    /* C (0x43) */     {0x3E, 0x41, 0x41, 0x41, 0x22},
    /* D (0x44) */     {0x7F, 0x41, 0x41, 0x22, 0x1C},
    /* E (0x45) */     {0x7F, 0x49, 0x49, 0x49, 0x41},
    /* F (0x46) */     {0x7F, 0x09, 0x09, 0x09, 0x01},
    /* G (0x47) */     {0x3E, 0x41, 0x49, 0x49, 0x7A},
    /* H (0x48) */     {0x7F, 0x08, 0x08, 0x08, 0x7F},
    /* I (0x49) */     {0x00, 0x41, 0x7F, 0x41, 0x00},
    /* J (0x4A) */     {0x20, 0x40, 0x41, 0x3F, 0x01},
    /* K (0x4B) */     {0x7F, 0x08, 0x14, 0x22, 0x41},
    /* L (0x4C) */     {0x7F, 0x40, 0x40, 0x40, 0x40},
    /* M (0x4D) */     {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    /* N (0x4E) */     {0x7F, 0x04, 0x08, 0x10, 0x7F},
    /* O (0x4F) */     {0x3E, 0x41, 0x41, 0x41, 0x3E},
    /* P (0x50) */     {0x7F, 0x09, 0x09, 0x09, 0x06},
    /* Q (0x51) */     {0x3E, 0x41, 0x51, 0x21, 0x5E},
    /* R (0x52) */     {0x7F, 0x09, 0x19, 0x29, 0x46},
    /* S (0x53) */     {0x46, 0x49, 0x49, 0x49, 0x31},
    /* T (0x54) */     {0x01, 0x01, 0x7F, 0x01, 0x01},
    /* U (0x55) */     {0x3F, 0x40, 0x40, 0x40, 0x3F},
    /* V (0x56) */     {0x1F, 0x20, 0x40, 0x20, 0x1F},
    /* W (0x57) */     {0x3F, 0x40, 0x38, 0x40, 0x3F},
    /* X (0x58) */     {0x63, 0x14, 0x08, 0x14, 0x63},
    /* Y (0x59) */     {0x07, 0x08, 0x70, 0x08, 0x07},
    /* Z (0x5A) */     {0x61, 0x51, 0x49, 0x45, 0x43},
    /* - (0x2D) */     {0x08, 0x08, 0x08, 0x08, 0x08},
    /* . (0x2E) */     {0x00, 0x60, 0x60, 0x00, 0x00},
    /* : (0x3A) */     {0x00, 0x36, 0x36, 0x00, 0x00},
    /* ? (0x3F) */     {0x02, 0x01, 0x51, 0x09, 0x06},
};

static int get_font_index(char c) {
    if (c >= '0' && c <= '9') return 0x30 - 0x20 + (c - '0');
    if (c >= 'A' && c <= 'Z') return 0x41 - 0x20 + (c - 'A');
    if (c >= 'a' && c <= 'z') return 0x41 - 0x20 + (c - 'a'); /* uppercase */
    switch (c) {
        case ' ': return 0;
        case '!': return 1;
        case '"': return 2;
        case '#': return 3;
        case '-': return 0x2D - 0x20;
        case '.': return 0x2E - 0x20;
        case ':': return 0x3A - 0x20;
        case '?': return 0x3F - 0x20;
    }
    return 0;  /* default to space */
}

/* ── Buffer operations ────────────────────────────────────────────── */
static void set_pixel(int x, int y, uint8_t on) {
    if (x < 0 || x >= SSD1306_WIDTH) return;
    if (y < 0 || y >= SSD1306_HEIGHT) return;
    int idx = x + (y / 8) * SSD1306_WIDTH;
    if (on)
        oled_buffer[idx] |= (1 << (y & 7));
    else
        oled_buffer[idx] &= ~(1 << (y & 7));
}

/* ── Public API ───────────────────────────────────────────────────── */

void oled_init(void) {
    /* Configure I2C1 */
    I2C1->CR1 = 0;  /* disable */
    /* Timing: 400 kHz, APB1 = 120 MHz
     * PRESC=3, SCLL=19, SCLH=9, SDADEL=3, SCLDEL=3 → ~400 kHz */
    I2C1->TIMINGR = (3U << 28) | (3U << 20) | (3U << 16)
                  | (19U << 8) | (9U << 0);
    I2C1->CR1 = I2C_CR1_PE;  /* enable */

    delay_ms(10);

    /* SSD1306 initialization sequence */
    ssd1306_command(0xAE);  /* display off */
    ssd1306_command(0xD5); ssd1306_command(0x80);  /* set display clock */
    ssd1306_command(0xA8); ssd1306_command(0x3F);  /* set multiplex */
    ssd1306_command(0xD3); ssd1306_command(0x00);  /* set display offset */
    ssd1306_command(0x40);  /* set start line */
    ssd1306_command(0x8D); ssd1306_command(0x14);  /* enable charge pump */
    ssd1306_command(0x20); ssd1306_command(0x00);  /* horizontal addressing */
    ssd1306_command(0xA1);  /* segment remap */
    ssd1306_command(0xC8);  /* COM scan direction */
    ssd1306_command(0xDA); ssd1306_command(0x12);  /* set COM pins */
    ssd1306_command(0x81); ssd1306_command(0xCF);  /* set contrast */
    ssd1306_command(0xD9); ssd1306_command(0xF1);  /* set precharge */
    ssd1306_command(0xDB); ssd1306_command(0x40);  /* set VCOM detect */
    ssd1306_command(0xA4);  /* display resume */
    ssd1306_command(0xA6);  /* normal display (not inverted) */
    ssd1306_command(0xAF);  /* display on */

    oled_clear();
    oled_initialized = 1;
}

void oled_clear(void) {
    for (int i = 0; i < (int)sizeof(oled_buffer); i++)
        oled_buffer[i] = 0;
}

void oled_draw_string(int x, int y, const char *str, int size) {
    int cx = x;
    for (const char *p = str; *p; p++) {
        int fi = get_font_index(*p);
        for (int col = 0; col < 5; col++) {
            uint8_t line = font5x7[fi][col];
            for (int row = 0; row < 7; row++) {
                if (line & (1 << row)) {
                    if (size == 1) {
                        set_pixel(cx + col, y + row, 1);
                    } else {
                        /* 2× scaling */
                        for (int dx = 0; dx < size; dx++)
                            for (int dy = 0; dy < size; dy++)
                                set_pixel(cx + col*size + dx,
                                          y + row*size + dy, 1);
                    }
                }
            }
        }
        cx += (size == 1) ? 6 : 6 * size;
    }
}

void oled_draw_status(int armed, int battery, const char *mode_str) {
    oled_clear();
    oled_draw_string(0, 0, "TESLA-PHANTOM", 2);

    /* Mode */
    oled_draw_string(0, 20, "MODE:", 1);
    oled_draw_string(36, 20, mode_str, 1);

    /* Armed status */
    if (armed)
        oled_draw_string(90, 20, "ARM", 1);
    else
        oled_draw_string(90, 20, "IDLE", 1);

    /* Battery */
    oled_draw_string(0, 30, "BAT:", 1);
    char bat_str[8];
    int_to_str_local(bat_str, battery);
    oled_draw_string(24, 30, bat_str, 1);
    oled_draw_string(50, 30, "%", 1);
}

void oled_draw_position(float x, float y, float z, uint16_t hv) {
    char buf[16];

    /* X position */
    oled_draw_string(0, 40, "X:", 1);
    int_to_str_local(buf, (int)(x * 100));
    oled_draw_string(12, 40, buf, 1);

    /* Y position */
    oled_draw_string(45, 40, "Y:", 1);
    int_to_str_local(buf, (int)(y * 100));
    oled_draw_string(57, 40, buf, 1);

    /* HV voltage */
    oled_draw_string(0, 54, "HV:", 1);
    int_to_str_local(buf, hv);
    oled_draw_string(18, 54, buf, 1);
    oled_draw_string(45, 54, "V", 1);

    /* Z position */
    oled_draw_string(65, 54, "Z:", 1);
    int_to_str_local(buf, (int)(z * 100));
    oled_draw_string(77, 54, buf, 1);
}

/* Local int-to-string (avoids dependency on main.c's version) */
static int int_to_str_local(char *buf, int val) {
    if (val < 0) { *buf++ = '-'; val = -val; }
    char tmp[12];
    int i = 0;
    if (val == 0) tmp[i++] = '0';
    while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
    int n = 0;
    while (i > 0) buf[n++] = tmp[--i];
    buf[n] = 0;
    return n;
}

void oled_refresh(void) {
    if (!oled_initialized) return;

    /* Set column address range */
    ssd1306_command(0x21);
    ssd1306_command(0x00);
    ssd1306_command(SSD1306_WIDTH - 1);

    /* Set page address range */
    ssd1306_command(0x22);
    ssd1306_command(0x00);
    ssd1306_command((SSD1306_HEIGHT / 8) - 1);

    /* Send buffer data */
    ssd1306_data(oled_buffer, sizeof(oled_buffer));
}