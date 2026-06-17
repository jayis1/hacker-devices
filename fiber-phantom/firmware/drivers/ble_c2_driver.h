/*
 * ble_c2_driver.h — BLE command & control driver (UART to nRF52840)
 * Author: jayis1
 * Date:   2026-06-17
 *
 * The nRF52840 module handles BLE 5.0 communication with the operator's
 * mobile app. This driver provides the UART interface between the RP2040
 * and the nRF52840, and implements the FiberPhantom C2 protocol.
 */

#ifndef FIBER_PHANTOM_BLE_C2_DRIVER_H
#define FIBER_PHANTOM_BLE_C2_DRIVER_H

#include <stdint.h>
#include "board.h"

/* ---- C2 protocol message types (app ↔ device) ---- */
typedef enum {
    C2_MSG_PING          = 0x01,  /* App → Dev: ping (keepalive) */
    C2_MSG_PONG          = 0x02,  /* Dev → App: pong (response to ping) */
    C2_MSG_GET_STATUS     = 0x03,  /* App → Dev: request status */
    C2_MSG_STATUS        = 0x04,  /* Dev → App: status report */
    C2_MSG_GET_STATS     = 0x05,  /* App → Dev: request capture stats */
    C2_MSG_STATS         = 0x06,  /* Dev → App: capture statistics */
    C2_MSG_SET_MODE      = 0x07,  /* App → Dev: set deployment mode */
    C2_MSG_ACK           = 0x08,  /* Dev → App: command acknowledged */
    C2_MSG_NACK          = 0x09,  /* Dev → App: command rejected */
    C2_MSG_SET_APD_BIAS  = 0x0A,  /* App → Dev: set APD bias voltage (mV) */
    C2_MSG_START_CAPTURE  = 0x0B,  /* App → Dev: begin capturing to SD */
    C2_MSG_STOP_CAPTURE  = 0x0C,  /* App → Dev: stop capturing */
    C2_MSG_SET_RULE      = 0x0D,  /* App → Dev: write MITM rule */
    C2_MSG_CLEAR_RULE    = 0x0E,  /* App → Dev: clear MITM rule slot */
    C2_MSG_CLEAR_ALL_RULES = 0x0F, /* App → Dev: clear all MITM rules */
    C2_MSG_ENABLE_MITM    = 0x10,  /* App → Dev: enable/disable MITM engine */
    C2_MSG_ENABLE_INJECT  = 0x11,  /* App → Dev: enable/disable VCSEL injection */
    C2_MSG_GET_RULE      = 0x12,  /* App → Dev: read back MITM rule */
    C2_MSG_RULE_DATA     = 0x13,  /* Dev → App: MITM rule data response */
    C2_MSG_AUTO_CALIBRATE = 0x14, /* App → Dev: run APD auto-calibration */
    C2_MSG_CALIB_RESULT  = 0x15,  /* Dev → App: calibration result (bias mV) */
    C2_MSG_GET_BATT      = 0x16,  /* App → Dev: request battery level */
    C2_MSG_BATT          = 0x17,  /* Dev → App: battery level response */
    C2_MSG_SET_AGC       = 0x18,  /* App → Dev: enable/disable APD AGC */
    C2_MSG_FRAME_NOTIFY  = 0x20,  /* Dev → App: frame notification (count) */
    C2_MSG_ALERT         = 0x21,  /* Dev → App: alert (SD full, low battery, etc.) */
    C2_MSG_GET_VERSION   = 0x30,  /* App → Dev: request firmware version */
    C2_MSG_VERSION       = 0x31,  /* Dev → App: firmware version string */
    C2_MSG_DOWNLOAD_PCAP = 0x40,  /* App → Dev: begin PCAP download */
    C2_MSG_PCAP_CHUNK    = 0x41,  /* Dev → App: PCAP data chunk */
    C2_MSG_PCAP_END      = 0x42,  /* Dev → App: PCAP download complete */
    C2_MSG_LIST_FILES    = 0x50,  /* App → Dev: list capture files on SD */
    C2_MSG_FILE_ENTRY    = 0x51,  /* Dev → App: file entry (name + size) */
    C2_MSG_FILE_LIST_END = 0x52,  /* Dev → App: file list complete */
    C2_MSG_DELETE_FILE   = 0x53,  /* App → Dev: delete capture file */
} c2_msg_type_t;

/* ---- C2 message structure ---- */
#define C2_MAX_PAYLOAD 244   /* Max payload size (BLE_CHUNK_SIZE * N, reassembled) */
#define C2_CRC8_POLY   0x07

typedef struct {
    uint8_t  type;         /* c2_msg_type_t */
    uint8_t  seq;          /* Sequence number (0–255, wraps) */
    uint8_t  flags;        /* Bit 0: CRC present, Bit 1: compressed, others reserved */
    uint8_t  length;       /* Payload length (0–244) */
    uint8_t  payload[C2_MAX_PAYLOAD];
    uint8_t  crc;          /* CRC-8 over type+seq+flags+length+payload */
} c2_msg_t;

_Static_assert(sizeof(c2_msg_t) <= 256, "c2_msg_t too large for BLE framing");

/* ---- Alert codes ---- */
typedef enum {
    ALERT_SD_FULL        = 0x01,
    ALERT_SD_REMOVED     = 0x02,
    ALERT_LOW_BATTERY    = 0x03,
    ALERT_BATTERY_CRITICAL = 0x04,
    ALERT_APD_FAULT      = 0x05,
    ALERT_FPGA_ERROR     = 0x06,
    ALERT_LINK_LOST      = 0x07,
    ALERT_MITM_MATCH     = 0x08,
    ALERT_FIFO_OVERFLOW  = 0x09,
    ALERT_OVERHEAT       = 0x0A,
} alert_code_t;

/* ---- API ---- */

/* Initialize UART1 for nRF52840 communication */
void ble_c2_init(void);

/* Send a C2 message to the app (via nRF52840 UART → BLE) */
int ble_c2_send(const c2_msg_t *msg);

/* Receive a C2 message from the app (non-blocking, returns 0 if none) */
int ble_c2_recv(c2_msg_t *msg, uint32_t timeout_ms);

/* Build and send a status message */
int ble_c2_send_status(const sys_status_t *status);

/* Build and send a stats message */
int ble_c2_send_stats(const capture_stats_t *stats);

/* Send an alert notification */
int ble_c2_send_alert(alert_code_t alert, uint8_t severity);

/* Send firmware version string */
int ble_c2_send_version(void);

/* Process incoming C2 messages (called from main loop) */
void ble_c2_poll(void);

/* CRC-8 calculation (polynomial 0x07) */
uint8_t c2_crc8(const uint8_t *data, uint32_t len);

/* Check if BLE is connected (app connected to nRF52840) */
uint8_t ble_c2_is_connected(void);

/* Set BLE connection state (called when nRF52840 notifies connection) */
void ble_c2_set_connected(uint8_t connected);

#endif /* FIBER_PHANTOM_BLE_C2_DRIVER_H */