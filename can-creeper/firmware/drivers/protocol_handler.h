/**
 * @file protocol_handler.h
 * @brief Binary Wire Protocol Handler API
 *
 * Implements the CAN Creeper binary protocol for communication
 * over BLE NUS and USB CDC-ACM. Handles frame serialization,
 * command parsing, CRC verification, and transport abstraction.
 */

#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "can_fd_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * PROTOCOL CONSTANTS
 *===========================================================================*/

#define PROTOCOL_START_BYTE         0xAA
#define PROTOCOL_MAX_PAYLOAD        1024
#define PROTOCOL_CRC_POLY           0x1021  /* CRC-16/CCITT */
#define PROTOCOL_CRC_INIT           0xFFFF

/*===========================================================================
 * COMMAND IDs
 *===========================================================================*/

typedef enum {
    CMD_FRAME_RX        = 0x01,  /* Device → App: Received CAN frame */
    CMD_FRAME_TX        = 0x02,  /* App → Device: Transmit CAN frame */
    CMD_FRAME_TX_ACK    = 0x03,  /* Device → App: TX acknowledge/error */
    CMD_CONFIG_SET      = 0x04,  /* App → Device: Set configuration */
    CMD_CONFIG_GET      = 0x05,  /* App → Device: Get configuration */
    CMD_CONFIG_RSP      = 0x06,  /* Device → App: Configuration response */
    CMD_STATUS_REQ      = 0x07,  /* App → Device: Request status */
    CMD_STATUS_RSP      = 0x08,  /* Device → App: Status response */
    CMD_SCRIPT_LOAD     = 0x09,  /* App → Device: Load injection script */
    CMD_SCRIPT_START    = 0x0A,  /* App → Device: Start script */
    CMD_SCRIPT_STOP     = 0x0B,  /* App → Device: Stop script */
    CMD_SCRIPT_STATUS   = 0x0C,  /* Device → App: Script status */
    CMD_DBC_UPLOAD      = 0x0D,  /* App → Device: Upload DBC file */
    CMD_DBC_LIST        = 0x0E,  /* App → Device: List DBC files */
    CMD_DBC_LIST_RSP    = 0x0F,  /* Device → App: DBC list response */
    CMD_BUS_STATUS      = 0x10,  /* Device → App: Bus status update */
    CMD_ERROR_EVENT     = 0x11,  /* Device → App: Error notification */
    CMD_PING            = 0x12,  /* App → Device: Keep-alive */
    CMD_PONG            = 0x13,  /* Device → App: Keep-alive response */
    CMD_DFU_START       = 0x14,  /* App → Device: Start DFU */
    CMD_DFU_DATA        = 0x15,  /* App → Device: DFU data chunk */
    CMD_DFU_COMPLETE    = 0x16,  /* App → Device: DFU complete */
} protocol_cmd_t;

/*===========================================================================
 * TRANSPORT TYPES
 *===========================================================================*/

typedef enum {
    PROTOCOL_TRANSPORT_BLE,
    PROTOCOL_TRANSPORT_USB
} protocol_transport_t;

/*===========================================================================
 * PROTOCOL STATE
 *===========================================================================*/

typedef struct {
    uint8_t  rx_buf[PROTOCOL_MAX_PAYLOAD + 5];  /* Start + cmd + len(2) + payload + crc(2) */
    uint16_t rx_pos;
    bool     rx_in_frame;
    uint16_t rx_expected_len;

    /* TX queues */
    uint8_t  ble_tx_buf[PROTOCOL_MAX_PAYLOAD + 5];
    uint16_t ble_tx_len;
    bool     ble_tx_pending;

    uint8_t  usb_tx_buf[PROTOCOL_MAX_PAYLOAD + 5];
    uint16_t usb_tx_len;
    bool     usb_tx_pending;
} protocol_state_t;

/*===========================================================================
 * ERROR CODES
 *===========================================================================*/

#define PROTO_ERR_OK              0
#define PROTO_ERR_CRC            -1
#define PROTO_ERR_OVERFLOW       -2
#define PROTO_ERR_INVALID_CMD    -3
#define PROTO_ERR_INVALID_LEN    -4
#define PROTO_ERR_NOT_INIT       -5

/*===========================================================================
 * API
 *===========================================================================*/

int protocol_init(protocol_state_t *state);
uint16_t protocol_crc16(const uint8_t *data, uint16_t len);
int protocol_serialize_frame_rx(const can_frame_t *frame, uint8_t channel, uint8_t *buf, uint16_t *len);
int protocol_serialize_status_rsp(uint8_t *buf, uint16_t *len, uint32_t uptime, uint16_t batt_mv,
                                   bool ble_conn, bool usb_conn, uint8_t can0_tec, uint8_t can0_rec,
                                   uint8_t can1_tec, uint8_t can1_rec, uint32_t can0_count,
                                   uint32_t can1_count, uint32_t can0_ovf, uint32_t can1_ovf);
int protocol_parse(protocol_state_t *state, const uint8_t *data, uint16_t len, protocol_transport_t transport);
int protocol_queue_frame(protocol_state_t *state, uint8_t channel, const can_frame_t *frame);
int protocol_flush_ble(protocol_state_t *state);
int protocol_flush_usb(protocol_state_t *state);

#ifdef __cplusplus
}
#endif
#endif
