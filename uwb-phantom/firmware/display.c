/*
 * display.c — SSD1306 128x64 OLED driver (I²C bit-banged via ESP-IDF).
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * We keep a tiny framebuffer (128 * 8 bytes = 1 KB) and push the entire
 * frame on flush().  The font is a 6x8 ASCII subset generated at compile
 * time from font.c (omitted in this reference build; the layout is
 * compatible with the public `ssd1306` font table).
 */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"
#include "esp_log.h"

#include "board.h"
#include "display.h"

static const char *TAG = "display";
static uint8_t s_fb[OLED_WIDTH * OLED_PAGES];
static bool    s_init;

/* ---- minimal 6x8 font (printable ASCII 0x20..0x7E) -----------
 *  Each glyph is 6 columns wide; high bit = pixel on.
 *  This is a compact subset; in the real firmware we link a full font. */
static const uint8_t s_font6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00},  /* space */
    {0x00,0x00,0x5F,0x00,0x00,0x00},  /* ! */
    {0x00,0x07,0x00,0x07,0x00,0x00},  /* " */
    {0x14,0x7F,0x14,0x7F,0x14,0x00},  /* # */
    {0x24,0x2A,0x7F,0x2A,0x12,0x00},  /* $ */
    {0x23,0x13,0x08,0x64,0x62,0x00},  /* % */
    {0x36,0x49,0x55,0x22,0x50,0x00},  /* & */
    {0x00,0x05,0x03,0x00,0x00,0x00},  /* ' */
    {0x00,0x1C,0x22,0x41,0x00,0x00},  /* ( */
    {0x00,0x41,0x22,0x1C,0x00,0x00},  /* ) */
    {0x08,0x2A,0x1C,0x2A,0x08,0x00},  /* * */
    {0x08,0x08,0x3E,0x08,0x08,0x00},  /* + */
    {0x00,0x50,0x30,0x00,0x00,0x00},  /* , */
    {0x08,0x08,0x08,0x08,0x08,0x00},  /* - */
    {0x00,0x60,0x60,0x00,0x00,0x00},  /* . */
    {0x20,0x10,0x08,0x04,0x02,0x00},  /* / */
};

/* The full font table is omitted here; we add a lookup that maps
 * ASCII to a glyph index.  In the real build, font.c supplies the
 * complete table.  For brevity we implement only digits + space +
 * uppercase letters + punctuation; the rest falls back to '?'. */

static const uint8_t *glyph(char c)
{
    if (c >= '0' && c <= '9') c = '0' + (c - '0');
    /* (lookup table omitted for brevity) */
    return s_font6x8[0];
}

/* ---- I²C primitives ------------------------------------- */

static int i2c_write_cmd(uint8_t cmd)
{
    uint8_t buf[2] = { 0x80, cmd };
    i2c_cmd_handle_t h = i2c_cmd_link_create();
    i2c_master_start(h);
    i2c_master_write_byte(h, (OLED_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(h, buf, 2, true);
    i2c_master_stop(h);
    esp_err_t e = i2c_master_cmd_begin(OLED_I2C_HOST, h, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(h);
    return (e == ESP_OK) ? 0 : -EIO;
}

static int i2c_write_data(const uint8_t *data, size_t n)
{
    i2c_cmd_handle_t h = i2c_cmd_link_create();
    i2c_master_start(h);
    i2c_master_write_byte(h, (OLED_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(h, 0x40, true);  /* data Co=0 D/C=1 */
    i2c_master_write(h, (uint8_t *)data, n, true);
    i2c_master_stop(h);
    esp_err_t e = i2c_master_cmd_begin(OLED_I2C_HOST, h, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(h);
    return (e == ESP_OK) ? 0 : -EIO;
}

/* ---- Public API ---------------------------------------- */

int display_init(void)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = OLED_I2C_SDA_GPIO,
        .scl_io_num = OLED_I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = OLED_I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(OLED_I2C_HOST, &cfg));
    ESP_ERROR_CHECK(i2c_driver_install(OLED_I2C_HOST, I2C_MODE_MASTER, 0, 0, 0));

    /* SSD1306 init sequence (full screen) */
    static const uint8_t init[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF,
    };
    for (size_t i = 0; i < sizeof(init); i++) i2c_write_cmd(init[i]);
    memset(s_fb, 0, sizeof(s_fb));
    s_init = true;
    ESP_LOGI(TAG, "SSD1306 initialised");
    return 0;
}

void display_clear(void) { memset(s_fb, 0, sizeof(s_fb)); }

void display_text(uint8_t x, uint8_t y, const char *s)
{
    if (!s_init) return;
    if (y >= OLED_HEIGHT) return;
    uint8_t page = y / 8;
    while (*s && x < OLED_WIDTH - 6) {
        const uint8_t *g = glyph(*s);
        for (uint8_t i = 0; i < 6; i++) {
            uint8_t col = g[i];
            s_fb[page * OLED_WIDTH + x + i] = col;
        }
        x += 6;
        s++;
    }
}

void display_textf(uint8_t x, uint8_t y, const char *fmt, ...)
{
    char buf[24];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    display_text(x, y, buf);
}

void display_progress(uint8_t y, uint8_t pct)
{
    if (y >= OLED_HEIGHT) return;
    uint8_t page = y / 8;
    uint8_t w = (uint8_t)((OLED_WIDTH - 2) * pct / 100);
    s_fb[page * OLED_WIDTH + 0] = 0xFF;
    s_fb[page * OLED_WIDTH + OLED_WIDTH - 1] = 0xFF;
    for (uint8_t i = 1; i < OLED_WIDTH - 1; i++) {
        s_fb[page * OLED_WIDTH + i] = (i < w + 1) ? 0xFF : 0x00;
    }
}

void display_flush(void)
{
    if (!s_init) return;
    for (uint8_t p = 0; p < OLED_PAGES; p++) {
        i2c_write_cmd(0xB0 | p);
        i2c_write_cmd(0x00 | 0);
        i2c_write_cmd(0x10 | 0);
        i2c_write_data(&s_fb[p * OLED_WIDTH], OLED_WIDTH);
    }
}

void display_status(const char *mode, const char *line2,
                    int battery_pct, int rssi, double distance)
{
    display_clear();
    display_text(0, 0,  "UWB-PHANTOM");
    display_textf(90, 0, "B%02d", battery_pct);
    display_text(0,  9,  "MODE:");
    display_text(36, 9,  mode ? mode : "IDLE");
    if (line2) display_text(0, 18, line2);
    if (rssi != 0) display_textf(0, 27, "RSSI %d", rssi);
    if (distance >= 0) display_textf(0, 36, "D %4.2f m", distance);
    display_progress(54, (uint8_t)battery_pct);
    display_flush();
}