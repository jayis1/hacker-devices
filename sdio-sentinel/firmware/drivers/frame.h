/* SD/SDIO frame analyzer API. Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SDIO_SENTINEL_FRAME_H
#define SDIO_SENTINEL_FRAME_H

#include "../board.h"

struct ss_frame {
    uint32_t timestamp_ms;
    uint32_t argument;
    uint16_t sequence;
    uint8_t command;
    uint8_t response_type;
    uint8_t data[SS_MAX_FRAME_BYTES];
    uint8_t data_length;
    enum ss_direction direction;
    bool crc_valid;
    bool timed_out;
};

struct ss_analysis {
    enum ss_severity severity;
    uint16_t score;
    uint32_t flags;
    char summary[96];
};

#define SS_FLAG_BAD_CRC            (1u << 0)
#define SS_FLAG_ILLEGAL_SEQUENCE   (1u << 1)
#define SS_FLAG_WRITE_BOOT_REGION  (1u << 2)
#define SS_FLAG_SDIO_FUNCTION_IO   (1u << 3)
#define SS_FLAG_VOLTAGE_SWITCH     (1u << 4)
#define SS_FLAG_CLOCK_EXCESS       (1u << 5)
#define SS_FLAG_TIMEOUT            (1u << 6)
#define SS_FLAG_RAPID_REINIT       (1u << 7)
#define SS_FLAG_LOCK_UNLOCK        (1u << 8)
#define SS_FLAG_VENDOR_COMMAND     (1u << 9)
#define SS_FLAG_OVERSIZED_FRAME    (1u << 10)
#define SS_FLAG_STATE_VIOLATION    (1u << 11)

struct ss_analyzer {
    enum {
        CARD_STATE_NO_MEDIA = 0,
        CARD_STATE_IDLE,
        CARD_STATE_READY,
        CARD_STATE_IDENT,
        CARD_STATE_STANDBY,
        CARD_STATE_TRANSFER,
        CARD_STATE_DATA,
        CARD_STATE_RECEIVE,
        CARD_STATE_PROGRAMMING,
        CARD_STATE_DISCONNECT
    } state;
    uint32_t command_count[64];
    uint32_t bad_crc_count;
    uint32_t timeout_count;
    uint32_t write_count;
    uint32_t read_count;
    uint32_t last_init_ms;
    uint32_t clock_hz;
    uint16_t next_sequence;
    bool high_capacity;
    bool sdio_present;
};

void ss_analyzer_init(struct ss_analyzer *analyzer);
uint8_t ss_crc7(const uint8_t *data, size_t length);
uint16_t ss_crc16(const uint8_t *data, size_t length);
bool ss_frame_decode_command(const uint8_t raw[6], uint32_t timestamp_ms,
                             struct ss_frame *frame);
void ss_analyze_frame(struct ss_analyzer *analyzer, const struct ss_frame *frame,
                      struct ss_analysis *analysis);
const char *ss_command_name(uint8_t command);
bool ss_frame_to_json(const struct ss_frame *frame, const struct ss_analysis *analysis,
                      char *output, size_t capacity);

#endif
