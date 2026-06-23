/*
 * ACOUSTIC-PHANTOM — DSP pipeline implementation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Real-time DSP: framing → Hann window → 512-point FFT → power
 * spectrogram → feature extraction (MFCC / spectral envelope / AM
 * demodulation). Implements a self-contained radix-2 FFT (no external
 * CMSIS-DSP dependency for this standalone build) and a DCT-II for
 * MFCC computation.
 */

#include <math.h>
#include <string.h>
#include "dsp_pipeline.h"
#include "beamformer.h"

#define PI  3.14159265358979f

/* ---- State ------------------------------------------------------------- */
static attack_profile_t s_profile = PROFILE_KEYBOARD;
static feature_vector_t s_features;
static spectrogram_t    s_spectrogram;
static float s_fft_buf[FFT_SIZE * 2];   /* [re, im, re, im, ...] */
static float s_hann_window[FFT_SIZE];
static float s_power_spectrum[FFT_SIZE / 2];

/* Mel filter bank (26 filters, each covers a range of FFT bins) */
#define NUM_MEL_FILTERS  26
static float s_mel_filters[NUM_MEL_FILTERS][FFT_SIZE / 2];

/* ---- Precompute Hann window -------------------------------------------- */
static void init_hann_window(void)
{
    for (int i = 0; i < FFT_SIZE; i++) {
        s_hann_window[i] = 0.5f * (1.0f - cosf(2.0f * PI * i / (FFT_SIZE - 1)));
    }
}

/* ---- Precompute Mel filter bank ---------------------------------------- */
static float hz_to_mel(float hz)  { return 2595.0f * log10f(1.0f + hz / 700.0f); }
static float mel_to_hz(float mel) { return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f); }

static void init_mel_filters(void)
{
    float mel_min = hz_to_mel(0);
    float mel_max = hz_to_mel(AUDIO_SAMPLE_RATE / 2);
    float mel_step = (mel_max - mel_min) / (NUM_MEL_FILTERS + 1);

    for (int f = 0; f < NUM_MEL_FILTERS; f++) {
        float mel_left  = mel_min + f * mel_step;
        float mel_center = mel_left + mel_step;
        float mel_right = mel_left + 2.0f * mel_step;

        float hz_left  = mel_to_hz(mel_left);
        float hz_center = mel_to_hz(mel_center);
        float hz_right = mel_to_hz(mel_right);

        for (int k = 0; k < FFT_SIZE / 2; k++) {
            float freq = (float)k * AUDIO_SAMPLE_RATE / FFT_SIZE;
            if (freq < hz_left || freq > hz_right) {
                s_mel_filters[f][k] = 0.0f;
            } else if (freq <= hz_center) {
                s_mel_filters[f][k] = (freq - hz_left) / (hz_center - hz_left);
            } else {
                s_mel_filters[f][k] = (hz_right - freq) / (hz_right - hz_center);
            }
        }
    }
}

/* ---- Radix-2 DIT FFT (in-place, 512-point) ----------------------------- */
static void fft_radix2(float *data, int n)
{
    /* Bit-reversal permutation */
    int j = 0;
    for (int i = 1; i < n; i++) {
        int bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            float tmp_re = data[2 * i];
            float tmp_im = data[2 * i + 1];
            data[2 * i]     = data[2 * j];
            data[2 * i + 1] = data[2 * j + 1];
            data[2 * j]     = tmp_re;
            data[2 * j + 1] = tmp_im;
        }
    }

    /* Butterfly stages */
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * PI / len;
        float w_re = cosf(ang);
        float w_im = sinf(ang);

        for (int i = 0; i < n; i += len) {
            float cur_w_re = 1.0f;
            float cur_w_im = 0.0f;
            for (int k = 0; k < len / 2; k++) {
                int idx1 = i + k;
                int idx2 = i + k + len / 2;

                float t_re = cur_w_re * data[2 * idx2] - cur_w_im * data[2 * idx2 + 1];
                float t_im = cur_w_re * data[2 * idx2 + 1] + cur_w_im * data[2 * idx2];

                data[2 * idx2]     = data[2 * idx1] - t_re;
                data[2 * idx2 + 1] = data[2 * idx1 + 1] - t_im;
                data[2 * idx1]     = data[2 * idx1] + t_re;
                data[2 * idx1 + 1] = data[2 * idx1 + 1] + t_im;

                /* Rotate w */
                float new_w_re = cur_w_re * w_re - cur_w_im * w_im;
                cur_w_im = cur_w_re * w_im + cur_w_im * w_re;
                cur_w_re = new_w_re;
            }
        }
    }
}

/* ---- DCT-II for MFCC --------------------------------------------------- */
static void dct_ii(const float *input, float *output, int num_in, int num_out)
{
    for (int k = 0; k < num_out; k++) {
        float sum = 0.0f;
        for (int n = 0; n < num_in; n++) {
            sum += input[n] * cosf(PI * k * (2 * n + 1) / (2.0f * num_in));
        }
        output[k] = sum;
    }
}

/* ---- Public DSP functions ---------------------------------------------- */
void dsp_fft512(float *data_inout)
{
    fft_radix2(data_inout, FFT_SIZE);
}

void dsp_apply_hann_window(float *data, uint16_t n)
{
    for (uint16_t i = 0; i < n && i < FFT_SIZE; i++) {
        data[i] *= s_hann_window[i];
    }
}

void dsp_extract_mfcc(const float *power_spectrum, float *mfcc_out)
{
    /* Step 1: Apply Mel filter bank → log(mel energy) */
    float mel_energies[NUM_MEL_FILTERS];
    for (int f = 0; f < NUM_MEL_FILTERS; f++) {
        float energy = 0.0f;
        for (int k = 0; k < FFT_SIZE / 2; k++) {
            energy += power_spectrum[k] * s_mel_filters[f][k];
        }
        /* Log with floor to avoid log(0) */
        mel_energies[f] = log10f(energy + 1.0f);
    }

    /* Step 2: DCT-II → 13 MFCC coefficients */
    dct_ii(mel_energies, mfcc_out, NUM_MEL_FILTERS, MFCC_COEFFS);
}

void dsp_extract_spectral_env(const float *power_spectrum, float *env_out)
{
    /* 32-band log-mel spectral envelope for HDD/printer profiles */
    /* Use the first 32 Mel filters */
    for (int f = 0; f < 32; f++) {
        float energy = 0.0f;
        for (int k = 0; k < FFT_SIZE / 2; k++) {
            energy += power_spectrum[k] * s_mel_filters[f][k];
        }
        env_out[f] = log10f(energy + 1.0f);
    }
}

void dsp_am_demod(const float *power_spectrum, float *env_out, uint16_t dim)
{
    /* AM demodulation for SMPS coil whine:
     * Find the dominant carrier frequency in the 200-800 Hz range,
     * then compute the envelope by summing power in sidebands. */
    int carrier_bin = 0;
    float max_power = 0.0f;

    int bin_200 = 200 * FFT_SIZE / AUDIO_SAMPLE_RATE;
    int bin_800 = 800 * FFT_SIZE / AUDIO_SAMPLE_RATE;

    for (int k = bin_200; k <= bin_800 && k < FFT_SIZE / 2; k++) {
        if (power_spectrum[k] > max_power) {
            max_power = power_spectrum[k];
            carrier_bin = k;
        }
    }

    /* Extract envelope features: carrier power, sideband powers,
     * modulation depth estimate, temporal stats */
    if (dim > 16) dim = 16;
    memset(env_out, 0, dim * sizeof(float));

    env_out[0] = max_power;
    env_out[1] = (float)carrier_bin * AUDIO_SAMPLE_RATE / FFT_SIZE;  /* carrier freq */

    /* Sideband power (±5 bins around carrier) */
    float side_lo = 0.0f, side_hi = 0.0f;
    for (int k = 1; k <= 5; k++) {
        if (carrier_bin - k >= 0)
            side_lo += power_spectrum[carrier_bin - k];
        if (carrier_bin + k < FFT_SIZE / 2)
            side_hi += power_spectrum[carrier_bin + k];
    }
    env_out[2] = side_lo;
    env_out[3] = side_hi;

    /* Modulation depth estimate */
    float total_side = side_lo + side_hi;
    env_out[4] = max_power > 0 ? total_side / max_power : 0.0f;

    /* Fill remaining features with band powers */
    for (int k = 5; k < (int)dim; k++) {
        int bin = bin_200 + (bin_800 - bin_200) * k / dim;
        if (bin < FFT_SIZE / 2)
            env_out[k] = power_spectrum[bin];
    }
}

/* ---- Main pipeline processing ------------------------------------------ */
static void compute_power_spectrum(void)
{
    for (int k = 0; k < FFT_SIZE / 2; k++) {
        float re = s_fft_buf[2 * k];
        float im = s_fft_buf[2 * k + 1];
        s_power_spectrum[k] = re * re + im * im;
    }
}

void dsp_pipeline_init(attack_profile_t profile)
{
    s_profile = profile;
    init_hann_window();
    init_mel_filters();
    memset(&s_features, 0, sizeof(s_features));
    memset(&s_spectrogram, 0, sizeof(s_spectrogram));
}

void dsp_pipeline_set_profile(attack_profile_t profile)
{
    s_profile = profile;
}

void dsp_pipeline_reset(void)
{
    memset(&s_features, 0, sizeof(s_features));
    memset(&s_spectrogram, 0, sizeof(s_spectrogram));
    memset(s_fft_buf, 0, sizeof(s_fft_buf));
}

void dsp_pipeline_process(audio_frame_t *frame)
{
    /* Get beamformed output */
    int16_t beam[FRAME_SAMPLES];
    beamformer_get_output(beam, FRAME_SAMPLES);

    /* Frame the audio: take the first FFT_SIZE samples (1200 > 512)
     * from the current frame. For continuous analysis, we would
     * maintain a hop buffer — for simplicity here, we process one
     * FFT per frame. */
    memset(s_fft_buf, 0, sizeof(s_fft_buf));

    /* Choose input source based on profile */
    int16_t *src;
    if (s_profile == PROFILE_HDD || s_profile == PROFILE_SMPS) {
        src = frame->piezo;   /* Contact sensor for internal vibrations */
    } else {
        src = beam;            /* Beamformed MEMS for keyboard/printer/relay */
    }

    /* Fill FFT buffer with windowed real data */
    for (int i = 0; i < FFT_SIZE && i < FRAME_SAMPLES; i++) {
        s_fft_buf[2 * i] = (float)src[i] * s_hann_window[i];
        s_fft_buf[2 * i + 1] = 0.0f;  /* imaginary part = 0 (real input) */
    }

    /* Run FFT */
    fft_radix2(s_fft_buf, FFT_SIZE);

    /* Compute power spectrum */
    compute_power_spectrum();

    /* Store spectrogram */
    for (int k = 0; k < FFT_SIZE / 2; k++) {
        s_spectrogram.power[k] = s_power_spectrum[k];
        s_spectrogram.magnitude_db[k] =
            10.0f * log10f(s_power_spectrum[k] + 1e-10f);
    }
    s_spectrogram.frame_idx++;

    /* Extract features based on profile */
    s_features.frame_idx = s_spectrogram.frame_idx;
    switch (s_profile) {
        case PROFILE_KEYBOARD:
            s_features.type = FEAT_MFCC;
            s_features.dim = MFCC_FEATURES;
            dsp_extract_mfcc(s_power_spectrum, s_features.data);
            /* Delta and delta-delta would be computed from previous frames
             * in a full implementation. Here we compute static MFCC only
             * and leave deltas as zeros (the model can still classify
             * with reasonable accuracy using static features). */
            /* Copy MFCC to delta positions (placeholder — real delta
             * computation requires frame history) */
            for (int i = 0; i < MFCC_COEFFS; i++) {
                s_features.data[MFCC_COEFFS + i] = 0.0f;       /* delta */
                s_features.data[2 * MFCC_COEFFS + i] = 0.0f;   /* delta-delta */
            }
            break;

        case PROFILE_HDD:
        case PROFILE_PRINTER:
            s_features.type = FEAT_SPECTRAL_ENV;
            s_features.dim = 32;
            dsp_extract_spectral_env(s_power_spectrum, s_features.data);
            break;

        case PROFILE_SMPS:
            s_features.type = FEAT_AM_DEMOD;
            s_features.dim = 16;
            dsp_am_demod(s_power_spectrum, s_features.data, 16);
            break;

        case PROFILE_RELAY:
            /* Relay profile uses onset timing, not spectral features.
             * The event detector handles this directly. */
            s_features.type = FEAT_ONSET_SEQ;
            s_features.dim = 0;
            break;

        default:
            break;
    }
}

const feature_vector_t *dsp_pipeline_get_features(void)
{
    return &s_features;
}

const spectrogram_t *dsp_pipeline_get_spectrogram(void)
{
    return &s_spectrogram;
}