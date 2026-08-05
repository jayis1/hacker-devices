/*
 * oled.h — OLED status display driver header
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef OLED_H
#define OLED_H

#include <stdint.h>

void oled_init(void);
void oled_clear(void);
void oled_refresh(void);
void oled_set_cursor(uint8_t row, uint8_t col);
void oled_print(const char *str);
void oled_print_u32(uint32_t val);

#endif /* OLED_H */