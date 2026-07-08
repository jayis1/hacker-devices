/*
 * tpm-phantom — drivers/usb_cdc.h
 * USB CDC virtual serial port driver interface.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef TPM_PHANTOM_USB_CDC_H
#define TPM_PHANTOM_USB_CDC_H

#include <stdint.h>

void usb_cdc_init(void);
void usb_cdc_poll(void);
uint16_t usb_cdc_send(const uint8_t *data, uint16_t len);
uint16_t usb_cdc_recv(uint8_t *buf, uint16_t max);
uint8_t usb_cdc_configured(void);
void usb_cdc_set_configured(uint8_t v);

#endif /* TPM_PHANTOM_USB_CDC_H */