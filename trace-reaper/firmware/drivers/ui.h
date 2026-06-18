/*
 * TRACE-REAPER — local UI driver header
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef UI_H
#define UI_H

#include <stdint.h>

void ui_init(void);
void ui_clear(void);
void ui_text(uint8_t row, const char *s);
void ui_led_status(uint8_t on);
void ui_led_mode(uint8_t on);
void ui_led_arm(uint8_t on);
void ui_poll(uint32_t now_ms);
int  ui_btn_mode_take(void);
int  ui_btn_arm_take(void);
int  ui_btn_dump_take(void);

#endif /* UI_H */