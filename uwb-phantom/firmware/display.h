/*
 * display.h — SSD1306 OLED status display interface.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UWB_PHANTOM_DISPLAY_H
#define UWB_PHANTOM_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int  display_init(void);
void display_clear(void);
void display_text(uint8_t x, uint8_t y, const char *s);
void display_textf(uint8_t x, uint8_t y, const char *fmt, ...);
void display_progress(uint8_t y, uint8_t pct);
void display_flush(void);
void display_status(const char *mode, const char *line2,
                    int battery_pct, int rssi, double distance);

#ifdef __cplusplus
}
#endif

#endif /* UWB_PHANTOM_DISPLAY_H */