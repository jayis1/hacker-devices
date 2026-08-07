/*
 * drivers/oled.h — OLED Status Display for Prism-Tap
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PRISM_TAP_OLED_H
#define PRISM_TAP_OLED_H

#include <stdint.h>

int  oled_init(void);
void oled_clear(void);
void oled_update(void);
void oled_set_pixel(uint8_t x, uint8_t y, uint8_t on);
void oled_draw_text(uint8_t x, uint8_t y, const char *text);
void oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void oled_draw_status(const char *mode, const char *link, uint8_t batt_pct,
                      uint32_t frames, uint8_t sd_present);

#endif /* PRISM_TAP_OLED_H */
/* Author: jayis1 */