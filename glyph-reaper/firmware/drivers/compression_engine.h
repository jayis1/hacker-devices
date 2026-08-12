/**
 * @file compression_engine.h
 * @brief Delta compression engine for frame data
 *
 * Implements block-based delta frame detection and RLE compression
 * to reduce bandwidth for frame exfiltration over BLE.
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef COMPRESSION_ENGINE_H
#define COMPRESSION_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the compression engine
 */
void compression_engine_init(void);

/**
 * @brief Compute delta between two frames using 8x8 block comparison
 * @param current Current frame buffer
 * @param previous Previous frame buffer
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @param bpp Bytes per pixel
 * @param threshold Pixel difference threshold
 * @param delta_map Output bitfield (1 bit per 8x8 block, 1=changed)
 * @param blocks_x Output: number of blocks in X direction
 * @param blocks_y Output: number of blocks in Y direction
 * @return Number of changed blocks
 */
uint32_t compression_compute_delta(const uint8_t *current, 
                                    const uint8_t *previous,
                                    uint16_t width, uint16_t height,
                                    uint8_t bpp, uint16_t threshold,
                                    uint8_t *delta_map,
                                    uint16_t *blocks_x, uint16_t *blocks_y);

/**
 * @brief RLE compress a data buffer
 * @param input Input data
 * @param input_len Input length
 * @param output Output buffer
 * @param output_max Maximum output size
 * @return Compressed size, or input_len if compression didn't help
 */
uint32_t compression_rle(const uint8_t *input, uint32_t input_len,
                         uint8_t *output, uint32_t output_max);

/**
 * @brief RLE decompress data
 * @param input Compressed data
 * @param input_len Compressed length
 * @param output Output buffer
 * @param output_max Maximum output size
 * @return Decompressed size, 0 on error
 */
uint32_t compression_rle_decompress(const uint8_t *input, uint32_t input_len,
                                    uint8_t *output, uint32_t output_max);

#ifdef __cplusplus
}
#endif

#endif /* COMPRESSION_ENGINE_H */