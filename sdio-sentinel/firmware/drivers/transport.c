/*
 * Bounded USB framing for SDIO Sentinel
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "transport.h"

#include <string.h>

static uint16_t read_u16(const uint8_t *input)
{
    return (uint16_t)input[0] | (uint16_t)((uint16_t)input[1] << 8u);
}

static uint32_t read_u32(const uint8_t *input)
{
    return (uint32_t)input[0] |
           ((uint32_t)input[1] << 8u) |
           ((uint32_t)input[2] << 16u) |
           ((uint32_t)input[3] << 24u);
}

static void write_u16(uint8_t *output, uint16_t value)
{
    output[0] = (uint8_t)(value & 0xFFu);
    output[1] = (uint8_t)(value >> 8u);
}

void ss_packet_write_u32(uint8_t *output, uint32_t value)
{
    if (output == NULL) {
        return;
    }
    output[0] = (uint8_t)(value & 0xFFu);
    output[1] = (uint8_t)((value >> 8u) & 0xFFu);
    output[2] = (uint8_t)((value >> 16u) & 0xFFu);
    output[3] = (uint8_t)((value >> 24u) & 0xFFu);
}

void ss_decoder_init(struct ss_decoder *decoder)
{
    if (decoder == NULL) {
        return;
    }
    memset(decoder, 0, sizeof(*decoder));
    decoder->expected = SS_PACKET_HEADER_SIZE;
}

uint32_t ss_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFu;
    if (data == NULL) {
        return 0u;
    }
    for (size_t i = 0u; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0u; bit < 8u; ++bit) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1u) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

static void decoder_resync(struct ss_decoder *decoder, uint8_t byte)
{
    decoder->discarded++;
    decoder->used = 0u;
    decoder->expected = SS_PACKET_HEADER_SIZE;
    if (byte == SS_PACKET_MAGIC_0) {
        decoder->buffer[0] = byte;
        decoder->used = 1u;
    }
}

static bool header_valid(const uint8_t *buffer, uint16_t *length)
{
    if (buffer[0] != SS_PACKET_MAGIC_0 || buffer[1] != SS_PACKET_MAGIC_1 ||
        buffer[2] != SS_PROTOCOL_VERSION) {
        return false;
    }
    *length = read_u16(&buffer[6]);
    return *length <= SS_MAX_HOST_PAYLOAD;
}

bool ss_decoder_push(struct ss_decoder *decoder, uint8_t byte, struct ss_packet *packet)
{
    if (decoder == NULL || packet == NULL) {
        return false;
    }
    if (decoder->used == 0u && byte != SS_PACKET_MAGIC_0) {
        decoder->discarded++;
        return false;
    }
    if (decoder->used == 1u && byte != SS_PACKET_MAGIC_1) {
        decoder_resync(decoder, byte);
        return false;
    }
    if (decoder->used >= sizeof(decoder->buffer)) {
        decoder_resync(decoder, byte);
        return false;
    }
    decoder->buffer[decoder->used++] = byte;

    if (decoder->used == SS_PACKET_HEADER_SIZE) {
        uint16_t length = 0u;
        if (!header_valid(decoder->buffer, &length)) {
            decoder_resync(decoder, byte);
            return false;
        }
        decoder->expected = SS_PACKET_HEADER_SIZE + (size_t)length + 4u;
    }
    if (decoder->used < decoder->expected) {
        return false;
    }

    size_t body_length = decoder->expected - 4u;
    uint32_t expected_crc = read_u32(&decoder->buffer[body_length]);
    uint32_t actual_crc = ss_crc32(decoder->buffer, body_length);
    if (expected_crc != actual_crc) {
        decoder_resync(decoder, byte);
        return false;
    }

    packet->version = decoder->buffer[2];
    packet->type = decoder->buffer[3];
    packet->sequence = read_u16(&decoder->buffer[4]);
    packet->length = read_u16(&decoder->buffer[6]);
    if (packet->length > 0u) {
        memcpy(packet->payload, &decoder->buffer[SS_PACKET_HEADER_SIZE],
               packet->length);
    }
    decoder->used = 0u;
    decoder->expected = SS_PACKET_HEADER_SIZE;
    return true;
}

size_t ss_packet_encode(const struct ss_packet *packet, uint8_t *output, size_t capacity)
{
    if (packet == NULL || output == NULL || packet->length > SS_MAX_HOST_PAYLOAD) {
        return 0u;
    }
    size_t total = SS_PACKET_HEADER_SIZE + (size_t)packet->length + 4u;
    if (capacity < total) {
        return 0u;
    }
    output[0] = SS_PACKET_MAGIC_0;
    output[1] = SS_PACKET_MAGIC_1;
    output[2] = packet->version;
    output[3] = packet->type;
    write_u16(&output[4], packet->sequence);
    write_u16(&output[6], packet->length);
    output[8] = 0u;
    output[9] = 0u;
    if (packet->length > 0u) {
        memcpy(&output[SS_PACKET_HEADER_SIZE], packet->payload, packet->length);
    }
    uint32_t crc = ss_crc32(output, total - 4u);
    ss_packet_write_u32(&output[total - 4u], crc);
    return total;
}

bool ss_packet_make_status(const struct ss_status_snapshot *status, uint16_t sequence,
                           struct ss_packet *packet)
{
    if (status == NULL || packet == NULL) {
        return false;
    }
    memset(packet, 0, sizeof(*packet));
    packet->version = SS_PROTOCOL_VERSION;
    packet->type = SS_PKT_STATUS;
    packet->sequence = sequence;
    packet->length = 28u;
    ss_packet_write_u32(&packet->payload[0], status->board_id);
    ss_packet_write_u32(&packet->payload[4], status->uptime_ms);
    ss_packet_write_u32(&packet->payload[8], status->frames_seen);
    ss_packet_write_u32(&packet->payload[12], status->blocked);
    ss_packet_write_u32(&packet->payload[16], status->anomalies);
    ss_packet_write_u32(&packet->payload[20], status->clock_hz);
    packet->payload[24] = status->mode;
    packet->payload[25] = status->card_state;
    packet->payload[26] = status->armed;
    packet->payload[27] = status->card_present;
    return true;
}

bool ss_packet_make_error(uint16_t sequence, uint8_t code, const char *message,
                          struct ss_packet *packet)
{
    if (message == NULL || packet == NULL) {
        return false;
    }
    size_t message_length = strlen(message);
    if (message_length > SS_MAX_HOST_PAYLOAD - 1u) {
        message_length = SS_MAX_HOST_PAYLOAD - 1u;
    }
    memset(packet, 0, sizeof(*packet));
    packet->version = SS_PROTOCOL_VERSION;
    packet->type = SS_PKT_ERROR;
    packet->sequence = sequence;
    packet->length = (uint16_t)(message_length + 1u);
    packet->payload[0] = code;
    memcpy(&packet->payload[1], message, message_length);
    return true;
}

bool ss_packet_read_u32(const struct ss_packet *packet, size_t offset, uint32_t *value)
{
    if (packet == NULL || value == NULL || offset > packet->length ||
        packet->length - offset < 4u) {
        return false;
    }
    *value = read_u32(&packet->payload[offset]);
    return true;
}
