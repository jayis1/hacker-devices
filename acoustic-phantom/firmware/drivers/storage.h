/*
 * ACOUSTIC-PHANTOM — Storage driver
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Manages QSPI NOR flash (model storage, calibration data) and
 * microSD card (raw audio capture, exported datasets). Handles
 * optional AES-256-CTR encryption of stored captures.
 */
#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include "event_detector.h"
#include "tflm_inference.h"

#define MAX_FILES 256

typedef struct {
    char     name[32];
    uint32_t size;
    uint32_t timestamp;
    uint8_t  encrypted;
} file_entry_t;

void  storage_init(const char *passkey);
void  storage_enable(void);
void  storage_disable(void);
uint8_t storage_is_enabled(void);
void  storage_append_event(const event_t *event, const classification_t *result);
void  storage_wipe_buffers(void);

/* File operations */
uint16_t storage_list_files(file_entry_t *files, uint16_t max);
void  storage_stream_file(uint32_t file_idx);
uint8_t storage_is_streaming(void);
void  storage_stream_chunk(void);

/* Calibration persistence */
void  storage_save_calibration(uint8_t profile);
uint8_t storage_load_calibration(uint8_t profile);

/* Battery (from BQ27421 fuel gauge) */
uint8_t storage_get_battery_pct(void);

#endif /* STORAGE_H */