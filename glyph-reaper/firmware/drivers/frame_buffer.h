/**
 * @file frame_buffer.h
 * @brief Frame buffer management for PSRAM-resident frame storage
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize frame buffer manager
 */
void frame_buffer_init(void);

/**
 * @brief Swap current and previous frame buffers
 */
void frame_buffer_swap(void);

/**
 * @brief Clear a frame buffer (zero fill)
 * @param buffer PSRAM address of frame buffer
 * @param size Size in bytes
 */
void frame_buffer_clear(uint8_t *buffer, uint32_t size);

/**
 * @brief Copy frame data from one buffer to another
 */
void frame_buffer_copy(const uint8_t *src, uint8_t *dst, uint32_t size);

/**
 * @brief Get a pixel from a frame buffer
 * @param buffer Frame buffer
 * @param x X coordinate
 * @param y Y coordinate
 * @param width Frame width
 * @param bpp Bytes per pixel
 * @return Pixel value (up to 32 bits)
 */
uint32_t frame_buffer_get_pixel(const uint8_t *buffer, uint16_t x, uint16_t y,
                                 uint16_t width, uint8_t bpp);

/**
 * @brief Set a pixel in a frame buffer
 */
void frame_buffer_set_pixel(uint8_t *buffer, uint16_t x, uint16_t y,
                            uint16_t width, uint8_t bpp, uint32_t value);

/**
 * @brief Convert RGB565 to RGB888
 */
uint32_t frame_buffer_rgb565_to_rgb888(uint16_t rgb565);

/**
 * @brief Convert RGB888 to RGB565
 */
uint16_t frame_buffer_rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_BUFFER_H */