/*
 * PHANTOM — USB HID Driver Header
 * Keyboard, Mouse, and Consumer Control HID Implementation
 *
 * Copyright (C) 2024 Hacker Devices
 * Licensed under GPL-2.0
 */

#ifndef USB_HID_H
#define USB_HID_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* HID report structures (defined in board.h) */
/* hid_keyboard_report_t, hid_mouse_report_t, hid_consumer_report_t */

/* Function prototypes */
void usb_device_init(void);
void usb_enumerate_msc(void);
void usb_enumerate_hid(void);
void usb_switch_to_hid(void);
void usb_switch_to_msc(void);
void usb_disconnect(void);
void usb_task(void);

/* Keyboard functions */
void hid_keyboard_send(uint8_t modifier, uint8_t keycode);
void hid_keyboard_send_raw(const hid_keyboard_report_t *report);
uint8_t hid_parse_keycode(const char *name);

/* Mouse functions */
void hid_mouse_move(int8_t dx, int8_t dy, int8_t wheel);
void hid_mouse_click(uint8_t buttons);
void hid_mouse_report(const hid_mouse_report_t *report);

/* Consumer control functions */
void hid_consumer_send(uint16_t key);
uint16_t hid_parse_consumer_keycode(const char *name);

/* HID task (called from main loop) */
void hid_task(void);

#endif /* USB_HID_H */