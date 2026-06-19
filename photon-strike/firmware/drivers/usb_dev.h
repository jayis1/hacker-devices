/**
 * drivers/usb_dev.h — USB CDC console + DFU
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PHOTONSTRIKE_USB_DEV_H
#define PHOTONSTRIKE_USB_DEV_H

#include <stdint.h>
#include <stdbool.h>

void usb_init(void);
void usb_print(const char *s);
void usb_printf(const char *fmt, ...);
bool usb_readline(char *buf, uint16_t maxlen);

#endif