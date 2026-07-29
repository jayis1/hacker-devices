/*
 * drivers/ble_c2.h — BLE command & control header
 *
 * Author:  jayis1
 * License: GPL-2.0
 */

#ifndef NVME_PHANTOM_BLE_C2_H
#define NVME_PHANTOM_BLE_C2_H

#include <stdint.h>

void ble_init(void);
void ble_set_key(const uint8_t key[16]);
int  ble_send(uint8_t cmd, const uint8_t *payload, uint16_t len);
int  ble_poll(uint8_t *cmd, uint16_t *len, uint8_t *payload, uint16_t max_len);

/* Command opcodes (see README §7.2) */
#define BLE_CMD_PING        0x01
#define BLE_CMD_GET_STATUS  0x02
#define BLE_CMD_SET_MODE    0x10
#define BLE_CMD_START_CAP   0x11
#define BLE_CMD_STOP_CAP    0x12
#define BLE_CMD_LOAD_RULES  0x20
#define BLE_CMD_CLEAR_RULES 0x21
#define BLE_CMD_INJECT_CMD  0x30
#define BLE_CMD_OPAL_SEND   0x40
#define BLE_CMD_OPAL_RECV   0x41
#define BLE_CMD_SPOOF_IDENT 0x50
#define BLE_CMD_DMA_ARM     0x60
#define BLE_CMD_DMA_GO      0x61
#define BLE_CMD_HOTPLUG     0x70
#define BLE_CMD_FW_DOWNLOAD 0x80
#define BLE_CMD_FW_COMMIT   0x81
#define BLE_CMD_GET_CAPMETA 0x90
#define BLE_CMD_GET_CAPREC  0x91

#endif /* NVME_PHANTOM_BLE_C2_H */