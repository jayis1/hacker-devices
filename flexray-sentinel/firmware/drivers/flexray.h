/*
 * FlexRay capture decoder API
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef FLEXRAY_SENTINEL_FLEXRAY_H
#define FLEXRAY_SENTINEL_FLEXRAY_H

#include "board.h"
#include "registers.h"

#define FR_HEADER_BYTES 5u
#define FR_TRAILER_BYTES 3u
#define FR_ENCODED_RECORD_MAX (20u + FS_MAX_PAYLOAD_BYTES)

typedef enum {
    FR_DECODE_OK = 0,
    FR_DECODE_NEED_MORE,
    FR_DECODE_BAD_ARGUMENT,
    FR_DECODE_BAD_LENGTH,
    FR_DECODE_BAD_HEADER_CRC,
    FR_DECODE_BAD_FRAME_CRC,
    FR_DECODE_BAD_CHANNEL
} fr_decode_result_t;

typedef struct {
    uint64_t timestamp_ticks;
    uint16_t slot_id;
    uint16_t payload_length;
    uint16_t header_crc;
    uint32_t frame_crc;
    uint8_t cycle;
    uint8_t flags;
    fs_channel_t channel;
    uint8_t payload[FS_MAX_PAYLOAD_BYTES];
} fr_frame_t;

typedef struct {
    uint64_t frames_seen;
    uint64_t valid_frames;
    uint64_t header_crc_errors;
    uint64_t frame_crc_errors;
    uint64_t malformed_records;
    uint64_t channel_a_frames;
    uint64_t channel_b_frames;
    uint64_t bytes_captured;
} fr_decoder_stats_t;

typedef struct {
    fr_decoder_stats_t stats;
    uint64_t last_timestamp;
    uint16_t last_slot;
    uint8_t last_cycle;
    bool synchronized;
} fr_decoder_t;

void fr_decoder_init(fr_decoder_t *decoder);
fr_decode_result_t fr_decode_record(fr_decoder_t *decoder,
                                    const uint8_t *record,
                                    size_t record_length,
                                    fr_frame_t *frame);
uint16_t fr_header_crc11(const uint8_t header[FR_HEADER_BYTES]);
uint32_t fr_frame_crc24(const uint8_t *data, size_t length);
bool fr_frame_is_startup(const fr_frame_t *frame);
bool fr_frame_is_sync(const fr_frame_t *frame);
bool fr_frame_is_null(const fr_frame_t *frame);
uint32_t fr_payload_fingerprint(const fr_frame_t *frame);
size_t fr_encode_record(const fr_frame_t *frame, uint8_t *output, size_t capacity);
const char *fr_decode_result_name(fr_decode_result_t result);

#endif
