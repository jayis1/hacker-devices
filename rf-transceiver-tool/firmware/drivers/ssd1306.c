/*
 * ssd1306.c — SSD1306 OLED Display Driver Implementation
 *
 * I2C driver for SSD1306 128×64 OLED display at address 0x3C.
 * Uses polling I2C transfers (sufficient for 30 fps at 400 kHz).
 */

#include "ssd1306.h"
#include "board.h"
#include "registers.h"

/* Framebuffer: 128 × 8 pages = 1024 bytes */
static uint8_t fb[SSD1306_BUF_SIZE];

/* SSD1306 I2C command/data bytes */
#define SSD1306_CTRL_CMD     0x00  /* Control byte: Co=0, D/C#=0 (command) */
#define SSD1306_CTRL_DATA    0x40  /* Control byte: Co=0, D/C#=1 (data) */

/* SSD1306 Commands */
#define SSD1306_SET_DISPLAY_OFF          0xAE
#define SSD1306_SET_DISPLAY_ON           0xAF
#define SSD1306_SET_DISPLAY_CLK_DIV      0xD5
#define SSD1306_SET_MULTIPLEX            0xA8
#define SSD1306_SET_DISPLAY_OFFSET       0xD3
#define SSD1306_SET_START_LINE           0x40
#define SSD1306_SET_CHARGE_PUMP          0x8D
#define SSD1306_SET_MEMORY_MODE          0x20
#define SSD1306_SET_SEG_REMAP            0xA1
#define SSD1306_SET_COM_SCAN_DEC         0xC8
#define SSD1306_SET_COM_PINS             0xDA
#define SSD1306_SET_CONTRAST             0x81
#define SSD1306_SET_PRECHARGE            0xD9
#define SSD1306_SET_VCOM_DETECT          0xDB
#define SSD1306_SET_ENTIRE_ON            0xA5
#define SSD1306_SET_ENTIRE_OFF           0xA4
#define SSD1306_SET_NORMAL_DISPLAY       0xA6
#define SSD1306_SET_INVERT_DISPLAY       0xA7
#define SSD1306_SET_COLUMN_ADDR          0x21
#define SSD1306_SET_PAGE_ADDR            0x22

/* ========================================================================
 * I2C Helper Functions
 * ======================================================================== */

static int i2c_start(void)
{
    /* Generate START condition */
    I2Cx_CR1(I2C1_BASE) |= I2C_CR1_START;
    
    /* Wait for SB flag */
    uint32_t timeout = 10000;
    while (!(I2Cx_SR1(I2C1_BASE) & I2C_SR1_SB)) {
        if (--timeout == 0) return -1;
    }
    return 0;
}

static int i2c_send_addr(uint8_t addr, uint8_t rw)
{
    /* Send address + R/W bit */
    I2Cx_DR(I2C1_BASE) = (addr << 1) | rw;
    
    /* Wait for ADDR flag */
    uint32_t timeout = 10000;
    while (!(I2Cx_SR1(I2C1_BASE) & I2C_SR1_ADDR)) {
        if (--timeout == 0) return -1;
    }
    /* Clear ADDR by reading SR1 then SR2 */
    (void)I2Cx_SR1(I2C1_BASE);
    (void)I2Cx_SR2(I2C1_BASE);
    return 0;
}

static int i2c_send_byte(uint8_t data)
{
    /* Write data byte */
    I2Cx_DR(I2C1_BASE) = data;
    
    /* Wait for TXE or BTF */
    uint32_t timeout = 10000;
    while (!(I2Cx_SR1(I2C1_BASE) & (I2C_SR1_TXE | I2C_SR1_BTF))) {
        if (--timeout == 0) return -1;
    }
    return 0;
}

static void i2c_stop(void)
{
    I2Cx_CR1(I2C1_BASE) |= I2C_CR1_STOP;
}

/**
 * i2c_write_cmd - Send a single command byte to SSD1306
 */
static int i2c_write_cmd(uint8_t cmd)
{
    if (i2c_start() != 0) return -1;
    if (i2c_send_addr(SSD1306_I2C_ADDR >> 1, 0) != 0) return -1;
    if (i2c_send_byte(SSD1306_CTRL_CMD) != 0) return -1;
    if (i2c_send_byte(cmd) != 0) return -1;
    i2c_stop();
    return 0;
}

/**
 * i2c_write_cmd_data - Send a command with parameter
 */
static int i2c_write_cmd_data(uint8_t cmd, uint8_t data)
{
    if (i2c_start() != 0) return -1;
    if (i2c_send_addr(SSD1306_I2C_ADDR >> 1, 0) != 0) return -1;
    if (i2c_send_byte(SSD1306_CTRL_CMD) != 0) return -1;
    if (i2c_send_byte(cmd) != 0) return -1;
    if (i2c_send_byte(data) != 0) return -1;
    i2c_stop();
    return 0;
}

/* ========================================================================
 * Font: 6×8 ASCII (0x20-0x7F)
 * ======================================================================== */

/* Minimal 5×7 font (stored as 5 bytes per char, 6th column is blank) */
static const uint8_t font5x7[][5] = {
    /* 0x20 ' ' */ {0x00, 0x00, 0x00, 0x00, 0x00},
    /* 0x21 '!' */ {0x00, 0x00, 0x5F, 0x00, 0x00},
    /* 0x22 '"' */ {0x00, 0x07, 0x00, 0x07, 0x00},
    /* 0x23 '#' */ {0x14, 0x7F, 0x14, 0x7F, 0x14},
    /* 0x24 '$' */ {0x24, 0x2A, 0x7F, 0x2A, 0x12},
    /* 0x25 '%' */ {0x23, 0x13, 0x08, 0x64, 0x62},
    /* 0x26 '&' */ {0x36, 0x49, 0x55, 0x22, 0x50},
    /* 0x27 '\'' */ {0x00, 0x05, 0x03, 0x00, 0x00},
    /* 0x28 '(' */ {0x00, 0x1C, 0x22, 0x41, 0x00},
    /* 0x29 ')' */ {0x00, 0x41, 0x22, 0x1C, 0x00},
    /* 0x2A '*' */ {0x14, 0x08, 0x3E, 0x08, 0x14},
    /* 0x2B '+' */ {0x08, 0x08, 0x3E, 0x08, 0x08},
    /* 0x2C ',' */ {0x00, 0x50, 0x30, 0x00, 0x00},
    /* 0x2D '-' */ {0x08, 0x08, 0x08, 0x08, 0x08},
    /* 0x2E '.' */ {0x00, 0x60, 0x60, 0x00, 0x00},
    /* 0x2F '/' */ {0x20, 0x10, 0x08, 0x04, 0x02},
    /* 0x30 '0' */ {0x3E, 0x51, 0x49, 0x45, 0x3E},
    /* 0x31 '1' */ {0x00, 0x42, 0x7F, 0x40, 0x00},
    /* 0x32 '2' */ {0x42, 0x61, 0x51, 0x49, 0x46},
    /* 0x33 '3' */ {0x21, 0x41, 0x45, 0x4B, 0x31},
    /* 0x34 '4' */ {0x18, 0x14, 0x12, 0x7F, 0x10},
    /* 0x35 '5' */ {0x27, 0x45, 0x45, 0x45, 0x39},
    /* 0x36 '6' */ {0x3C, 0x4A, 0x49, 0x49, 0x30},
    /* 0x37 '7' */ {0x01, 0x71, 0x09, 0x05, 0x03},
    /* 0x38 '8' */ {0x36, 0x49, 0x49, 0x49, 0x36},
    /* 0x39 '9' */ {0x06, 0x49, 0x49, 0x29, 0x1E},
    /* 0x3A ':' */ {0x00, 0x36, 0x36, 0x00, 0x00},
    /* 0x3B ';' */ {0x00, 0x56, 0x36, 0x00, 0x00},
    /* 0x3C '<' */ {0x08, 0x14, 0x22, 0x41, 0x00},
    /* 0x3D '=' */ {0x14, 0x14, 0x14, 0x14, 0x14},
    /* 0x3E '>' */ {0x00, 0x41, 0x22, 0x14, 0x08},
    /* 0x3F '?' */ {0x02, 0x01, 0x51, 0x09, 0x06},
    /* 0x40 '@' */ {0x32, 0x49, 0x79, 0x41, 0x3E},
    /* 0x41 'A' */ {0x7E, 0x11, 0x11, 0x11, 0x7E},
    /* 0x42 'B' */ {0x7F, 0x49, 0x49, 0x49, 0x36},
    /* 0x43 'C' */ {0x3E, 0x41, 0x41, 0x41, 0x22},
    /* 0x44 'D' */ {0x7F, 0x41, 0x41, 0x22, 0x1C},
    /* 0x45 'E' */ {0x7F, 0x49, 0x49, 0x49, 0x41},
    /* 0x46 'F' */ {0x7F, 0x09, 0x09, 0x09, 0x01},
    /* 0x47 'G' */ {0x3E, 0x41, 0x49, 0x49, 0x7A},
    /* 0x48 'H' */ {0x7F, 0x08, 0x08, 0x08, 0x7F},
    /* 0x49 'I' */ {0x00, 0x41, 0x7F, 0x41, 0x00},
    /* 0x4A 'J' */ {0x20, 0x40, 0x41, 0x3F, 0x01},
    /* 0x4B 'K' */ {0x7F, 0x08, 0x14, 0x22, 0x41},
    /* 0x4C 'L' */ {0x7F, 0x40, 0x40, 0x40, 0x40},
    /* 0x4D 'M' */ {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    /* 0x4E 'N' */ {0x7F, 0x04, 0x08, 0x10, 0x7F},
    /* 0x4F 'O' */ {0x3E, 0x41, 0x41, 0x41, 0x3E},
    /* 0x50 'P' */ {0x7F, 0x09, 0x09, 0x09, 0x06},
    /* 0x51 'Q' */ {0x3E, 0x41, 0x51, 0x21, 0x5E},
    /* 0x52 'R' */ {0x7F, 0x09, 0x19, 0x29, 0x46},
    /* 0x53 'S' */ {0x46, 0x49, 0x49, 0x49, 0x31},
    /* 0x54 'T' */ {0x01, 0x01, 0x7F, 0x01, 0x01},
    /* 0x55 'U' */ {0x3F, 0x40, 0x40, 0x40, 0x3F},
    /* 0x56 'V' */ {0x1F, 0x20, 0x40, 0x20, 0x1F},
    /* 0x57 'W' */ {0x3F, 0x40, 0x38, 0x40, 0x3F},
    /* 0x58 'X' */ {0x63, 0x14, 0x08, 0x14, 0x63},
    /* 0x59 'Y' */ {0x07, 0x08, 0x70, 0x08, 0x07},
    /* 0x5A 'Z' */ {0x61, 0x51, 0x49, 0x45, 0x43},
    /* 0x5B '[' */ {0x00, 0x7F, 0x41, 0x41, 0x00},
    /* 0x5C '\\' */ {0x02, 0x04, 0x08, 0x10, 0x20},
    /* 0x5D ']' */ {0x00, 0x41, 0x41, 0x7F, 0x00},
    /* 0x5E '^' */ {0x04, 0x02, 0x01, 0x02, 0x04},
    /* 0x5F '_' */ {0x40, 0x40, 0x40, 0x40, 0x40},
    /* 0x60 '`' */ {0x00, 0x01, 0x02, 0x04, 0x00},
    /* 0x61 'a' */ {0x20, 0x54, 0x54, 0x54, 0x78},
    /* 0x62 'b' */ {0x7F, 0x48, 0x44, 0x44, 0x38},
    /* 0x63 'c' */ {0x38, 0x44, 0x44, 0x44, 0x20},
    /* 0x64 'd' */ {0x38, 0x44, 0x44, 0x48, 0x7F},
    /* 0x65 'e' */ {0x38, 0x54, 0x54, 0x54, 0x18},
    /* 0x66 'f' */ {0x08, 0x7E, 0x09, 0x01, 0x02},
    /* 0x67 'g' */ {0x0C, 0x52, 0x52, 0x52, 0x3E},
    /* 0x68 'h' */ {0x7F, 0x08, 0x04, 0x04, 0x78},
    /* 0x69 'i' */ {0x00, 0x44, 0x7D, 0x40, 0x00},
    /* 0x6A 'j' */ {0x20, 0x40, 0x44, 0x3D, 0x00},
    /* 0x6B 'k' */ {0x7F, 0x10, 0x28, 0x44, 0x00},
    /* 0x6C 'l' */ {0x00, 0x41, 0x7F, 0x40, 0x00},
    /* 0x6D 'm' */ {0x7C, 0x04, 0x18, 0x04, 0x78},
    /* 0x6E 'n' */ {0x7C, 0x04, 0x04, 0x04, 0x78},
    /* 0x6F 'o' */ {0x38, 0x44, 0x44, 0x44, 0x38},
    /* 0x70 'p' */ {0x7C, 0x14, 0x14, 0x14, 0x08},
    /* 0x71 'q' */ {0x08, 0x14, 0x14, 0x18, 0x7C},
    /* 0x72 'r' */ {0x7C, 0x08, 0x04, 0x04, 0x08},
    /* 0x73 's' */ {0x48, 0x54, 0x54, 0x54, 0x20},
    /* 0x74 't' */ {0x04, 0x3F, 0x44, 0x40, 0x20},
    /* 0x75 'u' */ {0x3C, 0x40, 0x40, 0x20, 0x7C},
    /* 0x76 'v' */ {0x1C, 0x20, 0x40, 0x20, 0x1C},
    /* 0x77 'w' */ {0x3C, 0x40, 0x30, 0x40, 0x3C},
    /* 0x78 'x' */ {0x44, 0x28, 0x10, 0x28, 0x44},
    /* 0x79 'y' */ {0x0C, 0x50, 0x50, 0x50, 0x3C},
    /* 0x7A 'z' */ {0x44, 0x64, 0x54, 0x4C, 0x44},
    /* 0x7B '{' */ {0x00, 0x08, 0x36, 0x41, 0x00},
    /* 0x7C '|' */ {0x00, 0x00, 0x7F, 0x00, 0x00},
    /* 0x7D '}' */ {0x00, 0x41, 0x36, 0x08, 0x00},
    /* 0x7E '~' */ {0x10, 0x08, 0x08, 0x10, 0x08},
};

/* ========================================================================
 * Display Initialization
 * ======================================================================== */

int ssd1306_init(void)
{
    /* Wait for display power-up */
    volatile uint32_t timeout = 100000;
    while (timeout--)
        ;
    
    /* Init sequence per SSD1306 datasheet */
    if (i2c_write_cmd(SSD1306_SET_DISPLAY_OFF) != 0) return -1;
    if (i2c_write_cmd_data(SSD1306_SET_DISPLAY_CLK_DIV, 0x80) != 0) return -1;
    if (i2c_write_cmd_data(SSD1306_SET_MULTIPLEX, 0x3F) != 0) return -1;
    if (i2c_write_cmd_data(SSD1306_SET_DISPLAY_OFFSET, 0x00) != 0) return -1;
    if (i2c_write_cmd(SSD1306_SET_START_LINE | 0x00) != 0) return -1;
    if (i2c_write_cmd_data(SSD1306_SET_CHARGE_PUMP, 0x14) != 0) return -1;
    if (i2c_write_cmd_data(SSD1306_SET_MEMORY_MODE, 0x00) != 0) return -1;
    if (i2c_write_cmd(SSD1306_SET_SEG_REMAP) != 0) return -1;
    if (i2c_write_cmd(SSD1306_SET_COM_SCAN_DEC) != 0) return -1;
    if (i2c_write_cmd_data(SSD1306_SET_COM_PINS, 0x12) != 0) return -1;
    if (i2c_write_cmd_data(SSD1306_SET_CONTRAST, 0xCF) != 0) return -1;
    if (i2c_write_cmd_data(SSD1306_SET_PRECHARGE, 0xF1) != 0) return -1;
    if (i2c_write_cmd_data(SSD1306_SET_VCOM_DETECT, 0x40) != 0) return -1;
    if (i2c_write_cmd(SSD1306_SET_ENTIRE_OFF) != 0) return -1;
    if (i2c_write_cmd(SSD1306_SET_NORMAL_DISPLAY) != 0) return -1;
    
    /* Clear framebuffer */
    ssd1306_clear();
    ssd1306_update();
    
    /* Turn display on */
    if (i2c_write_cmd(SSD1306_SET_DISPLAY_ON) != 0) return -1;
    
    return 0;
}

/* ========================================================================
 * Framebuffer Operations
 * ======================================================================== */

void ssd1306_clear(void)
{
    for (uint32_t i = 0; i < SSD1306_BUF_SIZE; i++) {
        fb[i] = 0x00;
    }
}

void ssd1306_update(void)
{
    /* Set column and page address range */
    i2c_write_cmd(SSD1306_SET_COLUMN_ADDR);
    i2c_write_cmd(0);       /* Column start = 0 */
    i2c_write_cmd(127);     /* Column end = 127 */
    i2c_write_cmd(SSD1306_SET_PAGE_ADDR);
    i2c_write_cmd(0);       /* Page start = 0 */
    i2c_write_cmd(7);       /* Page end = 7 */
    
    /* Send framebuffer data in chunks (I2C max 32 bytes per transaction) */
    for (uint16_t offset = 0; offset < SSD1306_BUF_SIZE; offset += 16) {
        if (i2c_start() != 0) break;
        if (i2c_send_addr(SSD1306_I2C_ADDR >> 1, 0) != 0) { i2c_stop(); break; }
        
        /* First byte is control byte: D/C#=1 (data) */
        if (i2c_send_byte(SSD1306_CTRL_DATA) != 0) { i2c_stop(); break; }
        
        /* Send 16 data bytes */
        for (uint8_t i = 0; i < 16; i++) {
            if (i2c_send_byte(fb[offset + i]) != 0) { i2c_stop(); return; }
        }
        i2c_stop();
    }
}

void ssd1306_set_pixel(uint8_t x, uint8_t y, uint8_t on)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    
    uint16_t index = (y / 8) * SSD1306_WIDTH + x;
    if (on) {
        fb[index] |= (1U << (y & 7));
    } else {
        fb[index] &= ~(1U << (y & 7));
    }
}

void ssd1306_draw_char(uint8_t x, uint8_t y, char ch)
{
    if (ch < 0x20 || ch > 0x7E) ch = '?';
    uint8_t idx = ch - 0x20;
    
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = font5x7[idx][col];
        for (uint8_t row = 0; row < 8; row++) {
            if (line & (1U << row)) {
                ssd1306_set_pixel(x + col, y + row, 1);
            } else {
                ssd1306_set_pixel(x + col, y + row, 0);
            }
        }
    }
    /* 6th column is blank (spacing) */
    for (uint8_t row = 0; row < 8; row++) {
        ssd1306_set_pixel(x + 5, y + row, 0);
    }
}

void ssd1306_draw_string(uint8_t x, uint8_t y, const char *str)
{
    if (str == NULL) return;
    uint8_t cx = x;
    while (*str && cx < SSD1306_WIDTH - FONT_WIDTH) {
        ssd1306_draw_char(cx, y, *str);
        cx += FONT_WIDTH;
        str++;
    }
}

void ssd1306_draw_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t fill)
{
    if (fill > 100) fill = 100;
    uint8_t fill_width = (uint16_t)width * fill / 100;
    
    /* Draw bar outline */
    for (uint8_t i = 0; i < width; i++) {
        ssd1306_set_pixel(x + i, y, 1);       /* Top line */
        ssd1306_set_pixel(x + i, y + 6, 1);   /* Bottom line */
    }
    for (uint8_t j = 1; j < 6; j++) {
        ssd1306_set_pixel(x, y + j, 1);       /* Left line */
        ssd1306_set_pixel(x + width - 1, y + j, 1);  /* Right line */
    }
    
    /* Fill */
    for (uint8_t i = 1; i < fill_width && i < width - 1; i++) {
        for (uint8_t j = 1; j < 6; j++) {
            ssd1306_set_pixel(x + i, y + j, 1);
        }
    }
}

void ssd1306_set_contrast(uint8_t contrast)
{
    i2c_write_cmd_data(SSD1306_SET_CONTRAST, contrast);
}

void ssd1306_set_display_on(uint8_t on)
{
    if (on) {
        i2c_write_cmd(SSD1306_SET_DISPLAY_ON);
    } else {
        i2c_write_cmd(SSD1306_SET_DISPLAY_OFF);
    }
}