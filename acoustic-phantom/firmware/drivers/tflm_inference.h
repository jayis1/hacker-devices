/*
 * ACOUSTIC-PHANTOM — TFLM inference driver
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * TensorFlow Lite for Microcontrollers inference engine.
 * Loads quantized int8 models from QSPI NOR flash (XIP) and runs
 * classification on detected acoustic events. Also handles the
 * per-keyboard calibration step (online linear classifier training).
 */
#ifndef TFLM_INFERENCE_H
#define TFLM_INFERENCE_H

#include <stdint.h>
#include "board.h"
#include "event_detector.h"

/* Classification result */
typedef struct {
    uint8_t   class_id;        /* predicted class (key ID, seek bucket, etc.) */
    float     confidence;      /* softmax confidence [0..1] */
    float     top5_conf[5];    /* top-5 confidences */
    uint8_t   top5_id[5];      /* top-5 class IDs */
    uint32_t  timestamp_ms;    /* event timestamp relative to capture start */
    uint16_t  inter_event_ms;  /* time since previous event */
} classification_t;

void  tflm_inference_init(attack_profile_t profile);
void  tflm_inference_set_profile(attack_profile_t profile);
void  tflm_inference_classify(const event_t *event, classification_t *result);
void  tflm_inference_reset(void);

/* Calibration (keyboard profile) */
void  tflm_inference_start_calibration(void);
void  tflm_inference_add_calibration_sample(const uint8_t *data, uint16_t len);
void  tflm_inference_finish_calibration(void);

/* Get label string for a class ID */
const char *tflm_inference_get_label(uint8_t class_id);

#endif /* TFLM_INFERENCE_H */