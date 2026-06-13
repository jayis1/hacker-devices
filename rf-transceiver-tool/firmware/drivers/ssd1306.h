/*
 * ssd1306.h — SSD1306 OLED Display Driver (I2C)
 *
 * Driver for 128×64 monochrome OLED on I2C1 (address 0x3C).
 * Supports text rendering, pixel graphics, and RSSI bar display.
 */

#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64
#define SSD1306_PAGES   (SSD1306_HEIGHT / 8)
#define SSD1306_BUF_SIZE (SSD1306_WIDTH * SSD1306_PAGES)

/* Font: 6×8 ASCII printable characters (0x20-0x7F) */
#define FONT_WIDTH     6
#define FONT_HEIGHT     8

/**
 * ssd1306_init - Initialize SSD1306 display via I2C
 * Returns: 0 on success, -1 on I2C error
 */
int ssd1306_init(void);

/**
 * ssd1306_clear - Clear the display buffer
 */
void ssd1306_clear(void);

/**
 * ssd1306_update - Send display buffer to OLED
 * Transfers entire framebuffer via I2C.
 */
void ssd1306_update(void);

/**
 * ssd1306_set_pixel - Set/clear a pixel in the buffer
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-63)
 * @param on 1=set, 0=clear
 */
void ssd1306_set_pixel(uint8_t x, uint8_t y, uint8_t on);

/**
 * ssd1306_draw_char - Draw a character at a position
 * @param x X position (in pixels)
 * @param y Y position (in pixels, aligned to 8-pixel pages)
 * @param ch ASCII character (0x20-0x7F)
 */
void ssd1306_draw_char(uint8_t x, uint8_t y, char ch);

/**
 * ssd1306_draw_string - Draw a string at a position
 * @param x X position (in pixels)
 * @param y Y position (in pixels, aligned to 8-pixel pages)
 * @param str Null-terminated string
 */
void ssd1306_draw_string(uint8_t x, uint8_t y, const char *str);

/**
 * ssd1306_draw_bar - Draw a horizontal bar (for RSSI, signal level)
 * @param x X position
 * @param y Y position (page-aligned)
 * @param width Max width of bar
 * @param fill Fill percentage (0-100)
 */
void ssd1306_draw_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t fill);

/**
 * ssd1306_set_contrast - Set display contrast
 * @param contrast Contrast value (0-255)
 */
void ssd1306_set_contrast(uint8_t contrast);

/**
 * ssd1306_set_display_on - Turn display on/off
 * @param on 1=on, 0=off
 */
void ssd1306_set_display_on(uint8_t on);

#endif /* SSD1306_H */