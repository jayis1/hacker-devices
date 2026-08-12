/**
 * @file compression_engine.c
 * @brief Delta compression engine implementation
 *
 * Block-based delta detection (8x8 pixel blocks) and RLE compression
 * for reducing frame exfiltration bandwidth.
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "compression_engine.h"
#include "board.h"
#include "registers.h"
#include <string.h>

void compression_engine_init(void)
{
    /* Nothing to initialize — stateless compression functions */
}

uint32_t compression_compute_delta(const uint8_t *current,
                                    const uint8_t *previous,
                                    uint16_t width, uint16_t height,
                                    uint8_t bpp, uint16_t threshold,
                                    uint8_t *delta_map,
                                    uint16_t *blocks_x, uint16_t *blocks_y)
{
    if (current == NULL || previous == NULL || delta_map == NULL) {
        return 0;
    }

    /* Calculate number of 8x8 blocks */
    uint16_t bx = (width + 7) / 8;
    uint16_t by = (height + 7) / 8;
    *blocks_x = bx;
    *blocks_y = by;

    uint32_t changed_blocks = 0;
    uint32_t map_byte_idx = 0;
    uint8_t map_bit_idx = 0;

    /* Clear delta map */
    uint32_t map_size = (bx * by + 7) / 8;
    memset(delta_map, 0, map_size);

    /* Compare each 8x8 block */
    for (uint16_t by_idx = 0; by_idx < by; by_idx++) {
        for (uint16_t bx_idx = 0; bx_idx < bx; bx_idx++) {
            bool changed = false;
            uint16_t block_w = MIN(8, width - bx_idx * 8);
            uint16_t block_h = MIN(8, height - by_idx * 8);

            /* Compare pixels in this block */
            for (uint16_t py = 0; py < block_h && !changed; py++) {
                for (uint16_t px = 0; px < block_w && !changed; px++) {
                    uint32_t pixel_offset = ((by_idx * 8 + py) * width + 
                                             (bx_idx * 8 + px)) * bpp;

                    /* Compare bytes for this pixel */
                    for (uint8_t b = 0; b < bpp; b++) {
                        int16_t diff = (int16_t)current[pixel_offset + b] -
                                       (int16_t)previous[pixel_offset + b];
                        if (diff < 0) diff = -diff;
                        if (diff > threshold) {
                            changed = true;
                            break;
                        }
                    }
                }
            }

            if (changed) {
                delta_map[map_byte_idx] |= (1 << map_bit_idx);
                changed_blocks++;
            }

            map_bit_idx++;
            if (map_bit_idx >= 8) {
                map_bit_idx = 0;
                map_byte_idx++;
            }
        }
    }

    return changed_blocks;
}

uint32_t compression_rle(const uint8_t *input, uint32_t input_len,
                         uint8_t *output, uint32_t output_max)
{
    if (input == NULL || output == NULL || input_len == 0) {
        return 0;
    }

    uint32_t in_idx = 0;
    uint32_t out_idx = 0;

    while (in_idx < input_len && out_idx < output_max) {
        uint8_t current = input[in_idx];
        uint32_t run_len = 1;

        /* Count run length (max 127 for single-byte encoding) */
        while (in_idx + run_len < input_len &&
               input[in_idx + run_len] == current &&
               run_len < 127) {
            run_len++;
        }

        if (run_len >= COMPRESSION_RLE_THRESHOLD) {
            /* Encode as: [0x80 | run_len][value] */
            if (out_idx + 2 > output_max) break;
            output[out_idx++] = (uint8_t)(0x80 | run_len);
            output[out_idx++] = current;
        } else {
            /* Encode as literal bytes */
            if (out_idx + run_len > output_max) break;
            for (uint32_t i = 0; i < run_len; i++) {
                output[out_idx++] = current;
            }
        }

        in_idx += run_len;
    }

    /* Only use compressed version if it's smaller */
    if (out_idx >= input_len) {
        memcpy(output, input, input_len);
        return input_len;
    }

    return out_idx;
}

uint32_t compression_rle_decompress(const uint8_t *input, uint32_t input_len,
                                    uint8_t *output, uint32_t output_max)
{
    if (input == NULL || output == NULL || input_len == 0) {
        return 0;
    }

    uint32_t in_idx = 0;
    uint32_t out_idx = 0;

    while (in_idx < input_len && out_idx < output_max) {
        uint8_t byte = input[in_idx++];

        if (byte & 0x80) {
            /* RLE encoded: run length + value */
            uint32_t run_len = byte & 0x7F;
            if (in_idx >= input_len) break;
            uint8_t value = input[in_idx++];

            for (uint32_t i = 0; i < run_len && out_idx < output_max; i++) {
                output[out_idx++] = value;
            }
        } else {
            /* Literal byte */
            output[out_idx++] = byte;
        }
    }

    return out_idx;
}