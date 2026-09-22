/* RFFE Sentinel frame decoder
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "rffe.h"

#include <string.h>

static enum rffe_command_class classify(uint8_t command)
{
    switch ((command >> 1u) & 0x07u) {
    case 0u:
        return RFFE_CMD_REG_WRITE;
    case 1u:
        return RFFE_CMD_REG_READ;
    case 2u:
    case 3u:
        return RFFE_CMD_EXT_WRITE;
    case 4u:
    case 5u:
        return RFFE_CMD_EXT_READ;
    case 6u:
        return RFFE_CMD_MASTER_CONTEXT;
    case 7u:
        return RFFE_CMD_OWNERSHIP_HANDOVER;
    default:
        return RFFE_CMD_UNKNOWN;
    }
}

uint8_t rffe_odd_parity(uint64_t value, uint8_t bits)
{
    uint8_t parity = 1u;
    uint8_t index;

    for (index = 0u; index < bits; ++index) {
        parity ^= (uint8_t)((value >> index) & 1u);
    }
    return parity;
}

void rffe_decoder_init(struct rffe_decoder *decoder)
{
    if (decoder == NULL) {
        return;
    }
    memset(decoder, 0, sizeof(*decoder));
    decoder->expected_bits = 25u;
    decoder->next_sequence = 1u;
}

bool rffe_decode_word(uint64_t raw, uint8_t bits, uint64_t timestamp_us,
                      uint32_t sequence, struct rffe_frame *out)
{
    uint64_t body;
    uint8_t parity_bit;

    if ((out == NULL) || (bits != 25u)) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    parity_bit = (uint8_t)(raw & 1u);
    body = raw >> 1u;
    out->timestamp_us = timestamp_us;
    out->sequence = sequence;
    out->usid = (uint8_t)((body >> 20u) & 0x0fu);
    out->command_raw = (uint8_t)((body >> 16u) & 0x0fu);
    out->register_address = (uint16_t)((body >> 8u) & 0xffu);
    out->data[0] = (uint8_t)(body & 0xffu);
    out->data_length = 1u;
    out->command_class = classify(out->command_raw);
    out->raw_bit_count = bits;
    out->raw_bits = raw;

    if (rffe_odd_parity(body, 24u) == parity_bit) {
        out->flags |= 1u;
    }
    out->flags |= 2u;
    if ((out->command_class == RFFE_CMD_REG_READ) ||
        (out->command_class == RFFE_CMD_EXT_READ)) {
        out->flags |= 16u;
    }
    if ((out->command_class == RFFE_CMD_EXT_READ) ||
        (out->command_class == RFFE_CMD_EXT_WRITE)) {
        out->flags |= 32u;
    }
    return true;
}

bool rffe_decoder_push_bit(struct rffe_decoder *decoder, bool bit,
                           uint64_t timestamp_us, struct rffe_frame *out)
{
    bool complete;

    if ((decoder == NULL) || (out == NULL)) {
        return false;
    }
    if (!decoder->in_frame) {
        decoder->in_frame = true;
        decoder->shift = 0u;
        decoder->bit_count = 0u;
    }
    if (decoder->bit_count >= 63u) {
        rffe_decoder_init(decoder);
        return false;
    }

    decoder->shift = (decoder->shift << 1u) | (bit ? 1u : 0u);
    ++decoder->bit_count;
    if (decoder->bit_count < decoder->expected_bits) {
        return false;
    }

    complete = rffe_decode_word(decoder->shift, decoder->bit_count,
                                timestamp_us, decoder->next_sequence, out);
    ++decoder->next_sequence;
    decoder->shift = 0u;
    decoder->bit_count = 0u;
    decoder->in_frame = false;
    return complete;
}

const char *rffe_class_name(enum rffe_command_class command_class)
{
    switch (command_class) {
    case RFFE_CMD_REG_WRITE:
        return "register-write";
    case RFFE_CMD_REG_READ:
        return "register-read";
    case RFFE_CMD_EXT_WRITE:
        return "extended-write";
    case RFFE_CMD_EXT_READ:
        return "extended-read";
    case RFFE_CMD_MASTER_CONTEXT:
        return "master-context";
    case RFFE_CMD_OWNERSHIP_HANDOVER:
        return "ownership-handover";
    case RFFE_CMD_UNKNOWN:
    default:
        return "unknown";
    }
}
