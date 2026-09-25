/*
 * SAE J2716 SENT decoder interface
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef SENT_SENTINEL_SENT_H
#define SENT_SENTINEL_SENT_H

#include "board.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SENT_CRC_SEED 0x5u
#define SENT_CRC_POLYNOMIAL 0xDu
#define SENT_ENCODED_EDGE_MAX 12u

typedef enum {
    SENT_DECODE_OK = 0,
    SENT_DECODE_BAD_ARGUMENT,
    SENT_DECODE_BAD_CHANNEL,
    SENT_DECODE_BAD_SYNC,
    SENT_DECODE_BAD_PULSE,
    SENT_DECODE_BAD_CRC,
    SENT_DECODE_BAD_LENGTH
} sent_decode_result_t;

typedef struct {
    uint64_t timestamp_us;
    uint32_t sequence;
    uint32_t tick_time_ns;
    uint16_t analog_mv;
    uint16_t fast_value;
    uint16_t slow_value;
    uint8_t status;
    uint8_t data[SS_MAX_NIBBLES];
    uint8_t data_length;
    uint8_t crc;
    ss_channel_t channel;
    bool has_slow_channel;
} sent_frame_t;

typedef struct {
    uint64_t frames_seen;
    uint64_t valid_frames;
    uint64_t crc_failures;
    uint64_t timing_failures;
    uint64_t malformed_frames;
} sent_stats_t;

typedef struct {
    sent_stats_t stats;
    uint32_t last_sequence[SS_CHANNEL_COUNT];
    uint64_t last_timestamp_us[SS_CHANNEL_COUNT];
} sent_decoder_t;

void sent_decoder_init(sent_decoder_t *decoder);
uint8_t sent_crc4(const uint8_t *nibbles, size_t length);
bool sent_crc_valid(const sent_frame_t *frame);
sent_decode_result_t sent_decode_edges(sent_decoder_t *decoder,
                                       ss_channel_t channel,
                                       uint32_t sequence,
                                       uint64_t timestamp_us,
                                       uint16_t analog_mv,
                                       const uint32_t *edge_ticks,
                                       size_t edge_count,
                                       sent_frame_t *frame);
size_t sent_encode_edges(const sent_frame_t *frame, uint32_t *edges, size_t capacity);
uint16_t sent_fast_value(const sent_frame_t *frame);
uint32_t sent_fingerprint(const sent_frame_t *frame);
const char *sent_decode_result_name(sent_decode_result_t result);

#endif
