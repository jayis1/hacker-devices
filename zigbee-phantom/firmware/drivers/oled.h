/*
 * drivers/oled.h — SH1106 OLED driver (128x64, SPI)
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef ZIGBEE_PHANTOM_OLED_H
#define ZIGBEE_PHANTOM_OLED_H

#include <stdint.h>
#include <stdbool.h>

int  oled_init(void);
void oled_clear(void);
void oled_power(bool on);
void oled_set_cursor(uint8_t col, uint8_t row);
void oled_puts(const char *s);                 /* 6x8 font, wraps */
void oled_putc(char c);
void oled_draw_hbar(uint8_t col, uint8_t row, uint8_t width, uint8_t fill_pct);
void oled_invert(bool on);
void oled_flip(bool on);

/* Status line helpers used by main.c */
void oled_status_channel(uint8_t ch, int8_t rssi);
void oled_status_mode(uint8_t mode);
void oled_status_counters(uint32_t frames, uint32_t keys, uint32_t injected);

#endif