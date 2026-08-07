/*
 * drivers/protocol.h — Binary Command Protocol for Prism-Tap
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PRISM_TAP_PROTOCOL_H
#define PRISM_TAP_PROTOCOL_H

#include <stdint.h>

/* Command opcodes (shared with ble_if.h) */
#include "ble_if.h"

/* Initialize and register all command handlers */
int  protocol_init(void);

/* Dispatch a command (used by both BLE and USB) */
int  protocol_dispatch(uint8_t opcode, const uint8_t *payload, uint8_t len,
                       uint8_t *response, uint8_t *resp_len);

/* Get/set configuration */
typedef struct __attribute__((packed)) {
    uint8_t  csi_lanes;
    uint32_t csi_speed;
    uint16_t csi_width;
    uint16_t csi_height;
    uint8_t  csi_format;
    uint8_t  dsi_lanes;
    uint32_t dsi_speed;
    uint16_t dsi_width;
    uint16_t dsi_height;
    uint8_t  dsi_format;
    uint8_t  jpeg_compress;
    uint8_t  reserved[8];
} config_packet_t;

#endif /* PRISM_TAP_PROTOCOL_H */
/* Author: jayis1 */