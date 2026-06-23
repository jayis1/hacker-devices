/*
 * ACOUSTIC-PHANTOM — DSP pipeline driver
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Real-time DSP pipeline: framing → windowing → FFT → power
 * spectrogram → feature extraction (MFCC for keyboard, spectral
 * envelope for HDD/printer, AM demodulation for SMPS).
 *
 * Uses CMSIS-DSP-style radix-2 FFT (arm_rfft_q15 equivalent) but
 * implemented inline to avoid external library dependency for this
 * standalone build.
 */
#ifndef DSP_PIPELINE_H
#define DSP_PIPELINE_H

#include <stdint.h>
#include "board.h"
#include "audio_capture.h"

/* Feature vector types — depends on attack profile */
typedef enum {
    FEAT_MFCC,         /* 39-dim: 13 MFCC + delta + delta-delta (keyboard) */
    FEAT_SPECTRAL_ENV, /* 32-dim: log-mel spectral envelope (HDD/printer) */
    FEAT_AM_DEMOD,     /* 16-dim: AM-demodulated envelope stats (SMPS) */
    FEAT_ONSET_SEQ     /* Variable: onset timing sequence (relay) */
} feature_type_t;

typedef struct {
    feature_type_t type;
    uint16_t       dim;
    float          data[64];     /* max feature dimension */
    uint32_t       frame_idx;
} feature_vector_t;

typedef struct {
    float power[FFT_SIZE / 2];     /* power spectrogram (257 bins) */
    float magnitude_db[FFT_SIZE / 2];
    uint32_t frame_idx;
} spectrogram_t;

void  dsp_pipeline_init(attack_profile_t profile);
void  dsp_pipeline_set_profile(attack_profile_t profile);
void  dsp_pipeline_process(audio_frame_t *frame);
void  dsp_pipeline_reset(void);

/* Get the feature vector from the most recent frame */
const feature_vector_t *dsp_pipeline_get_features(void);

/* Get the spectrogram from the most recent frame */
const spectrogram_t *dsp_pipeline_get_spectrogram(void);

/* FFT — radix-2 decimation-in-time, in-place, 512-point */
void  dsp_fft512(float *data_inout);   /* data = [re, im, re, im, ...] */

/* MFCC extraction — 13 coefficients from a power spectrum */
void  dsp_extract_mfcc(const float *power_spectrum, float *mfcc_out);

/* Spectral envelope — 32-band log-mel */
void  dsp_extract_spectral_env(const float *power_spectrum, float *env_out);

/* AM demodulation — envelope extraction for SMPS coil whine */
void  dsp_am_demod(const float *power_spectrum, float *env_out, uint16_t dim);

/* Hann window */
void  dsp_apply_hann_window(float *data, uint16_t n);

#endif /* DSP_PIPELINE_H */