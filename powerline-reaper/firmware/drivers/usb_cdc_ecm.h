/*
 * usb_cdc_ecm.h — USB composite gadget interface
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef USB_CDC_ECM_H
#define USB_CDC_ECM_H

#include <stdint.h>
#include "qca7420.h"

#ifdef __cplusplus
extern "C" {
#endif

void usb_cdc_ecm_init(void);
void usb_cdc_ecm_tick(void);
void usb_cdc_ecm_inject(const qca7420_frame_t *f);
void usb_cdc_acm_rx(const uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* USB_CDC_ECM_H */