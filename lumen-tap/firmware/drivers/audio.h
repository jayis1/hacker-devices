/*
 * lumen-tap/firmware/drivers/audio.h
 * ADC1 + DMA double-buffer capture, USB UAC2 audio streaming, and the
 * CDC control plane for LumenTap.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

#ifndef LUMEN_TAP_AUDIO_H
#define LUMEN_TAP_AUDIO_H

#include <stdint.h>
#include "dsp.h"

/* Number of DMA half-buffers (double-buffer) */
#define AUDIO_NBUFFERS  2

/* Total samples per buffer (per half) */
#define AUDIO_BUF_SAMPLES  DSP_BLOCK_SAMPLES

/* USB audio feedback value (16.16 fixed) for 48k @ 1ms */
#define AUDIO_FB_48K       (0x0000BB80UL)   /* 48000 in 16.16 ≈ 0xBB80 */

/* Initialize ADC1 + DMA1_Stream0 + USB OTG-HS (FS mode). */
void audio_init(void);

/* Start continuous ADC capture. */
void audio_start_capture(void);

/* Stop capture. */
void audio_stop_capture(void);

/* Pull a processed block (post-DSP, decimated to 48k) into the USB
 * audio IN FIFO. Called from main loop. Returns 1 if a new block was
 * pushed, 0 if none ready. */
int audio_pump_usb(void);

/* DMA half-transfer / full-transfer ISR. Does the DSP work inline. */
void audio_dma_isr(void);

/* Handle a USB setup request (standard + class). Returns 1 if handled. */
int usb_handle_setup(const uint8_t *setup);

/* Push a CDC status JSON line to the host (non-blocking). */
void cdc_send_status(const char *json, int len);

/* Read pending CDC command bytes (non-blocking). Returns bytes read. */
int cdc_read(uint8_t *buf, int maxlen);

#endif /* LUMEN_TAP_AUDIO_H */