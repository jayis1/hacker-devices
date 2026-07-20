/*
 * hart-bleeder — oled_drv.h
 * SSD1306 128x64 OLED display driver over I2C2.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef HART_BLEEDER_OLED_DRV_H
#define HART_BLEEDER_OLED_DRV_H

#include <stdint.h>

int  oled_init(void);
void oled_clear(void);
void oled_set_contrast(uint8_t val);
void oled_draw_string(int x, int y, const char *s, int size);
void oled_draw_status(uint8_t mode, uint8_t battery, uint8_t n_dev, float i_ma);
void oled_refresh(void);
void oled_off(void);
void oled_on(void);

#endif /* HART_BLEEDER_OLED_DRV_H */