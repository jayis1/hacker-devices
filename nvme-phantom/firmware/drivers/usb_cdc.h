/*
 * drivers/usb_cdc.h — USB CDC-ACM header
 *
 * Author:  jayis1
 * License: GPL-2.0
 */

#ifndef NVME_PHANTOM_USB_CDC_H
#define NVME_PHANTOM_USB_CDC_H

#include <stdint.h>

void usb_cdc_init(void);
void usb_cdc_poll(void);
int  usb_cdc_getc(uint8_t *c);
int  usb_cdc_putc(uint8_t c);
int  usb_cdc_write(const uint8_t *data, uint16_t len);

#endif /* NVME_PHANTOM_USB_CDC_H */