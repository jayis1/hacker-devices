/**
 * @file oled_driver.h
 * @brief SSD1306 OLED Display Driver API
 *
 * 128x64 pixel monochrome OLED display over I²C.
 * Address: 0x3C, I²C speed: 400 kHz.
 */

#ifndef OLED_DRIVER_H
#define OLED_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SSD1306_I2C_ADDR    0x3C
#define SSD1306_WIDTH       128
#define SSD1306_HEIGHT      64
#define SSD1306_PAGES       8

typedef enum { OLED_FONT_5X7, OLED_FONT_8X16 } oled_font_t;

int oled_init(void);
int oled_clear(void);
int oled_display(void);
int oled_set_pixel(uint8_t x, uint8_t y, bool on);
int oled_draw_char(uint8_t x, uint8_t y, char c, oled_font_t font);
int oled_draw_string(uint8_t x, uint8_t y, const char *str, oled_font_t font);
int oled_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
int oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool fill);
int oled_set_contrast(uint8_t contrast);
int oled_power_on(void);
int oled_power_off(void);
int oled_invert(bool invert);

#ifdef __cplusplus
}
#endif
#endif
