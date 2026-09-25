/*
 * SAE J2716 SENT pulse decoder and encoder
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "sent.h"

#include <limits.h>
#include <string.h>

static uint32_t absolute_difference(uint32_t left, uint32_t right)
{
    return left > right ? left - right : right - left;
}

static bool pulse_to_nibble(uint32_t pulse_ticks, uint32_t tick_ticks, uint8_t *nibble)
{
    uint32_t rounded;
    uint32_t nominal;

    if (tick_ticks == 0u || nibble == NULL) {
        return false;
    }
    rounded = (pulse_ticks + tick_ticks / 2u) / tick_ticks;
    if (rounded < SS_NIBBLE_OFFSET_TICKS || rounded > SS_NIBBLE_OFFSET_TICKS + 15u) {
        return false;
    }
    nominal = rounded * tick_ticks;
    if (absolute_difference(pulse_ticks, nominal) > tick_ticks / 3u + 1u) {
        return false;
    }
    *nibble = (uint8_t)(rounded - SS_NIBBLE_OFFSET_TICKS);
    return true;
}

void sent_decoder_init(sent_decoder_t *decoder)
{
    if (decoder != NULL) {
        memset(decoder, 0, sizeof(*decoder));
    }
}

uint8_t sent_crc4(const uint8_t *nibbles, size_t length)
{
    uint8_t crc = SENT_CRC_SEED;
    size_t index;
    unsigned bit;

    if (nibbles == NULL && length != 0u) {
        return 0u;
    }
    for (index = 0u; index < length; ++index) {
        uint8_t value = (uint8_t)(nibbles[index] & 0x0Fu);
        for (bit = 0u; bit < 4u; ++bit) {
            const uint8_t feedback = (uint8_t)(((crc >> 3u) ^ (value >> (3u - bit))) & 1u);
            crc = (uint8_t)((crc << 1u) & 0x0Fu);
            if (feedback != 0u) {
                crc ^= SENT_CRC_POLYNOMIAL;
            }
        }
    }
    return (uint8_t)(crc & 0x0Fu);
}

bool sent_crc_valid(const sent_frame_t *frame)
{
    uint8_t crc_input[SS_MAX_NIBBLES + 1u];
    size_t index;

    if (frame == NULL || frame->data_length == 0u || frame->data_length > SS_MAX_NIBBLES) {
        return false;
    }
    crc_input[0] = (uint8_t)(frame->status & 0x0Fu);
    for (index = 0u; index < frame->data_length; ++index) {
        crc_input[index + 1u] = frame->data[index];
    }
    return sent_crc4(crc_input, (size_t)frame->data_length + 1u) == frame->crc;
}

uint16_t sent_fast_value(const sent_frame_t *frame)
{
    uint16_t value = 0u;
    size_t count;
    size_t index;

    if (frame == NULL) {
        return 0u;
    }
    count = frame->data_length < 3u ? frame->data_length : 3u;
    for (index = 0u; index < count; ++index) {
        value = (uint16_t)((value << 4u) | (frame->data[index] & 0x0Fu));
    }
    return value;
}

uint32_t sent_fingerprint(const sent_frame_t *frame)
{
    uint32_t hash = 2166136261u;
    size_t index;

    if (frame == NULL) {
        return 0u;
    }
    hash = (hash ^ (uint32_t)frame->channel) * 16777619u;
    hash = (hash ^ frame->status) * 16777619u;
    hash = (hash ^ frame->data_length) * 16777619u;
    for (index = 0u; index < frame->data_length; ++index) {
        /* Preserve identity-oriented upper nibbles but mask changing signal LSBs. */
        const uint8_t stable = index < 3u ? (uint8_t)(frame->data[index] & 0x0Cu) : frame->data[index];
        hash = (hash ^ stable) * 16777619u;
    }
    return hash;
}

static sent_decode_result_t reject(sent_decoder_t *decoder, sent_decode_result_t result)
{
    if (decoder == NULL) {
        return result;
    }
    if (result == SENT_DECODE_BAD_CRC) {
        decoder->stats.crc_failures++;
    } else if (result == SENT_DECODE_BAD_SYNC || result == SENT_DECODE_BAD_PULSE) {
        decoder->stats.timing_failures++;
    } else {
        decoder->stats.malformed_frames++;
    }
    return result;
}

sent_decode_result_t sent_decode_edges(sent_decoder_t *decoder,
                                       ss_channel_t channel,
                                       uint32_t sequence,
                                       uint64_t timestamp_us,
                                       uint16_t analog_mv,
                                       const uint32_t *edge_ticks,
                                       size_t edge_count,
                                       sent_frame_t *frame)
{
    uint32_t tick_ticks;
    uint32_t sync_error;
    size_t index;

    if (decoder == NULL || edge_ticks == NULL || frame == NULL) {
        return SENT_DECODE_BAD_ARGUMENT;
    }
    decoder->stats.frames_seen++;
    if (!ss_channel_valid(channel)) {
        return reject(decoder, SENT_DECODE_BAD_CHANNEL);
    }
    if (edge_count < 4u || edge_count > SS_MAX_NIBBLES + 3u) {
        return reject(decoder, SENT_DECODE_BAD_LENGTH);
    }
    tick_ticks = (edge_ticks[0] + SS_SYNC_TICKS / 2u) / SS_SYNC_TICKS;
    if (tick_ticks == 0u) {
        return reject(decoder, SENT_DECODE_BAD_SYNC);
    }
    sync_error = absolute_difference(edge_ticks[0], tick_ticks * SS_SYNC_TICKS);
    if (sync_error > tick_ticks * 2u) {
        return reject(decoder, SENT_DECODE_BAD_SYNC);
    }

    memset(frame, 0, sizeof(*frame));
    frame->channel = channel;
    frame->sequence = sequence;
    frame->timestamp_us = timestamp_us;
    frame->analog_mv = analog_mv;
    frame->tick_time_ns = (uint32_t)(((uint64_t)tick_ticks * 1000000000ull) / SS_CAPTURE_CLOCK_HZ);
    frame->data_length = (uint8_t)(edge_count - 3u);

    if (!pulse_to_nibble(edge_ticks[1], tick_ticks, &frame->status)) {
        return reject(decoder, SENT_DECODE_BAD_PULSE);
    }
    for (index = 0u; index < frame->data_length; ++index) {
        if (!pulse_to_nibble(edge_ticks[index + 2u], tick_ticks, &frame->data[index])) {
            return reject(decoder, SENT_DECODE_BAD_PULSE);
        }
    }
    if (!pulse_to_nibble(edge_ticks[edge_count - 1u], tick_ticks, &frame->crc)) {
        return reject(decoder, SENT_DECODE_BAD_PULSE);
    }
    frame->fast_value = sent_fast_value(frame);
    if (!sent_crc_valid(frame)) {
        return reject(decoder, SENT_DECODE_BAD_CRC);
    }
    decoder->last_sequence[channel] = sequence;
    decoder->last_timestamp_us[channel] = timestamp_us;
    decoder->stats.valid_frames++;
    return SENT_DECODE_OK;
}

size_t sent_encode_edges(const sent_frame_t *frame, uint32_t *edges, size_t capacity)
{
    uint8_t crc_input[SS_MAX_NIBBLES + 1u];
    uint32_t tick_ticks;
    size_t required;
    size_t index;

    if (frame == NULL || edges == NULL || frame->data_length == 0u ||
        frame->data_length > SS_MAX_NIBBLES) {
        return 0u;
    }
    required = (size_t)frame->data_length + 3u;
    if (capacity < required || frame->tick_time_ns == 0u) {
        return 0u;
    }
    tick_ticks = (uint32_t)(((uint64_t)frame->tick_time_ns * SS_CAPTURE_CLOCK_HZ + 500000000ull) /
                            1000000000ull);
    if (tick_ticks == 0u) {
        return 0u;
    }
    edges[0] = SS_SYNC_TICKS * tick_ticks;
    edges[1] = (SS_NIBBLE_OFFSET_TICKS + (frame->status & 0x0Fu)) * tick_ticks;
    crc_input[0] = (uint8_t)(frame->status & 0x0Fu);
    for (index = 0u; index < frame->data_length; ++index) {
        crc_input[index + 1u] = (uint8_t)(frame->data[index] & 0x0Fu);
        edges[index + 2u] = (SS_NIBBLE_OFFSET_TICKS + crc_input[index + 1u]) * tick_ticks;
    }
    edges[required - 1u] = (SS_NIBBLE_OFFSET_TICKS + sent_crc4(crc_input, frame->data_length + 1u)) * tick_ticks;
    return required;
}

const char *sent_decode_result_name(sent_decode_result_t result)
{
    switch (result) {
    case SENT_DECODE_OK: return "ok";
    case SENT_DECODE_BAD_ARGUMENT: return "bad-argument";
    case SENT_DECODE_BAD_CHANNEL: return "bad-channel";
    case SENT_DECODE_BAD_SYNC: return "bad-sync";
    case SENT_DECODE_BAD_PULSE: return "bad-pulse";
    case SENT_DECODE_BAD_CRC: return "bad-crc";
    case SENT_DECODE_BAD_LENGTH: return "bad-length";
    default: return "unknown";
    }
}
