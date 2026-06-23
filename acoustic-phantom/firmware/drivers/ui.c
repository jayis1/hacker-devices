/*
 * ACOUSTIC-PHANTOM — UI driver implementation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * SSD1306 OLED display driver (128×64, I2C1). Implements basic text
 * rendering for device status, active profile, event count, and beam
 * bearing. Uses a 6×8 font stored in flash.
 */

#include <string.h>
#include <stdio.h>
#include "ui.h"
#include "registers.h"
#include "board.h"

/* ---- SSD1306 commands -------------------------------------------------- */
#define SSD1306_CMD_SET_CONTRAST    0x81
#define SSD1306_CMD_ENTIRE_ON       0xA4
#define SSD1306_CMD_NORMAL_DISP     0xA6
#define SSD1306_CMD_DISP_ON         0xAF
#define SSD1306_CMD_DISP_OFF        0xAE
#define SSD1306_CMD_SET_COL_ADDR    0x21
#define SSD1306_CMD_SET_PAGE_ADDR   0x22
#define SSD1306_CMD_SET_START_LINE  0x40
#define SSD1306_CMD_SET_SEG_REMAP   0xA1
#define SSD1306_CMD_SET_COM_SCAN    0xC8
#define SSD1306_CMD_SET_MUX_RATIO   0xA8
#define SSD1306_CMD_SET_DISP_OFFSET 0xD3
#define SSD1306_CMD_SET_CHARGE_PUMP 0x8D
#define SSD1306_CMD_SET_VCOMH       0xDB
#define SSD1306_CMD_SET_OSC_FREQ    0xD5
#define SSD1306_CMD_SET_PRECHARGE   0xD9

/* ---- Display buffer (128×64 / 8 = 128×8 pages = 1024 bytes) ------------ */
#define OLED_WIDTH   128
#define OLED_HEIGHT  64
#define OLED_PAGES   (OLED_HEIGHT / 8)
static uint8_t s_display_buf[OLED_WIDTH * OLED_PAGES];

/* ---- 5×7 font (compact subset for status display) ---------------------- */
/* Each character is 5 columns × 7 rows, stored as 5 bytes (one per column).
 * Bit 0 = top row, bit 6 = bottom row, bit 7 unused. */
static const uint8_t font5x7[][5] = {
    /* Space */ {0x00,0x00,0x00,0x00,0x00},
    /* ! */     {0x00,0x00,0x5F,0x00,0x00},
    /* " */     {0x00,0x07,0x00,0x07,0x00},
    /* # */     {0x14,0x7F,0x14,0x7F,0x14},
    /* $ */     {0x24,0x2A,0x7F,0x2A,0x12},
    /* % */     {0x23,0x13,0x08,0x64,0x62},
    /* & */     {0x36,0x49,0x55,0x22,0x50},
    /* ' */     {0x00,0x05,0x03,0x00,0x00},
    /* ( */     {0x00,0x1C,0x22,0x41,0x00},
    /* ) */     {0x00,0x41,0x22,0x1C,0x00},
    /* * */     {0x08,0x2A,0x1C,0x2A,0x08},
    /* + */     {0x08,0x08,0x3E,0x08,0x08},
    /* , */     {0x00,0x50,0x30,0x00,0x00},
    /* - */     {0x08,0x08,0x08,0x08,0x08},
    /* . */     {0x00,0x60,0x60,0x00,0x00},
    /* / */     {0x20,0x10,0x08,0x04,0x02},
    /* 0-9 */   {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
                {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
                {0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},
                {0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
                {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},
    /* : */     {0x00,0x36,0x36,0x00,0x00},
    /* ; */     {0x00,0x56,0x36,0x00,0x00},
    /* < */     {0x00,0x08,0x14,0x22,0x41},
    /* = */     {0x14,0x14,0x14,0x14,0x14},
    /* > */     {0x41,0x22,0x14,0x08,0x00},
    /* ? */     {0x02,0x01,0x51,0x09,0x06},
    /* @ */     {0x32,0x49,0x79,0x41,0x3E},
    /* A-Z */   {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},
                {0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},
                {0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x01,0x01},
                {0x3E,0x41,0x41,0x51,0x32},{0x7F,0x08,0x08,0x08,0x7F},
                {0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},
                {0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
                {0x7F,0x02,0x04,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},
                {0x3E,0x41,0x41,0x41,0x3E},{0x7F,0x09,0x09,0x09,0x06},
                {0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
                {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},
                {0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x20,0x20,0x1F},
                {0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
                {0x03,0x04,0x78,0x04,0x03},{0x61,0x51,0x49,0x45,0x43},
    /* [ */     {0x00,0x7F,0x41,0x41,0x00},
    /* \ */     {0x02,0x04,0x08,0x10,0x20},
    /* ] */     {0x00,0x41,0x41,0x7F,0x00},
    /* ^ */     {0x04,0x02,0x01,0x02,0x04},
    /* _ */     {0x40,0x40,0x40,0x40,0x40},
};

/* ---- I2C write to SSD1306 ----------------------------------------------- */
static void ssd1306_cmd(uint8_t cmd)
{
    /* Control byte: Co=0, D/C#=0 (command) */
    uint8_t buf[2] = {0x00, cmd};

    I2C1->CR2 = (I2C1_ADDR_OLED << 1) | (2U << 16) | (1U << 25);
    I2C1->CR2 |= I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXE)) { }
    I2C1->TXDR = buf[0];
    while (!(I2C1->ISR & I2C_ISR_TXE)) { }
    I2C1->TXDR = buf[1];
    while (!(I2C1->ISR & I2C_ISR_TC)) { }
}

static void ssd1306_data(const uint8_t *data, uint16_t len)
{
    /* Control byte: Co=0, D/C#=1 (data) */
    I2C1->CR2 = (I2C1_ADDR_OLED << 1) | ((len + 1) << 16) | (1U << 25);
    I2C1->CR2 |= I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXE)) { }
    I2C1->TXDR = 0x40;  /* data stream */
    for (uint16_t i = 0; i < len; i++) {
        while (!(I2C1->ISR & I2C_ISR_TXE)) { }
        I2C1->TXDR = data[i];
    }
    while (!(I2C1->ISR & I2C_ISR_TC)) { }
}

/* ---- Character lookup -------------------------------------------------- */
static int char_to_font_idx(char c)
{
    if (c == ' ') return 0;
    if (c >= '!' && c <= 'Z') {
        return c - '!' + 1;
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 33;  /* map lowercase to uppercase */
    }
    return 0;  /* unknown → space */
}

/* ---- Draw a character at (x, page, ...) -------------------------------- */
static void draw_char(uint8_t x, uint8_t page, char c)
{
    if (x + 6 > OLED_WIDTH) return;
    int idx = char_to_font_idx(c);
    for (int col = 0; col < 5; col++) {
        s_display_buf[page * OLED_WIDTH + x + col] = font5x7[idx][col];
    }
    s_display_buf[page * OLED_WIDTH + x + 5] = 0;  /* spacing column */
}

/* ---- Draw a string ----------------------------------------------------- */
static void draw_string(uint8_t x, uint8_t page, const char *str)
{
    while (*str && x + 6 <= OLED_WIDTH) {
        draw_char(x, page, *str);
        x += 6;
        str++;
    }
}

/* ---- Update the physical display --------------------------------------- */
static void oled_flush(void)
{
    ssd1306_cmd(SSD1306_CMD_SET_COL_ADDR);
    ssd1306_cmd(0);
    ssd1306_cmd(OLED_WIDTH - 1);
    ssd1306_cmd(SSD1306_CMD_SET_PAGE_ADDR);
    ssd1306_cmd(0);
    ssd1306_cmd(OLED_PAGES - 1);
    ssd1306_data(s_display_buf, OLED_WIDTH * OLED_PAGES);
}

/* ---- Public API -------------------------------------------------------- */
void ui_init(void)
{
    /* SSD1306 initialization sequence */
    ssd1306_cmd(SSD1306_CMD_DISP_OFF);
    ssd1306_cmd(SSD1306_CMD_SET_MUX_RATIO);  ssd1306_cmd(0x3F);
    ssd1306_cmd(SSD1306_CMD_SET_DISP_OFFSET); ssd1306_cmd(0x00);
    ssd1306_cmd(SSD1306_CMD_SET_START_LINE);
    ssd1306_cmd(SSD1306_CMD_SET_SEG_REMAP);
    ssd1306_cmd(SSD1306_CMD_SET_COM_SCAN);
    ssd1306_cmd(SSD1306_CMD_SET_OSC_FREQ);  ssd1306_cmd(0x80);
    ssd1306_cmd(SSD1306_CMD_SET_PRECHARGE); ssd1306_cmd(0xF1);
    ssd1306_cmd(SSD1306_CMD_SET_CHARGE_PUMP); ssd1306_cmd(0x14);
    ssd1306_cmd(SSD1306_CMD_SET_VCOMH);    ssd1306_cmd(0x40);
    ssd1306_cmd(SSD1306_CMD_SET_CONTRAST); ssd1306_cmd(0xCF);
    ssd1306_cmd(SSD1306_CMD_ENTIRE_ON);
    ssd1306_cmd(SSD1306_CMD_NORMAL_DISP);
    ssd1306_cmd(SSD1306_CMD_DISP_ON);

    memset(s_display_buf, 0, sizeof(s_display_buf));
    oled_flush();
}

void oled_clear(void)
{
    memset(s_display_buf, 0, sizeof(s_display_buf));
}

void oled_draw_str(uint8_t x, uint8_t y, const char *str)
{
    draw_string(x, y / 8, str);
}

void oled_draw_line(uint8_t y, const char *str)
{
    draw_string(0, y / 8, str);
}

void oled_refresh(void)
{
    oled_flush();
}

/* ---- Profile names ----------------------------------------------------- */
static const char *profile_names[] = {
    "KEYBOARD", "HDD", "PRINTER", "SMPS", "RELAY"
};

/* ---- Screen implementations -------------------------------------------- */
void ui_show_boot(void)
{
    oled_clear();
    draw_string(0, 0, "ACOUSTIC-PHANTOM");
    draw_string(0, 1, "v1.0.0 jayis1");
    draw_string(0, 3, "Initializing...");
    oled_flush();
}

void ui_show_idle(attack_profile_t profile, uint32_t event_count)
{
    char buf[22];
    oled_clear();
    draw_string(0, 0, "ACOUSTIC-PHANTOM");
    draw_string(0, 1, "IDLE");
    draw_string(0, 2, "Profile:");
    draw_string(60, 2, profile_names[profile]);
    snprintf(buf, sizeof(buf), "Events: %lu", (unsigned long)event_count);
    draw_string(0, 3, buf);
    draw_string(0, 5, "PWR=ARM  PROF=Next");
    oled_flush();
}

void ui_show_armed(attack_profile_t profile)
{
    oled_clear();
    draw_string(0, 0, "ACOUSTIC-PHANTOM");
    draw_string(0, 1, "ARMED");
    draw_string(0, 2, "Profile:");
    draw_string(60, 2, profile_names[profile]);
    draw_string(0, 3, "Listening...");
    draw_string(0, 5, "ARM=Capture");
    oled_flush();
}

void ui_show_capturing(attack_profile_t profile, uint32_t event_count)
{
    char buf[22];
    oled_clear();
    draw_string(0, 0, "CAPTURING");
    draw_string(0, 1, profile_names[profile]);
    snprintf(buf, sizeof(buf), "Events: %lu", (unsigned long)event_count);
    draw_string(0, 2, buf);
    draw_string(0, 4, "ARM=Pause");
    oled_flush();
}

void ui_show_calibrating(void)
{
    oled_clear();
    draw_string(0, 0, "CALIBRATING");
    draw_string(0, 1, "Press keys as");
    draw_string(0, 2, "prompted by app");
    draw_string(0, 4, "Do not move");
    oled_flush();
}

void ui_show_storage(void)
{
    oled_clear();
    draw_string(0, 0, "STORAGE MODE");
    draw_string(0, 1, "Use app to");
    draw_string(0, 2, "browse files");
    oled_flush();
}

void ui_show_tamper(void)
{
    oled_clear();
    draw_string(0, 0, "!!! TAMPER !!!");
    draw_string(0, 2, "Memory wiped");
    draw_string(0, 3, "Power cycle");
    draw_string(0, 4, "to reset");
    oled_flush();
}

void ui_show_profile(attack_profile_t profile)
{
    char buf[22];
    oled_clear();
    draw_string(0, 0, "PROFILE:");
    draw_string(0, 2, profile_names[profile]);
    snprintf(buf, sizeof(buf), "Profile %d/%d", profile + 1, PROFILE_COUNT);
    draw_string(0, 4, buf);
    oled_flush();
}

void ui_show_ble_connected(void)
{
    draw_string(100, 0, "BLE");
    oled_flush();
}

void ui_show_ble_disconnected(void)
{
    /* Clear the BLE indicator area */
    for (int i = 100; i < 118; i++) {
        s_display_buf[i] = 0;  /* page 0 */
    }
    oled_flush();
}

void ui_update_live(uint32_t event_count, int new_events, int16_t bearing)
{
    char buf[22];
    snprintf(buf, sizeof(buf), "E:%lu N:%d B:%d",
             (unsigned long)event_count, new_events, bearing);
    draw_string(0, 3, buf);
    oled_flush();
}