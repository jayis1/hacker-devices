/*
 * adc_capture.h — ADC waveform capture driver header
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef ADC_CAPTURE_H
#define ADC_CAPTURE_H

#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

void adc_capture_init(void);
void adc_capture_arm(uint16_t *buf, uint16_t *len, uint16_t max_samples);
void adc_capture_read(uint16_t *buf, uint16_t *len);
void adc_capture_dma_isr(void);

/* Glitch quality classification */
typedef enum {
    GLITCH_QUALITY_CLEAN = 0,
    GLITCH_QUALITY_OVERSHOOT,
    GLITCH_QUALITY_UNDERDEPTH,
    GLITCH_QUALITY_INVALID,
    GLITCH_QUALITY_NO_GLITCH
} glitch_quality_t;

glitch_quality_t adc_capture_classify(const uint16_t *waveform,
                                      uint16_t len,
                                      uint16_t expected_depth_mv);

#endif /* ADC_CAPTURE_H */