/* RSCP/1 framed USB protocol
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef RFFE_SENTINEL_PROTOCOL_H
#define RFFE_SENTINEL_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RSCP_MAGIC 0x50435352u
#define RSCP_VERSION 1u
#define RSCP_HEADER_SIZE 20u
#define RSCP_MAX_PAYLOAD 512u

enum rscp_type {
    RSCP_GET_STATUS = 1,
    RSCP_SET_MODE = 2,
    RSCP_LOAD_POLICY = 3,
    RSCP_ARM_LEASE = 4,
    RSCP_LIST_EVENTS = 5,
    RSCP_READ_EVENTS = 6,
    RSCP_CLEAR_EVENTS = 7,
    RSCP_GET_CAPABILITIES = 8,
    RSCP_FACTORY_RESET = 9,
    RSCP_ACK = 0x80,
    RSCP_ERROR = 0x81
};

struct rscp_message {
    uint8_t version;
    uint8_t type;
    uint16_t flags;
    uint32_t sequence;
    uint32_t nonce;
    uint16_t payload_length;
    uint8_t payload[RSCP_MAX_PAYLOAD];
};

uint32_t rscp_crc32c(const uint8_t *data, size_t length);
size_t rscp_encode(const struct rscp_message *message, uint8_t *out,
                   size_t capacity);
bool rscp_decode(const uint8_t *data, size_t length,
                 struct rscp_message *out);
bool rscp_is_mutating(uint8_t type);

#endif
