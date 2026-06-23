/*
 * ACOUSTIC-PHANTOM — UI driver
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * SSD1306 OLED display driver (128×64, I²C). Renders device status,
 * active profile, event count, and beam bearing. Minimal text-based
 * UI suitable for the 128×64 monochrome display.
 */
#ifndef UI_H
#define UI_H

#include "board.h"

void  ui_init(void);
void  ui_show_boot(void);
void  ui_show_idle(attack_profile_t profile, uint32_t event_count);
void  ui_show_armed(attack_profile_t profile);
void  ui_show_capturing(attack_profile_t profile, uint32_t event_count);
void  ui_show_calibrating(void);
void  ui_show_storage(void);
void  ui_show_tamper(void);
void  ui_show_profile(attack_profile_t profile);
void  ui_show_ble_connected(void);
void  ui_show_ble_disconnected(void);
void  ui_update_live(uint32_t event_count, int new_events, int16_t bearing);

#endif /* UI_H */