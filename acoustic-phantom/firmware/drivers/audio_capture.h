/*
 * ACOUSTIC-PHANTOM — Audio capture driver
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Manages I²S DMA audio capture from the 4-element MEMS microphone
 * array (I2S2) and the WM8904 codec for the contact piezo channel
 * (I2S3). Provides double-buffered frame delivery to the DSP pipeline.
 */
#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <stdint.h>
#include "board.h"

/* Audio frame — one processing quantum (25 ms at 48 kHz = 1200 samples) */
typedef struct {
    int16_t  mic[NUM_MICS][FRAME_SAMPLES];  /* 4 MEMS mic channels */
    int16_t  piezo[FRAME_SAMPLES];          /* Contact piezo channel */
    int16_t  ambient[FRAME_SAMPLES];        /* WM8904 reference channel */
    uint32_t timestamp_ms;                  /* Capture timestamp */
    uint32_t seq;                           /* Sequence number */
} audio_frame_t;

void     audio_capture_init(void);
void     audio_capture_start(void);
void     audio_capture_stop(void);
uint8_t  audio_capture_get_frame(audio_frame_t *frame);
void     audio_capture_wipe(void);

/* DMA half/full transfer callbacks (called from ISR context) */
void     audio_capture_dma_half_cb(void);
void     audio_capture_dma_full_cb(void);

#endif /* AUDIO_CAPTURE_H */