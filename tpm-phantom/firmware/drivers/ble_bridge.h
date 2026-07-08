/*
 * tpm-phantom — drivers/ble_bridge.h
 * BLE UART bridge interface.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef TPM_PHANTOM_BLE_BRIDGE_H
#define TPM_PHANTOM_BLE_BRIDGE_H

#include <stdint.h>

typedef void (*ble_msg_handler_t)(uint8_t cmd, uint8_t *payload, uint8_t len);

void ble_bridge_init(void);
void ble_bridge_reset(void);
void ble_bridge_set_handler(ble_msg_handler_t h);
uint8_t ble_bridge_send(uint8_t cmd, const uint8_t *payload, uint8_t len);

extern volatile uint32_t ble_frames_tx;
extern volatile uint32_t ble_frames_rx;
extern volatile uint32_t ble_crc_errors;

#endif /* TPM_PHANTOM_BLE_BRIDGE_H */