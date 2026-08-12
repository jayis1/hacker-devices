/**
 * @file usb_cdc_driver.h
 * @brief USB CDC-ACM driver for wired communication
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef USB_CDC_DRIVER_H
#define USB_CDC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "registers.h"

#ifdef __cplusplus
extern "C" {
#endif

void usb_cdc_init(void);
uint32_t usb_cdc_send_frame_header(const frame_metadata_t *meta);
uint32_t usb_cdc_send_data(const uint8_t *data, uint16_t length);
uint32_t usb_cdc_send_ocr_text(const uint8_t *data, uint16_t length, uint32_t frame_idx);
uint32_t usb_cdc_send_credential_alert(const uint8_t *data, uint16_t length, uint32_t frame_idx);
uint32_t usb_cdc_receive_firmware(uint8_t *buffer, uint32_t max_size, uint32_t *crc_out);
bool usb_cdc_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_CDC_DRIVER_H */