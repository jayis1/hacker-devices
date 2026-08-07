/*
 * drivers/frame_capture.h — Frame Capture for Prism-Tap
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PRISM_TAP_FRAME_CAPTURE_H
#define PRISM_TAP_FRAME_CAPTURE_H

#include <stdint.h>
#include "board.h"

int  capture_init(void);
int  capture_start(const capture_config_t *cfg);
void capture_stop(void);
int  capture_is_active(void);
uint32_t capture_get_frame_count(void);
uint32_t capture_get_bytes_written(void);

/* Called from main loop to poll FPGA for completed frames */
void capture_poll(void);

/* PTF (Prism-Tap Frame) file header */
#define PTF_MAGIC "PRISMTAP-FRAME\0"
#define PTF_VERSION 1

typedef struct __attribute__((packed)) {
    char     magic[16];
    uint32_t version;
    uint32_t frame_index;
    uint32_t timestamp;
    uint16_t width;
    uint16_t height;
    uint32_t format;
    uint32_t frame_size;
    uint32_t crc32;
    uint32_t reserved;
} ptf_header_t;

void capture_build_header(ptf_header_t *hdr, uint32_t frame_idx,
                          uint32_t timestamp, uint16_t w, uint16_t h,
                          uint8_t fmt, uint32_t size);

uint32_t capture_crc32(const uint8_t *data, uint32_t len);

#endif /* PRISM_TAP_FRAME_CAPTURE_H */
/* Author: jayis1 */