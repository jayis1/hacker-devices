/*
 * drivers/frame_inject.h — Frame Injection for Prism-Tap
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PRISM_TAP_FRAME_INJECT_H
#define PRISM_TAP_FRAME_INJECT_H

#include <stdint.h>
#include "board.h"

int  inject_init(void);
int  inject_load_frame(const uint8_t *data, uint32_t length,
                       uint16_t width, uint16_t height, uint8_t format);
int  inject_start(const inject_config_t *cfg);
void inject_stop(void);
int  inject_is_active(void);
uint32_t inject_get_count(void);

/* Generate a solid-color test frame (for testing injection without a file) */
int inject_generate_test_frame(uint16_t width, uint16_t height, uint8_t format,
                                uint8_t r, uint8_t g, uint8_t b,
                                uint8_t *out_buf, uint32_t buf_size,
                                uint32_t *out_len);

/* Generate a pattern frame (gradient or noise) */
int inject_generate_pattern(uint16_t width, uint16_t height, uint8_t format,
                            uint8_t pattern_type,
                            uint8_t *out_buf, uint32_t buf_size,
                            uint32_t *out_len);

#define PATTERN_GRADIENT  0
#define PATTERN_NOISE     1
#define PATTERN_CHECKER   2
#define PATTERN_BARS      3

#endif /* PRISM_TAP_FRAME_INJECT_H */
/* Author: jayis1 */