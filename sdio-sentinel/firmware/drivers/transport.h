/* SDIO Sentinel USB protocol. Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SDIO_SENTINEL_TRANSPORT_H
#define SDIO_SENTINEL_TRANSPORT_H

#include "policy.h"

#define SS_PACKET_MAGIC_0 0x53u
#define SS_PACKET_MAGIC_1 0x44u
#define SS_PACKET_HEADER_SIZE 10u

struct ss_packet {
    uint8_t version;
    uint8_t type;
    uint16_t sequence;
    uint16_t length;
    uint8_t payload[SS_MAX_HOST_PAYLOAD];
};

enum ss_packet_type {
    SS_PKT_HELLO = 1,
    SS_PKT_STATUS = 2,
    SS_PKT_FRAME = 3,
    SS_PKT_AUDIT = 4,
    SS_PKT_SET_MODE = 16,
    SS_PKT_CONFIRM_SCOPE = 17,
    SS_PKT_SET_RULE = 18,
    SS_PKT_CLEAR_RULES = 19,
    SS_PKT_EXPORT_AUDIT = 20,
    SS_PKT_ERROR = 127
};

struct ss_decoder {
    uint8_t buffer[SS_PACKET_HEADER_SIZE + SS_MAX_HOST_PAYLOAD + 4u];
    size_t used;
    size_t expected;
    uint32_t discarded;
};

struct ss_status_snapshot {
    uint32_t board_id;
    uint32_t uptime_ms;
    uint32_t frames_seen;
    uint32_t blocked;
    uint32_t anomalies;
    uint32_t clock_hz;
    uint8_t mode;
    uint8_t card_state;
    uint8_t armed;
    uint8_t card_present;
};

void ss_decoder_init(struct ss_decoder *decoder);
uint32_t ss_crc32(const uint8_t *data, size_t length);
bool ss_decoder_push(struct ss_decoder *decoder, uint8_t byte, struct ss_packet *packet);
size_t ss_packet_encode(const struct ss_packet *packet, uint8_t *output, size_t capacity);
bool ss_packet_make_status(const struct ss_status_snapshot *status, uint16_t sequence,
                           struct ss_packet *packet);
bool ss_packet_make_error(uint16_t sequence, uint8_t code, const char *message,
                          struct ss_packet *packet);
bool ss_packet_read_u32(const struct ss_packet *packet, size_t offset, uint32_t *value);
void ss_packet_write_u32(uint8_t *output, uint32_t value);

#endif
