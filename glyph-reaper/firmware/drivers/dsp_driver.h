/**
 * @file dsp_driver.h
 * @brief ESP32-S3 DSP co-processor driver
 *
 * Interface to the ESP32-S3 DSP that handles delta compression, OCR text
 * extraction, credential pattern matching, and QR/barcode decoding.
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef DSP_DRIVER_H
#define DSP_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"
#include "registers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Alert structure for credential detection results */
typedef struct {
    uint8_t pattern_type;       /* pattern_type_t enum */
    uint16_t x;                 /* X coordinate of detected text */
    uint16_t y;                 /* Y coordinate */
    uint16_t width;             /* Bounding box width */
    uint16_t height;            /* Bounding box height */
    uint8_t text[64];           /* Extracted text content */
    uint8_t confidence;         /* 0-100 confidence score */
} __attribute__((packed)) dsp_alert_t;

/**
 * @brief Initialize DSP driver and SPI interface
 */
void dsp_driver_init(void);

/**
 * @brief Wait for DSP to signal ready after reset
 * @param timeout_ms Maximum wait time in milliseconds
 * @return true if DSP became ready
 */
bool dsp_wait_ready(uint32_t timeout_ms);

/**
 * @brief Set DSP processing mode
 * @param mode DSP_MODE_* constant
 */
void dsp_set_mode(uint8_t mode);

/**
 * @brief Set DSP power mode
 * @param mode DSP_POWER_* constant
 */
void dsp_set_power_mode(uint8_t mode);

/**
 * @brief Set expected frame resolution
 */
void dsp_set_resolution(uint16_t width, uint16_t height);

/**
 * @brief Set color depth
 */
void dsp_set_color_depth(uint8_t depth);

/**
 * @brief Load OCR model from flash into DSP
 */
void dsp_load_ocr_model(void);

/**
 * @brief Load credential pattern definitions into DSP
 */
void dsp_load_patterns(void);

/**
 * @brief Start DSP processing pipeline
 */
void dsp_start_processing(void);

/**
 * @brief Stop DSP processing pipeline
 */
void dsp_stop_processing(void);

/**
 * @brief Write frame data to DSP for processing
 * @param frame_data Pixel data buffer
 * @param size Size in bytes
 */
void dsp_write_frame(const uint8_t *frame_data, uint32_t size);

/**
 * @brief Write reference (previous) frame for delta computation
 */
void dsp_write_reference_frame(const uint8_t *frame_data, uint32_t size);

/**
 * @brief Start delta compression with given threshold
 * @param threshold Pixel difference threshold (0-255)
 */
void dsp_start_compression(uint16_t threshold);

/**
 * @brief Start OCR text extraction
 */
void dsp_start_ocr(void);

/**
 * @brief Start credential pattern matching
 */
void dsp_start_pattern_matching(void);

/**
 * @brief Start QR/barcode decoding
 */
void dsp_start_qr_decode(void);

/**
 * @brief Check if all DSP processing is complete
 */
bool dsp_is_processing_complete(void);

/**
 * @brief Read compressed frame data from DSP
 * @param buffer Output buffer
 * @param max_size Maximum bytes to read
 * @return Number of bytes read
 */
uint32_t dsp_read_compressed(uint8_t *buffer, uint32_t max_size);

/**
 * @brief Read OCR text results from DSP
 * @param buffer Output buffer for text
 * @param max_size Maximum bytes to read
 * @return Number of bytes read
 */
uint16_t dsp_read_ocr_text(uint8_t *buffer, uint16_t max_size);

/**
 * @brief Get number of credential alerts detected
 */
uint16_t dsp_read_alerts(void);

/**
 * @brief Read alert data from DSP
 * @param buffer Output buffer for alert data
 * @param max_size Maximum bytes
 * @return Number of bytes read
 */
uint16_t dsp_read_alert_data(uint8_t *buffer, uint16_t max_size);

#ifdef __cplusplus
}
#endif

#endif /* DSP_DRIVER_H */