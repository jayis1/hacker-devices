/* Credential Canary protocol engines
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef CREDENTIAL_CANARY_PROTOCOL_H
#define CREDENTIAL_CANARY_PROTOCOL_H
#include "board.h"

typedef struct {
    uint64_t bits;
    uint8_t bit_count;
    uint32_t first_edge_us;
    uint32_t last_edge_us;
    uint16_t glitches;
    bool overflow;
} wiegand_decoder_t;

typedef struct {
    uint8_t buffer[OSDP_FRAME_MAX];
    uint16_t length;
    uint16_t expected;
    uint32_t last_byte_us;
    bool escaped;
} osdp_decoder_t;

typedef struct {
    uint8_t address;
    uint8_t control;
    uint8_t command;
    uint16_t payload_length;
    bool uses_crc;
    bool secure_block;
    bool valid_check;
    uint8_t payload[OSDP_FRAME_MAX - 8u];
} osdp_frame_t;

void protocol_init(void);
void wiegand_edge(interface_t iface, bool data_one, uint32_t timestamp_us);
void wiegand_poll(uint32_t now_us);
void osdp_rx_byte(interface_t iface, uint8_t byte, uint32_t timestamp_us);
void osdp_poll(uint32_t now_us);
bool osdp_decode(const uint8_t *raw, uint16_t length, osdp_frame_t *frame);
uint16_t osdp_crc16(const uint8_t *data, uint16_t length);
uint8_t osdp_checksum(const uint8_t *data, uint16_t length);
uint64_t wiegand_payload(uint64_t raw, uint8_t bit_count);
bool wiegand_parity_valid(uint64_t raw, uint8_t bit_count);
void protocol_set_forwarding(bool enabled);

#endif
