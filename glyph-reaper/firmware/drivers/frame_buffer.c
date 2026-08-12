/**
 * @file frame_buffer.c
 * @brief Frame buffer management implementation
 *
 * Manages frame buffers in external PSRAM, including swap, clear,
 * copy, and pixel-level access utilities.
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "frame_buffer.h"
#include "psram_driver.h"
#include "registers.h"
#include <string.h>

void frame_buffer_init(void)
{
    /* Clear both frame buffers on startup */
    uint8_t zero_buf[256];
    memset(zero_buf, 0, sizeof(zero_buf));

    /* Clear current frame buffer */
    for (uint32_t offset = 0; offset < PSRAM_FRAME_CURRENT_SIZE; 
         offset += sizeof(zero_buf)) {
        uint32_t chunk = MIN(sizeof(zero_buf), PSRAM_FRAME_CURRENT_SIZE - offset);
        psram_write(PSRAM_BASE_ADDR + PSRAM_FRAME_CURRENT + offset, 
                    zero_buf, chunk);
    }

    /* Clear previous frame buffer */
    for (uint32_t offset = 0; offset < PSRAM_FRAME_PREVIOUS_SIZE;
         offset += sizeof(zero_buf)) {
        uint32_t chunk = MIN(sizeof(zero_buf), PSRAM_FRAME_PREVIOUS_SIZE - offset);
        psram_write(PSRAM_BASE_ADDR + PSRAM_FRAME_PREVIOUS + offset,
                    zero_buf, chunk);
    }
}

void frame_buffer_swap(void)
{
    /* Swap is handled by pointer swap in main.c
     * This function is a placeholder for any additional
     * swap-related bookkeeping */
}

void frame_buffer_clear(uint8_t *buffer, uint32_t size)
{
    if (buffer == NULL) return;
    memset(buffer, 0, size);
}

void frame_buffer_copy(const uint8_t *src, uint8_t *dst, uint32_t size)
{
    if (src == NULL || dst == NULL) return;
    memcpy(dst, src, size);
}

uint32_t frame_buffer_get_pixel(const uint8_t *buffer, uint16_t x, uint16_t y,
                                 uint16_t width, uint8_t bpp)
{
    if (buffer == NULL) return 0;

    uint32_t offset = (y * width + x) * bpp;
    uint32_t value = 0;

    for (uint8_t i = 0; i < bpp && offset + i < PSRAM_FRAME_CURRENT_SIZE; i++) {
        value |= (buffer[offset + i] << (i * 8));
    }

    return value;
}

void frame_buffer_set_pixel(uint8_t *buffer, uint16_t x, uint16_t y,
                            uint16_t width, uint8_t bpp, uint32_t value)
{
    if (buffer == NULL) return;

    uint32_t offset = (y * width + x) * bpp;

    for (uint8_t i = 0; i < bpp && offset + i < PSRAM_FRAME_CURRENT_SIZE; i++) {
        buffer[offset + i] = (uint8_t)(value >> (i * 8));
    }
}

uint32_t frame_buffer_rgb565_to_rgb888(uint16_t rgb565)
{
    uint8_t r = RGB565_TO_R(rgb565);
    uint8_t g = RGB565_TO_G(rgb565);
    uint8_t b = RGB565_TO_B(rgb565);

    /* Expand to full 8-bit range */
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);

    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

uint16_t frame_buffer_rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return RGB888_TO_RGB565(r, g, b);
}