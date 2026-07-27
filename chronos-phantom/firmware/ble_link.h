/*
 * ble_link.h — UART link to ESP32-C3 BLE module for app communication
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef CHRONOS_PHANTOM_BLE_LINK_H
#define CHRONOS_PHANTOM_BLE_LINK_H

#include <stdint.h>
#include "ptp_engine.h"
#include "ntp_engine.h"
#include "covert_codec.h"

/* Command opcodes from the companion app */
typedef enum {
    CMD_GET_STATUS        = 0x01,
    CMD_SET_MODE          = 0x02,
    CMD_SET_SKEW_PROFILE  = 0x03,
    CMD_SET_SKEW_PRESET   = 0x04,
    CMD_BMCA_SPOOF        = 0x05,
    CMD_BMCA_AUTO_WIN     = 0x06,
    CMD_COVERT_SEND       = 0x07,
    CMD_COVERT_RECV_START = 0x08,
    CMD_COVERT_RECV_STOP  = 0x09,
    CMD_CAPTURE_START    = 0x0A,
    CMD_CAPTURE_STOP     = 0x0B,
    CMD_GNSS_DISCIPLINE  = 0x0C,
    CMD_TAMPER_THRESHOLD = 0x0D,
    CMD_FIRMWARE_UPDATE  = 0x0E,
    CMD_SET_SPOOF_GM     = 0x0F,
    CMD_PING             = 0x10
} ble_cmd_t;

/* Response/event opcodes from device to app */
typedef enum {
    EVT_STATUS            = 0x81,
    EVT_FRAME_CAPTURED    = 0x82,
    EVT_COVERT_RX         = 0x83,
    EVT_TAMPER            = 0x84,
    EVT_GNSS_LOCK         = 0x85,
    EVT_PING_RESP         = 0x90,
    EVT_ERROR             = 0xFF
} ble_evt_t;

/* BLE link state */
typedef struct {
    uint8_t  rx_buf[BLE_UART_BUF_SIZE];
    uint16_t rx_idx;
    uint16_t rx_expected;
    uint8_t  tx_buf[BLE_UART_BUF_SIZE];
    uint16_t tx_len;
    uint8_t  connected;
} ble_link_t;

void ble_link_init(ble_link_t *st);
void ble_link_uart_rx_byte(ble_link_t *st, uint8_t b);
void ble_link_send_event(ble_link_t *st, uint8_t evt_code,
                         const uint8_t *payload, uint16_t len);
int  ble_link_tx_drain(ble_link_t *st, uint8_t *out, uint16_t max);

/* Dispatch a received command — called by main loop with current state */
void ble_link_dispatch(ble_link_t *ble,
                       ptp_engine_state_t *ptp,
                       ntp_engine_state_t *ntp,
                       covert_state_t *covert,
                       uint8_t *mode_out);

#endif /* CHRONOS_PHANTOM_BLE_LINK_H */