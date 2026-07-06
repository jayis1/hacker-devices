/*
 * lumen-tap/firmware/drivers/dsp.h
 * DSP pipeline: DC block, bandpass, FM demod, Wiener noise suppress, AGC,
 * decimation. CMSIS-DSP-style fixed/f32 implementation.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

#ifndef LUMEN_TAP_DSP_H
#define LUMEN_TAP_DSP_H

#include <stdint.h>

/* Block size used throughout the pipeline */
#ifndef DSP_BLOCK_SAMPLES
#define DSP_BLOCK_SAMPLES  128
#endif

/* Filter tap counts */
#define DSP_DC_TAPS         31
#define DSP_BANDPASS_TAPS   63
#define DSP_HILBERT_TAPS    64

/* Number of biquad cascade stages for the bandpass */
#define DSP_BIQUAD_STAGES   4

/* AGC parameters */
#define DSP_AGC_TARGET_RMS  (30000.0f)   /* target RMS in float samples */
#define DSP_AGC_MAX_GAIN    (1000.0f)    /* 60 dB */
#define DSP_AGC_MIN_GAIN    (0.001f)
#define DSP_AGC_ATTACK_S    (0.010f)     /* 10 ms */
#define DSP_AGC_RELEASE_S   (0.500f)     /* 500 ms */

/* Noise suppressor depth (0..1): 0=off, 1=max subtraction */
#define DSP_NOISE_DEPTH_DEFAULT  (0.6f)

/* Demodulation mode */
typedef enum {
    DSP_DEMOD_AM = 0,   /* envelope detection (direct detection) */
    DSP_DEMOD_FM = 1    /* quadrature FM (interferometric/coherent) */
} ltm_demod_mode_t;

/* Pipeline configuration (mutable from control plane) */
typedef struct {
    ltm_demod_mode_t demod_mode;
    float bp_low_hz;        /* bandpass low cutoff */
    float bp_high_hz;       /* bandpass high cutoff */
    float noise_depth;      /* 0..1 */
    float agc_target;       /* target RMS */
    uint8_t bypass;         /* if 1, output raw ADC (for debug) */
} ltm_dsp_config_t;

/* Pipeline persistent state */
typedef struct {
    /* DC blocker FIR state */
    float dc_state[DSP_DC_TAPS + DSP_BLOCK_SAMPLES];
    /* Bandpass biquad state (4 stages × 2 direct-form-II stages) */
    float bq_state[2 * DSP_BIQUAD_STAGES];
    /* Hilbert FIR state (for FM demod) */
    float hb_state[DSP_HILBERT_TAPS + DSP_BLOCK_SAMPLES];
    float hb_out[DSP_BLOCK_SAMPLES];
    float hb_delay[DSP_BLOCK_SAMPLES];  /* I path = delayed input */
    /* AGC state */
    float agc_gain;
    /* Noise estimator (running minima of PSD bins) */
    float noise_floor[64];
    /* Previous demod sample for atan2 unwrap */
    float prev_i, prev_q;
    /* Stats for host */
    float last_rms_in;
    float last_rms_out;
    float last_snr_db;
} ltm_dsp_state_t;

/* Initialize state and config defaults. */
void dsp_init(ltm_dsp_state_t *st, ltm_dsp_config_t *cfg);

/* Recompute bandpass biquad coefficients for new cutoffs. */
void dsp_set_bandpass(ltm_dsp_state_t *st, ltm_dsp_config_t *cfg,
                      float low_hz, float high_hz);

/* Process one block of 128 float samples in-place.
 * input  : raw ADC samples normalized to [-1, 1]
 * output : processed samples in [-1, 1], ready for 24-bit packing
 * Returns the post-DSP RMS (for AGC display). */
float dsp_process_block(ltm_dsp_state_t *st, ltm_dsp_config_t *cfg,
                        const float *input, float *output);

/* Convert int16 ADC block → float block (scale by 1/32768). */
void dsp_int16_to_float(const int16_t *in, float *out, int n);

/* Convert float block → 24-bit-in-32-bit packed samples for UAC2. */
void dsp_float_to_uac24(const float *in, uint8_t *out, int n);

/* Estimate SNR (dB) given current RMS and noise floor. */
float dsp_estimate_snr(const ltm_dsp_state_t *st);

#endif /* LUMEN_TAP_DSP_H */