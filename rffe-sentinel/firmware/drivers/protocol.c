/* RFFE Sentinel RSCP/1 codec
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "protocol.h"

#include <string.h>

static uint16_t read_u16(const uint8_t *data)
{
    return (uint16_t)data[0] | (uint16_t)((uint16_t)data[1] << 8u);
}

static uint32_t read_u32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

static void write_u16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

static void write_u32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

uint32_t rscp_crc32c(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xffffffffu;
    size_t index;
    uint8_t bit;

    if ((data == NULL) && (length != 0u)) {
        return 0u;
    }
    for (index = 0u; index < length; ++index) {
        crc ^= data[index];
        for (bit = 0u; bit < 8u; ++bit) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1u) ^ (0x82f63b78u & mask);
        }
    }
    return ~crc;
}

size_t rscp_encode(const struct rscp_message *message, uint8_t *out,
                   size_t capacity)
{
    size_t total;
    uint32_t crc;

    if ((message == NULL) || (out == NULL) ||
        (message->payload_length > RSCP_MAX_PAYLOAD)) {
        return 0u;
    }
    total = RSCP_HEADER_SIZE + message->payload_length;
    if (capacity < total) {
        return 0u;
    }

    memset(out, 0, total);
    write_u32(out, RSCP_MAGIC);
    out[4] = message->version;
    out[5] = message->type;
    write_u16(out + 6u, message->flags);
    write_u32(out + 8u, message->sequence);
    write_u32(out + 12u, message->nonce);
    write_u16(out + 16u, message->payload_length);
    if (message->payload_length > 0u) {
        memcpy(out + RSCP_HEADER_SIZE, message->payload,
               message->payload_length);
    }
    crc = rscp_crc32c(out, 18u);
    crc = rscp_crc32c(out + RSCP_HEADER_SIZE, message->payload_length) ^ crc;
    write_u16(out + 18u, (uint16_t)(crc ^ (crc >> 16u)));
    return total;
}

bool rscp_decode(const uint8_t *data, size_t length,
                 struct rscp_message *out)
{
    uint16_t payload_length;
    uint16_t expected;
    uint32_t crc;

    if ((data == NULL) || (out == NULL) || (length < RSCP_HEADER_SIZE)) {
        return false;
    }
    if (read_u32(data) != RSCP_MAGIC) {
        return false;
    }
    if (data[4] != RSCP_VERSION) {
        return false;
    }
    payload_length = read_u16(data + 16u);
    if ((payload_length > RSCP_MAX_PAYLOAD) ||
        (length != RSCP_HEADER_SIZE + payload_length)) {
        return false;
    }
    expected = read_u16(data + 18u);
    crc = rscp_crc32c(data, 18u);
    crc = rscp_crc32c(data + RSCP_HEADER_SIZE, payload_length) ^ crc;
    if ((uint16_t)(crc ^ (crc >> 16u)) != expected) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->version = data[4];
    out->type = data[5];
    out->flags = read_u16(data + 6u);
    out->sequence = read_u32(data + 8u);
    out->nonce = read_u32(data + 12u);
    out->payload_length = payload_length;
    if (payload_length > 0u) {
        memcpy(out->payload, data + RSCP_HEADER_SIZE, payload_length);
    }
    return true;
}

bool rscp_is_mutating(uint8_t type)
{
    switch (type) {
    case RSCP_SET_MODE:
    case RSCP_LOAD_POLICY:
    case RSCP_ARM_LEASE:
    case RSCP_CLEAR_EVENTS:
    case RSCP_FACTORY_RESET:
        return true;
    case RSCP_GET_STATUS:
    case RSCP_LIST_EVENTS:
    case RSCP_READ_EVENTS:
    case RSCP_GET_CAPABILITIES:
    case RSCP_ACK:
    case RSCP_ERROR:
    default:
        return false;
    }
}
