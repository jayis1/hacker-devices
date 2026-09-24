/*
 * FlexRay record framing, CRC, and capture decoder
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "flexray.h"

#include <string.h>

#define RECORD_FIXED_BYTES 20u
#define CRC11_POLYNOMIAL 0x385u
#define CRC24_POLYNOMIAL 0x5D6DCBu
#define CRC24_MASK 0xFFFFFFu

static uint16_t read_be16(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8u) | data[1]);
}

static uint32_t read_be24(const uint8_t *data)
{
    return ((uint32_t)data[0] << 16u) |
           ((uint32_t)data[1] << 8u) |
           (uint32_t)data[2];
}

static uint64_t read_be64(const uint8_t *data)
{
    uint64_t value = 0u;
    size_t index;
    for (index = 0u; index < 8u; ++index) {
        value = (value << 8u) | data[index];
    }
    return value;
}

static void write_be16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8u);
    data[1] = (uint8_t)value;
}

static void write_be24(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 16u);
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)value;
}

static void write_be64(uint8_t *data, uint64_t value)
{
    size_t index;
    for (index = 0u; index < 8u; ++index) {
        data[7u - index] = (uint8_t)(value >> (index * 8u));
    }
}

void fr_decoder_init(fr_decoder_t *decoder)
{
    if (decoder == NULL) {
        return;
    }
    memset(decoder, 0, sizeof(*decoder));
}

uint16_t fr_header_crc11(const uint8_t header[FR_HEADER_BYTES])
{
    uint16_t crc = 0x01Au;
    size_t byte_index;
    unsigned bit_index;

    if (header == NULL) {
        return 0u;
    }
    for (byte_index = 0u; byte_index < FR_HEADER_BYTES; ++byte_index) {
        for (bit_index = 0u; bit_index < 8u; ++bit_index) {
            const uint16_t incoming = (uint16_t)((header[byte_index] >> (7u - bit_index)) & 1u);
            const uint16_t feedback = (uint16_t)(((crc >> 10u) & 1u) ^ incoming);
            crc = (uint16_t)((crc << 1u) & 0x7FFu);
            if (feedback != 0u) {
                crc ^= CRC11_POLYNOMIAL;
            }
        }
    }
    return (uint16_t)(crc & 0x7FFu);
}

uint32_t fr_frame_crc24(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFEDCBAu;
    size_t byte_index;
    unsigned bit_index;

    if (data == NULL && length != 0u) {
        return 0u;
    }
    for (byte_index = 0u; byte_index < length; ++byte_index) {
        crc ^= (uint32_t)data[byte_index] << 16u;
        for (bit_index = 0u; bit_index < 8u; ++bit_index) {
            const bool high = (crc & 0x800000u) != 0u;
            crc = (crc << 1u) & CRC24_MASK;
            if (high) {
                crc ^= CRC24_POLYNOMIAL;
            }
        }
    }
    return crc & CRC24_MASK;
}

static void build_header(const fr_frame_t *frame, uint8_t header[FR_HEADER_BYTES])
{
    header[0] = (uint8_t)(((frame->flags & 0x0Fu) << 4u) | ((frame->slot_id >> 8u) & 0x07u));
    header[1] = (uint8_t)frame->slot_id;
    header[2] = (uint8_t)(frame->payload_length / 2u);
    header[3] = (uint8_t)((frame->header_crc >> 8u) & 0x07u);
    header[4] = (uint8_t)frame->header_crc;
}

static fr_decode_result_t validate_frame_fields(const fr_frame_t *frame)
{
    if (frame->channel != FS_CHANNEL_A && frame->channel != FS_CHANNEL_B) {
        return FR_DECODE_BAD_CHANNEL;
    }
    if (frame->slot_id == 0u || frame->slot_id >= FS_MAX_SLOTS) {
        return FR_DECODE_BAD_LENGTH;
    }
    if (frame->cycle > FS_MAX_CYCLE || frame->payload_length > FS_MAX_PAYLOAD_BYTES) {
        return FR_DECODE_BAD_LENGTH;
    }
    if ((frame->payload_length & 1u) != 0u) {
        return FR_DECODE_BAD_LENGTH;
    }
    return FR_DECODE_OK;
}

fr_decode_result_t fr_decode_record(fr_decoder_t *decoder,
                                    const uint8_t *record,
                                    size_t record_length,
                                    fr_frame_t *frame)
{
    uint8_t header[FR_HEADER_BYTES];
    uint8_t crc_input[FR_HEADER_BYTES + FS_MAX_PAYLOAD_BYTES];
    size_t expected_length;
    uint16_t calculated_header_crc;
    uint32_t calculated_frame_crc;
    fr_decode_result_t field_result;

    if (decoder == NULL || record == NULL || frame == NULL) {
        return FR_DECODE_BAD_ARGUMENT;
    }
    if (record_length < RECORD_FIXED_BYTES) {
        decoder->stats.malformed_records++;
        return FR_DECODE_NEED_MORE;
    }

    memset(frame, 0, sizeof(*frame));
    decoder->stats.frames_seen++;
    frame->channel = (fs_channel_t)record[0];
    frame->flags = record[1];
    frame->cycle = record[2];
    frame->slot_id = read_be16(record + 3u);
    frame->payload_length = read_be16(record + 5u);
    frame->timestamp_ticks = read_be64(record + 7u);

    expected_length = RECORD_FIXED_BYTES + frame->payload_length;
    if (frame->payload_length > FS_MAX_PAYLOAD_BYTES || record_length != expected_length) {
        decoder->stats.malformed_records++;
        return FR_DECODE_BAD_LENGTH;
    }
    field_result = validate_frame_fields(frame);
    if (field_result != FR_DECODE_OK) {
        decoder->stats.malformed_records++;
        return field_result;
    }

    if (frame->payload_length != 0u) {
        memcpy(frame->payload, record + 15u, frame->payload_length);
    }
    frame->header_crc = read_be16(record + 15u + frame->payload_length) & 0x7FFu;
    frame->frame_crc = read_be24(record + 17u + frame->payload_length);

    build_header(frame, header);
    header[3] = 0u;
    header[4] = 0u;
    calculated_header_crc = fr_header_crc11(header);
    if (calculated_header_crc != frame->header_crc) {
        decoder->stats.header_crc_errors++;
        return FR_DECODE_BAD_HEADER_CRC;
    }

    build_header(frame, header);
    memcpy(crc_input, header, FR_HEADER_BYTES);
    if (frame->payload_length != 0u) {
        memcpy(crc_input + FR_HEADER_BYTES, frame->payload, frame->payload_length);
    }
    calculated_frame_crc = fr_frame_crc24(crc_input, FR_HEADER_BYTES + frame->payload_length);
    if (calculated_frame_crc != frame->frame_crc) {
        decoder->stats.frame_crc_errors++;
        return FR_DECODE_BAD_FRAME_CRC;
    }

    decoder->stats.valid_frames++;
    decoder->stats.bytes_captured += frame->payload_length;
    if (frame->channel == FS_CHANNEL_A) {
        decoder->stats.channel_a_frames++;
    } else {
        decoder->stats.channel_b_frames++;
    }
    decoder->last_timestamp = frame->timestamp_ticks;
    decoder->last_slot = frame->slot_id;
    decoder->last_cycle = frame->cycle;
    decoder->synchronized = true;
    return FR_DECODE_OK;
}

bool fr_frame_is_startup(const fr_frame_t *frame)
{
    return frame != NULL && (frame->flags & FS_FRAME_FLAG_STARTUP) != 0u;
}

bool fr_frame_is_sync(const fr_frame_t *frame)
{
    return frame != NULL && (frame->flags & FS_FRAME_FLAG_SYNC) != 0u;
}

bool fr_frame_is_null(const fr_frame_t *frame)
{
    return frame != NULL && (frame->flags & FS_FRAME_FLAG_NULL) != 0u;
}

uint32_t fr_payload_fingerprint(const fr_frame_t *frame)
{
    uint32_t hash = 2166136261u;
    size_t index;
    if (frame == NULL) {
        return 0u;
    }
    for (index = 0u; index < frame->payload_length; ++index) {
        hash ^= frame->payload[index];
        hash *= 16777619u;
    }
    hash ^= frame->slot_id;
    hash *= 16777619u;
    hash ^= frame->payload_length;
    return hash;
}

size_t fr_encode_record(const fr_frame_t *frame, uint8_t *output, size_t capacity)
{
    fr_frame_t prepared;
    uint8_t header[FR_HEADER_BYTES];
    uint8_t crc_input[FR_HEADER_BYTES + FS_MAX_PAYLOAD_BYTES];
    size_t required;

    if (frame == NULL || output == NULL || validate_frame_fields(frame) != FR_DECODE_OK) {
        return 0u;
    }
    required = RECORD_FIXED_BYTES + frame->payload_length;
    if (capacity < required) {
        return 0u;
    }
    prepared = *frame;
    prepared.header_crc = 0u;
    build_header(&prepared, header);
    prepared.header_crc = fr_header_crc11(header);
    build_header(&prepared, header);
    memcpy(crc_input, header, FR_HEADER_BYTES);
    if (prepared.payload_length != 0u) {
        memcpy(crc_input + FR_HEADER_BYTES, prepared.payload, prepared.payload_length);
    }
    prepared.frame_crc = fr_frame_crc24(crc_input, FR_HEADER_BYTES + prepared.payload_length);

    output[0] = (uint8_t)prepared.channel;
    output[1] = prepared.flags;
    output[2] = prepared.cycle;
    write_be16(output + 3u, prepared.slot_id);
    write_be16(output + 5u, prepared.payload_length);
    write_be64(output + 7u, prepared.timestamp_ticks);
    if (prepared.payload_length != 0u) {
        memcpy(output + 15u, prepared.payload, prepared.payload_length);
    }
    write_be16(output + 15u + prepared.payload_length, prepared.header_crc);
    write_be24(output + 17u + prepared.payload_length, prepared.frame_crc);
    return required;
}

const char *fr_decode_result_name(fr_decode_result_t result)
{
    switch (result) {
    case FR_DECODE_OK: return "ok";
    case FR_DECODE_NEED_MORE: return "need-more";
    case FR_DECODE_BAD_ARGUMENT: return "bad-argument";
    case FR_DECODE_BAD_LENGTH: return "bad-length";
    case FR_DECODE_BAD_HEADER_CRC: return "bad-header-crc";
    case FR_DECODE_BAD_FRAME_CRC: return "bad-frame-crc";
    case FR_DECODE_BAD_CHANNEL: return "bad-channel";
    default: return "unknown";
    }
}
