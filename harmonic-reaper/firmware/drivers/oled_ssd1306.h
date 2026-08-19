/*
 * oled_ssd1306.h — OLED UI driver
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef HARMONIC_REAPER_OLED_H
#define HARMONIC_REAPER_OLED_H

#include <stdint.h>
#include "../board.h"

void oled_init(void);
void oled_show_boot(void);
void oled_set_mode(sweep_mode_t m);
void oled_update_live(int16_t p2, int16_t p3, int8_t ratio,
                      uint8_t classify, uint8_t batt, uint8_t charging,
                      sweep_mode_t mode, uint32_t hits,
                      int16_t pitch, int16_t yaw);
void oled_flash_hit(int16_t p2, int8_t ratio, uint32_t hit_num);
void oled_flash_low_battery(uint8_t pct);

#endif /* HARMONIC_REAPER_OLED_H */
/* EOF — oled_ssd1306.h — jayis1 */