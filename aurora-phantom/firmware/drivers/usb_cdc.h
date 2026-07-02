/*
 * usb_cdc.h — USB-C CDC console
 * Author: jayis1   License: GPL-2.0
 */
#ifndef AURORA_USB_CDC_H
#define AURORA_USB_CDC_H
#include <stdint.h>

int  usb_cdc_init(void);
int  usb_cdc_write(const uint8_t *data, uint32_t len);
int  usb_cdc_read(uint8_t *buf, uint32_t max);
void usb_cdc_puts(const char *s);
void usb_cdc_dump_regs(void);

#endif