/*
 * usb_cdc.h — USB CDC log streaming + DFU trigger
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef HARMONIC_REAPER_USB_CDC_H
#define HARMONIC_REAPER_USB_CDC_H

#include <stdint.h>

void usb_cdc_init(void);
void usb_cdc_pump(void);
void usb_cdc_write(const uint8_t *data, uint16_t len);

#endif /* HARMONIC_REAPER_USB_CDC_H */
/* EOF — usb_cdc.h — jayis1 */