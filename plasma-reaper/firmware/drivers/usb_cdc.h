/*
 * usb_cdc.h — USB CDC communication driver header
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef USB_CDC_H
#define USB_CDC_H

#include <stdint.h>
#include <stdbool.h>

void usb_cdc_init(void);
void usb_cdc_poll(void);
uint32_t usb_cdc_rx(uint8_t *buf, uint32_t max_len);
void usb_cdc_tx(const uint8_t *buf, uint32_t len);
bool usb_cdc_connected(void);

#endif /* USB_CDC_H */