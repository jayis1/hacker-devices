/**
 * @file usb_cdc_driver.c
 * @brief USB CDC-ACM driver implementation
 *
 * USB 2.0 CDC-ACM virtual serial port for wired frame exfiltration
 * and device control. Uses the nRF52840 USBD peripheral.
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "usb_cdc_driver.h"
#include "board.h"
#include "registers.h"
#include <string.h>

static bool s_usb_connected = false;
static uint8_t s_usb_tx_buffer[512];
static uint16_t s_usb_tx_len = 0;

void usb_cdc_init(void)
{
    /* Enable USB peripheral */
    NRF_USBD->ENABLE = USBD_ENABLE_ENABLE;
    for (volatile int i = 0; i < 100; i++);

    /* Pull up D+ to signal device presence */
    NRF_USBD->USBPULLUP = 1;

    /* In a real implementation, this would set up:
     * - CDC-ACM interface descriptors
     * - Bulk IN endpoint (EP1) for data to host
     * - Bulk OUT endpoint (EP1) for data from host
     * - Interrupt IN endpoint (EP2) for notifications
     * - CDC class requests (SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE)
     */
}

uint32_t usb_cdc_send_frame_header(const frame_metadata_t *meta)
{
    if (!s_usb_connected || meta == NULL) return 0;

    s_usb_tx_buffer[0] = MSG_FRAME_DATA;
    memcpy(&s_usb_tx_buffer[1], meta, sizeof(frame_metadata_t));
    s_usb_tx_len = 1 + sizeof(frame_metadata_t);

    /* Write to USB IN endpoint */
    /* USBD_TASKS_STARTEPIN(USB_EP_BULK_IN) = 1; */
    /* Wait for completion */

    return s_usb_tx_len;
}

uint32_t usb_cdc_send_data(const uint8_t *data, uint16_t length)
{
    if (!s_usb_connected || data == NULL) return 0;

    uint32_t total_sent = 0;
    uint16_t offset = 0;

    while (offset < length) {
        uint16_t chunk = MIN(USB_BULK_MAX_PACKET_SIZE, length - offset);
        memcpy(s_usb_tx_buffer, data + offset, chunk);
        s_usb_tx_len = chunk;

        /* Write to USB IN endpoint and wait for completion */
        total_sent += chunk;
        offset += chunk;
    }

    return total_sent;
}

uint32_t usb_cdc_send_ocr_text(const uint8_t *data, uint16_t length, uint32_t frame_idx)
{
    if (!s_usb_connected) return 0;

    s_usb_tx_buffer[0] = MSG_OCR_TEXT;
    s_usb_tx_buffer[1] = (frame_idx >> 24) & 0xFF;
    s_usb_tx_buffer[2] = (frame_idx >> 16) & 0xFF;
    s_usb_tx_buffer[3] = (frame_idx >> 8) & 0xFF;
    s_usb_tx_buffer[4] = frame_idx & 0xFF;
    s_usb_tx_buffer[5] = (length >> 8) & 0xFF;
    s_usb_tx_buffer[6] = length & 0xFF;
    s_usb_tx_len = 7;

    /* Send header then data */
    return 7 + usb_cdc_send_data(data, length);
}

uint32_t usb_cdc_send_credential_alert(const uint8_t *data, uint16_t length, uint32_t frame_idx)
{
    if (!s_usb_connected) return 0;

    s_usb_tx_buffer[0] = MSG_CREDENTIAL_ALERT;
    s_usb_tx_buffer[1] = (frame_idx >> 24) & 0xFF;
    s_usb_tx_buffer[2] = (frame_idx >> 16) & 0xFF;
    s_usb_tx_buffer[3] = (frame_idx >> 8) & 0xFF;
    s_usb_tx_buffer[4] = frame_idx & 0xFF;
    s_usb_tx_buffer[5] = (length >> 8) & 0xFF;
    s_usb_tx_buffer[6] = length & 0xFF;
    s_usb_tx_len = 7;

    return 7 + usb_cdc_send_data(data, length);
}

uint32_t usb_cdc_receive_firmware(uint8_t *buffer, uint32_t max_size, uint32_t *crc_out)
{
    /* Wait for firmware data on USB OUT endpoint */
    uint32_t offset = 0;

    while (offset < max_size) {
        /* Read from USB OUT endpoint */
        uint16_t chunk = 0; /* Would read from USBD */
        if (chunk == 0) break;
        offset += chunk;
    }

    if (crc_out) *crc_out = 0;
    return offset;
}

bool usb_cdc_is_connected(void)
{
    return s_usb_connected;
}