/* RFFE frame decoder for RFFE Sentinel
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef RFFE_SENTINEL_RFFE_H
#define RFFE_SENTINEL_RFFE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RFFE_MAX_DATA 16u

enum rffe_command_class {
    RFFE_CMD_UNKNOWN = 0,
    RFFE_CMD_REG_WRITE,
    RFFE_CMD_REG_READ,
    RFFE_CMD_EXT_WRITE,
    RFFE_CMD_EXT_READ,
    RFFE_CMD_MASTER_CONTEXT,
    RFFE_CMD_OWNERSHIP_HANDOVER
};

struct rffe_frame {
    uint64_t timestamp_us;
    uint32_t sequence;
    uint16_t register_address;
    uint8_t usid;
    uint8_t command_raw;
    enum rffe_command_class command_class;
    uint8_t data[RFFE_MAX_DATA];
    uint8_t data_length;
    uint8_t flags;
    uint8_t raw_bit_count;
    uint64_t raw_bits;
};

struct rffe_decoder {
    uint64_t shift;
    uint8_t bit_count;
    uint8_t expected_bits;
    uint32_t next_sequence;
    bool in_frame;
};

void rffe_decoder_init(struct rffe_decoder *decoder);
bool rffe_decoder_push_bit(struct rffe_decoder *decoder, bool bit,
                           uint64_t timestamp_us, struct rffe_frame *out);
bool rffe_decode_word(uint64_t raw, uint8_t bits, uint64_t timestamp_us,
                      uint32_t sequence, struct rffe_frame *out);
uint8_t rffe_odd_parity(uint64_t value, uint8_t bits);
const char *rffe_class_name(enum rffe_command_class command_class);

#endif
