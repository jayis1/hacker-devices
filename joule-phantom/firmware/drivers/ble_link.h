/*
 * ble_link.h — BLE back-haul UART protocol header (Joule-Phantom).
 *
 * Author : jayis1
 * License: GPL-2.0
 *
 * Defines the framing between the STM32L4 main MCU and the ESP32-C3
 * BLE co-processor.  Frames are length-prefixed and CRC-8 protected.
 */

#ifndef JOULE_PHANTOM_BLE_LINK_H
#define JOULE_PHANTOM_BLE_LINK_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Message types (opcode space) */
typedef enum {
    BLE_MSG_HELLO         = 0x01,  /* device -> app: handshake           */
    BLE_MSG_STATUS        = 0x02,  /* device -> app: periodic status     */
    BLE_MSG_FRAME         = 0x10,  /* device -> app: captured SMBus frame*/
    BLE_MSG_RULE_ADD      = 0x20,  /* app -> device: add MITM rule       */
    BLE_MSG_RULE_CLR      = 0x21,  /* app -> device: clear rules         */
    BLE_MSG_RULE_LIST     = 0x22,  /* app -> device: request rule list   */
    BLE_MSG_RULE_LIST_RSP = 0x23,  /* device -> app: rule list response  */
    BLE_MSG_INJECT        = 0x30,  /* app -> device: inject raw SMBus op */
    BLE_MSG_GLITCH        = 0x40,  /* app -> device: fire glitch pulse   */
    BLE_MSG_SET_MODE      = 0x50,  /* app -> device: set operating mode  */
    BLE_MSG_AUTH_BYPASS   = 0x60,  /* app -> device: enable auth bypass  */
    BLE_MSG_ACK           = 0xF0,  /* generic acknowledgement            */
    BLE_MSG_NACK          = 0xF1
} ble_msg_t;

/* Operating modes */
typedef enum {
    MODE_PASSTHROUGH = 0,   /* transparent bridge, capture only */
    MODE_MITM        = 1,   /* apply active MITM rules          */
    MODE_JAM         = 2,   /* NACK all transactions (DoS)      */
    MODE_SPOOF_FULL  = 3,   /* present a completely fake battery*/
    MODE_STANDBY     = 4    /* idle, both ports tristated       */
} op_mode_t;

/* Frame structure on the wire:
 *   [SOF=0xAA][LEN][OP][payload...][CRC8][EOF=0x55]
 * LEN does not include SOF/LEN/OP/CRC/EOF.
 */
#define BLE_SOF 0xAA
#define BLE_EOF 0x55

/* Public API */
void ble_init(void);

/* Send a framed message (non-blocking, best-effort). */
void ble_send(ble_msg_t op, const uint8_t *payload, uint8_t len);

/* Poll the UART RX line and reassemble frames; calls ble_dispatch()
 * for each complete inbound frame. */
void ble_poll(void);

/* Dispatch a fully-decoded inbound frame. */
void ble_dispatch(ble_msg_t op, const uint8_t *payload, uint8_t len);

/* CRC-8 (poly 0x07, init 0x00) */
uint8_t ble_crc8(const uint8_t *data, uint8_t len);

/* Set current operating mode */
void ble_set_mode(op_mode_t m);
op_mode_t ble_get_mode(void);

/* Periodic status beacon (called from main loop) */
void ble_status_beacon(void);

#ifdef __cplusplus
}
#endif

#endif /* JOULE_PHANTOM_BLE_LINK_H */