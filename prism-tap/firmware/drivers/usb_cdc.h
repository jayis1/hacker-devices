/*
 * drivers/usb_cdc.h — USB CDC Virtual Serial for Prism-Tap
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PRISM_TAP_USB_CDC_H
#define PRISM_TAP_USB_CDC_H

#include <stdint.h>

int  usb_init(void);
int  usb_is_connected(void);
void usb_poll(void);
int  usb_write(const uint8_t *data, uint32_t len);
int  usb_read(uint8_t *data, uint32_t max_len);
void usb_set_baud_callback(void (*cb)(uint32_t baud));

/* Bulk frame exfil mode */
int  usb_start_bulk_exfil(void);
int  usb_stop_bulk_exfil(void);
int  usb_bulk_exfil_active(void);

#endif /* PRISM_TAP_USB_CDC_H */
/* Author: jayis1 */