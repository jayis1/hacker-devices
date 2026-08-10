/*
 * usb_cdc.h — USB 2.0 Full-Speed CDC virtual serial port
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef EMBER_USB_CDC_H
#define EMBER_USB_CDC_H

#include <stdint.h>

/* Initialize the USB FS peripheral and CDC descriptors. */
void usb_cdc_init(void);

/* Poll — call from main loop to handle CDC RX and flush TX. */
void usb_cdc_poll(void);

/* Push bytes into the TX ring buffer (non-blocking). Returns bytes queued. */
int usb_cdc_write(const char *buf, int len);

/* Convenience: write a NUL-terminated string. */
int usb_cdc_puts(const char *s);

/* Read one byte from CDC RX ring. Returns -1 if empty. */
int usb_cdc_getchar(void);

/* Returns 1 if the host has opened the serial port (DTR set). */
int usb_cdc_connected(void);

#endif /* EMBER_USB_CDC_H */
/* end of file — author: jayis1 */