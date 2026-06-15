/*
 * ssd1306.c — SSD1306 128x32 OLED display driver (SPI2)
 * Connected via SPI2 on STM32H743
 */

#include "drivers/ssd1306.h"
#include "registers.h"
#include <string.h>

/* Framebuffer: 128 × 4 pages = 512 bytes */
static uint8_t g_display_buffer[SSD1306_WIDTH * SSD1306_PAGES];

/* 6×8 ASCII font (printable 0x20-0x7E = 95 chars) */
static const uint8_t font_6x8[][6] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  /* 0x20 ' ' */
    {0x00, 0x00, 0x5F, 0x00, 0x00, 0x00},  /* 0x21 '!' */
    {0x00, 0x07, 0x00, 0x07, 0x00, 0x00},  /* 0x22 '"' */
    {0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00},  /* 0x23 '#' */
    {0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00},  /* 0x24 '$' */
    {0x23, 0x13, 0x08, 0x64, 0x62, 0x00},  /* 0x25 '%' */
    {0x36, 0x49, 0x55, 0x22, 0x50, 0x00},  /* 0x26 '&' */
    {0x00, 0x05, 0x03, 0x00, 0x00, 0x00},  /* 0x27 ''' */
    {0x00, 0x1C, 0x22, 0x41, 0x00, 0x00},  /* 0x28 '(' */
    {0x00, 0x41, 0x22, 0x1C, 0x00, 0x00},  /* 0x29 ')' */
    {0x14, 0x08, 0x3E, 0x08, 0x14, 0x00},  /* 0x2A '*' */
    {0x08, 0x08, 0x3E, 0x08, 0x08, 0x00},  /* 0x2B '+' */
    {0x00, 0x50, 0x30, 0x00, 0x00, 0x00},  /* 0x2C ',' */
    {0x08, 0x08, 0x08, 0x08, 0x08, 0x00},  /* 0x2D '-' */
    {0x00, 0x60, 0x60, 0x00, 0x00, 0x00},  /* 0x2E '.' */
    {0x20, 0x10, 0x08, 0x04, 0x02, 0x00},  /* 0x2F '/' */
    {0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00},  /* 0x30 '0' */
    {0x00, 0x42, 0x7F, 0x40, 0x00, 0x00},  /* 0x31 '1' */
    {0x42, 0x61, 0x51, 0x49, 0x46, 0x00},  /* 0x32 '2' */
    {0x21, 0x41, 0x45, 0x4B, 0x31, 0x00},  /* 0x33 '3' */
    {0x18, 0x14, 0x12, 0x7F, 0x10, 0x00},  /* 0x34 '4' */
    {0x27, 0x45, 0x45, 0x45, 0x39, 0x00},  /* 0x35 '5' */
    {0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00},  /* 0x36 '6' */
    {0x01, 0x71, 0x09, 0x05, 0x03, 0x00},  /* 0x37 '7' */
    {0x36, 0x49, 0x49, 0x49, 0x36, 0x00},  /* 0x38 '8' */
    {0x06, 0x49, 0x49, 0x29, 0x1E, 0x00},  /* 0x39 '9' */
    {0x00, 0x36, 0x36, 0x00, 0x00, 0x00},  /* 0x3A ':' */
    {0x00, 0x56, 0x36, 0x00, 0x00, 0x00},  /* 0x3B ';' */
    {0x08, 0x14, 0x22, 0x41, 0x00, 0x00},  /* 0x3C '<' */
    {0x14, 0x14, 0x14, 0x14, 0x14, 0x00},  /* 0x3D '=' */
    {0x41, 0x22, 0x14, 0x08, 0x00, 0x00},  /* 0x3E '>' */
    {0x02, 0x01, 0x51, 0x09, 0x06, 0x00},  /* 0x3F '?' */
    {0x32, 0x49, 0x79, 0x41, 0x3E, 0x00},  /* 0x40 '@' */
    {0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00},  /* 0x41 'A' */
    {0x7F, 0x49, 0x49, 0x49, 0x36, 0x00},  /* 0x42 'B' */
    {0x3E, 0x41, 0x41, 0x41, 0x22, 0x00},  /* 0x43 'C' */
    {0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00},  /* 0x44 'D' */
    {0x7F, 0x49, 0x49, 0x49, 0x41, 0x00},  /* 0x45 'E' */
    {0x7F, 0x09, 0x09, 0x09, 0x01, 0x00},  /* 0x46 'F' */
    {0x3E, 0x41, 0x49, 0x49, 0x7A, 0x00},  /* 0x47 'G' */
    {0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00},  /* 0x48 'H' */
    {0x00, 0x41, 0x7F, 0x41, 0x00, 0x00},  /* 0x49 'I' */
    {0x20, 0x40, 0x41, 0x3F, 0x01, 0x00},  /* 0x4A 'J' */
    {0x7F, 0x08, 0x14, 0x22, 0x41, 0x00},  /* 0x4B 'K' */
    {0x7F, 0x40, 0x40, 0x40, 0x40, 0x00},  /* 0x4C 'L' */
    {0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00},  /* 0x4D 'M' */
    {0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00},  /* 0x4E 'N' */
    {0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00},  /* 0x4F 'O' */
    {0x7F, 0x09, 0x09, 0x09, 0x06, 0x00},  /* 0x50 'P' */
    {0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00},  /* 0x51 'Q' */
    {0x7F, 0x09, 0x19, 0x29, 0x46, 0x00},  /* 0x52 'R' */
    {0x46, 0x49, 0x49, 0x49, 0x32, 0x00},  /* 0x53 'S' */
    {0x01, 0x01, 0x7F, 0x01, 0x01, 0x00},  /* 0x54 'T' */
    {0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00},  /* 0x55 'U' */
    {0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00},  /* 0x56 'V' */
    {0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00},  /* 0x57 'W' */
    {0x63, 0x14, 0x08, 0x14, 0x63, 0x00},  /* 0x58 'X' */
    {0x07, 0x08, 0x70, 0x08, 0x07, 0x00},  /* 0x59 'Y' */
    {0x61, 0x51, 0x49, 0x45, 0x43, 0x00},  /* 0x5A 'Z' */
    {0x00, 0x7F, 0x41, 0x41, 0x00, 0x00},  /* 0x5B '[' */
    {0x02, 0x04, 0x08, 0x10, 0x20, 0x00},  /* 0x5C '\\' */
    {0x00, 0x41, 0x41, 0x7F, 0x00, 0x00},  /* 0x5D ']' */
    {0x04, 0x02, 0x01, 0x02, 0x04, 0x00},  /* 0x5E '^' */
    {0x40, 0x40, 0x40, 0x40, 0x40, 0x00},  /* 0x5F '_' */
    /* Remaining chars 0x60-0x7E omitted for brevity, filled with spaces */
};

/* ========================================
 * SPI2 transfer (8-bit, blocking)
 * ======================================== */

static void spi2_write(uint8_t data) {
    /* Wait for TXE */
    uint32_t timeout = 10000;
    while (!(SPI2->SR & SPI_SR_TXE)) {
        if (--timeout == 0) return;
    }
    SPI2->DR = data;

    /* Wait for BSY clear */
    timeout = 10000;
    while (SPI2->SR & SPI_SR_BSY) {
        if (--timeout == 0) return;
    }
}

/* ========================================
 * Write command to SSD1306
 * ======================================== */

static void ssd1306_write_cmd(uint8_t cmd) {
    SSD1306_DC_CMD();   /* Command mode */
    SSD1306_CS_LOW();
    spi2_write(cmd);
    SSD1306_CS_HIGH();
}

/* ========================================
 * Write data to SSD1306
 * ======================================== */

static void ssd1306_write_data(const uint8_t *data, uint16_t len) {
    SSD1306_DC_DATA();  /* Data mode */
    SSD1306_CS_LOW();
    for (uint16_t i = 0; i < len; i++) {
        spi2_write(data[i]);
    }
    SSD1306_CS_HIGH();
}

/* ========================================
 * Initialize SSD1306
 * ======================================== */

void ssd1306_init(void) {
    /* Hardware reset */
    SSD1306_RST_LOW();
    for (volatile int i = 0; i < 100000; i++);  /* ~10 ms delay */
    SSD1306_RST_HIGH();
    for (volatile int i = 0; i < 100000; i++);

    /* Init sequence for 128×32 OLED */
    ssd1306_write_cmd(SSD1306_DISPLAY_OFF);                    /* 0xAE */
    ssd1306_write_cmd(SSD1306_SET_MUX_RATIO);                  /* 0xA8 */
    ssd1306_write_cmd(0x1F);                                    /* Mux ratio = 31 (32 lines) */
    ssd1306_write_cmd(SSD1306_SET_DISPLAY_OFFSET);              /* 0xD3 */
    ssd1306_write_cmd(0x00);                                    /* No offset */
    ssd1306_write_cmd(SSD1306_SET_START_LINE | 0x00);          /* 0x40: Start line 0 */
    ssd1306_write_cmd(SSD1306_SET_SEGMENT_REMAP);              /* 0xA1: Column 127 mapped to SEG0 */
    ssd1306_write_cmd(SSD1306_SET_COM_SCAN_DIR);               /* 0xC8: Remapped mode */
    ssd1306_write_cmd(SSD1306_SET_COM_PINS);                    /* 0xDA */
    ssd1306_write_cmd(0x02);                                    /* Sequential COM, Disable left/right remap */
    ssd1306_write_cmd(SSD1306_SET_CONTRAST);                     /* 0x81 */
    ssd1306_write_cmd(0x7F);                                    /* Contrast = 127 (mid) */
    ssd1306_write_cmd(SSD1306_SET_MEMORY_MODE);                 /* 0x20 */
    ssd1306_write_cmd(0x00);                                    /* Horizontal addressing mode */
    ssd1306_write_cmd(SSD1306_NORMAL_DISPLAY);                   /* 0xA6 */
    ssd1306_write_cmd(SSD1306_SET_CLOCK_DIV);                    /* 0xD5 */
    ssd1306_write_cmd(0x80);                                    /* Clock = f_osc/1, divide = 1 → ~100 fps */
    ssd1306_write_cmd(SSD1306_CHARGE_PUMP);                      /* 0x8D */
    ssd1306_write_cmd(0x14);                                    /* Enable charge pump */
    ssd1306_write_cmd(SSD1306_SET_PRECHARGE);                    /* 0xD9 */
    ssd1306_write_cmd(0xF1);                                    /* Phase 1: 15 clocks, Phase 2: 1 clock */
    ssd1306_write_cmd(SSD1306_SET_VCOM_DETECT);                  /* 0xDB */
    ssd1306_write_cmd(0x40);                                    /* VCOMH deselect level */
    ssd1306_write_cmd(SSD1306_DISPLAY_ON);                       /* 0xAF */

    /* Clear display buffer */
    memset(g_display_buffer, 0, sizeof(g_display_buffer));
    ssd1306_update();
}

/* ========================================
 * Clear display buffer
 * ======================================== */

void ssd1306_clear(void) {
    memset(g_display_buffer, 0, sizeof(g_display_buffer));
}

/* ========================================
 * Update display from buffer
 * ======================================== */

void ssd1306_update(void) {
    /* Set column and page address range */
    ssd1306_write_cmd(SSD1306_SET_COL_ADDR);   /* 0x21 */
    ssd1306_write_cmd(0x00);                     /* Start column = 0 */
    ssd1306_write_cmd(0x7F);                     /* End column = 127 */
    ssd1306_write_cmd(SSD1306_SET_PAGE_ADDR);   /* 0x22 */
    ssd1306_write_cmd(0x00);                     /* Start page = 0 */
    ssd1306_write_cmd(0x03);                     /* End page = 3 */

    /* Write entire buffer */
    ssd1306_write_data(g_display_buffer, sizeof(g_display_buffer));
}

/* ========================================
 * Draw a pixel in the buffer
 * ======================================== */

void ssd1306_draw_pixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    uint16_t index = x + (y / 8) * SSD1306_WIDTH;
    if (color) {
        g_display_buffer[index] |= (1U << (y % 8));
    } else {
        g_display_buffer[index] &= ~(1U << (y % 8));
    }
}

/* ========================================
 * Draw a character at position (x, page)
 * ======================================== */

void ssd1306_draw_char(uint8_t x, uint8_t page, char c) {
    if (c < 0x20 || c > 0x5F) c = 0x20;  /* Clamp to printable */
    uint16_t char_index = c - 0x20;

    for (uint8_t col = 0; col < FONT_WIDTH; col++) {
        uint16_t buf_index = x + col + page * SSD1306_WIDTH;
        if (buf_index < sizeof(g_display_buffer)) {
            if (char_index < 63) {
                g_display_buffer[buf_index] = font_6x8[char_index][col];
            } else {
                /* Extended chars — fill with empty for now */
                g_display_buffer[buf_index] = 0x00;
            }
        }
    }
}

/* ========================================
 * Draw a string at position (x, page)
 * ======================================== */

void ssd1306_draw_string(uint8_t x, uint8_t page, const char *str) {
    uint8_t cursor = x;
    while (*str && cursor < SSD1306_WIDTH - FONT_WIDTH) {
        ssd1306_draw_char(cursor, page, *str);
        cursor += FONT_WIDTH;
        str++;
    }
}

/* ========================================
 * Set contrast (0-255)
 * ======================================== */

void ssd1306_set_contrast(uint8_t contrast) {
    ssd1306_write_cmd(SSD1306_SET_CONTRAST);
    ssd1306_write_cmd(contrast);
}

/* ========================================
 * Display on/off
 * ======================================== */

void ssd1306_display_on(void) {
    ssd1306_write_cmd(SSD1306_DISPLAY_ON);
}

void ssd1306_display_off(void) {
    ssd1306_write_cmd(SSD1306_DISPLAY_OFF);
}