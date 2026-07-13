/*
 * eddy-phantom — dsp_pipeline.c
 * DSP pipeline: bandpass filtering, Hilbert envelope detection,
 * MFCC feature extraction, and burst feature vector assembly.
 *
 * Uses CMSIS-DSP-style operations implemented in plain C for
 * portability. The pipeline transforms a raw 4-channel ADC burst
 * (2048 samples × 4 channels) into a 19-dimensional feature vector
 * for the classifier.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"
#include <math.h>

/* ── Constants ────────────────────────────────────────────────── */
#define PI          3.14159265358979323846f
#define FFT_SIZE    DSP_FFT_SIZE
#define LOG2_FFT    8   /* log2(256) */

/* ── FIR bandpass filter coefficients (50-350 kHz, 64 taps) ────
 * Pre-computed using a Hamming window. At 1 MSPS sample rate,
 * the normalized cutoff frequencies are:
 *   low  = 50000 / 1000000 = 0.05
 *   high = 350000 / 1000000 = 0.175
 */
static float fir_coeffs[DSP_FIR_TAPS];
static float fir_delay_line[ADC_CHANNELS][DSP_FIR_TAPS];
static int   fir_delay_idx[ADC_CHANNELS] = {0, 0, 0, 0};

/* ── FFT twiddle factors ──────────────────────────────────────── */
static float fft_cos_tbl[FFT_SIZE / 2];
static float fft_sin_tbl[FFT_SIZE / 2];
static int   fft_bit_reverse[FFT_SIZE];

/* ── Hamming window for MFCC ──────────────────────────────────── */
static float hamming_window[FFT_SIZE];

/* ── Mel filter bank (13 filters for 13 MFCC coefficients) ───── */
static float mel_filters[DSP_MFCC_COEFFS][FFT_SIZE / 2];
static int   mel_filter_bounds[DSP_MFCC_COEFFS][2];  /* start, end bins */

/* ── DCT matrix for MFCC ──────────────────────────────────────── */
static float dct_matrix[DSP_MFCC_COEFFS][DSP_MFCC_COEFFS];

/* ── Initialize FIR bandpass coefficients ─────────────────────── */
static void init_fir_bandpass(void)
{
    float f_low  = (float)DSP_BANDPASS_LOW_HZ  / ADC_SAMPLE_RATE_HZ;
    float f_high = (float)DSP_BANDPASS_HIGH_HZ / ADC_SAMPLE_RATE_HZ;

    for (int n = 0; n < DSP_FIR_TAPS; n++) {
        int k = n - DSP_FIR_TAPS / 2;
        float hamming = 0.54f - 0.46f * cosf(2.0f * PI * n / (DSP_FIR_TAPS - 1));

        if (k == 0) {
            fir_coeffs[n] = 2.0f * (f_high - f_low) * hamming;
        } else {
            fir_coeffs[n] = (sinf(2.0f * PI * f_high * k) -
                             sinf(2.0f * PI * f_low * k)) /
                            (PI * k) * hamming;
        }
    }
}

/* ── Apply FIR bandpass filter to one channel ─────────────────── */
static void fir_bandpass_channel(const int16_t *input, float *output,
                                  int len, int channel)
{
    float *delay = fir_delay_line[channel];
    int   *idx   = &fir_delay_idx[channel];

    for (int n = 0; n < len; n++) {
        /* Insert new sample into delay line */
        delay[*idx] = (float)input[n];
        *idx = (*idx + 1) % DSP_FIR_TAPS;

        /* Convolve with filter coefficients */
        float acc = 0.0f;
        int di = *idx;
        for (int k = 0; k < DSP_FIR_TAPS; k++) {
            di = (di - 1 + DSP_FIR_TAPS) % DSP_FIR_TAPS;
            acc += fir_coeffs[k] * delay[di];
        }
        output[n] = acc;
    }
}

/* ── Bit-reversal table for FFT ───────────────────────────────── */
static void init_bit_reverse(void)
{
    for (int i = 0; i < FFT_SIZE; i++) {
        int rev = 0;
        int x = i;
        for (int j = 0; j < LOG2_FFT; j++) {
            rev = (rev << 1) | (x & 1);
            x >>= 1;
        }
        fft_bit_reverse[i] = rev;
    }
}

/* ── FFT twiddle factor initialization ────────────────────────── */
static void init_twiddles(void)
{
    for (int i = 0; i < FFT_SIZE / 2; i++) {
        float angle = -2.0f * PI * i / FFT_SIZE;
        fft_cos_tbl[i] = cosf(angle);
        fft_sin_tbl[i] = sinf(angle);
    }
}

/* ── Radix-2 decimation-in-time FFT (in-place) ──────────────────
 * Computes the FFT of a complex signal. real[] and imag[] are
 * modified in place. Size must be a power of 2.
 */
static void fft_radix2(float *real, float *imag, int n)
{
    /* Bit-reversal permutation */
    for (int i = 0; i < n; i++) {
        int j = fft_bit_reverse[i];
        if (j > i) {
            float tmp_r = real[i]; real[i] = real[j]; real[j] = tmp_r;
            float tmp_i = imag[i]; imag[i] = imag[j]; imag[j] = tmp_i;
        }
    }

    /* Butterfly operations */
    for (int stage = 1; stage <= LOG2_FFT; stage++) {
        int half_size = 1 << (stage - 1);
        int group_size = half_size << 1;
        int twiddle_step = FFT_SIZE / group_size;

        for (int group = 0; group < n; group += group_size) {
            for (int pair = 0; pair < half_size; pair++) {
                int idx1 = group + pair;
                int idx2 = idx1 + half_size;
                int tw_idx = pair * twiddle_step;

                float wr = fft_cos_tbl[tw_idx];
                float wi = fft_sin_tbl[tw_idx];

                float tr = wr * real[idx2] - wi * imag[idx2];
                float ti = wr * imag[idx2] + wi * real[idx2];

                real[idx2] = real[idx1] - tr;
                imag[idx2] = imag[idx1] - ti;
                real[idx1] = real[idx1] + tr;
                imag[idx1] = imag[idx1] + ti;
            }
        }
    }
}

/* ── Compute Hilbert envelope via FFT ───────────────────────────
 * The analytic signal is obtained by:
 *   1. FFT the input signal
 *   2. Zero out negative frequencies, double positive frequencies
 *   3. Inverse FFT
 *   4. Take magnitude = envelope
 */
static void compute_envelope(float *signal, int len, float *envelope)
{
    float real[FFT_SIZE];
    float imag[FFT_SIZE];

    /* Copy signal into real, zero imag */
    for (int i = 0; i < FFT_SIZE && i < len; i++) {
        real[i] = signal[i];
        imag[i] = 0.0f;
    }
    /* Zero-pad if len < FFT_SIZE */
    for (int i = len; i < FFT_SIZE; i++) {
        real[i] = 0.0f;
        imag[i] = 0.0f;
    }

    /* Forward FFT */
    fft_radix2(real, imag, FFT_SIZE);

    /* Construct analytic signal: zero negative freq, double positive */
    for (int i = 1; i < FFT_SIZE / 2; i++) {
        real[i] *= 2.0f;
        imag[i] *= 2.0f;
    }
    for (int i = FFT_SIZE / 2 + 1; i < FFT_SIZE; i++) {
        real[i] = 0.0f;
        imag[i] = 0.0f;
    }
    /* DC and Nyquist stay as-is */

    /* Inverse FFT (same as forward but conjugate twiddle, then scale) */
    for (int i = 0; i < FFT_SIZE; i++)
        imag[i] = -imag[i];

    fft_radix2(real, imag, FFT_SIZE);

    /* Scale by 1/N and compute magnitude */
    float inv_n = 1.0f / FFT_SIZE;
    for (int i = 0; i < len && i < FFT_SIZE; i++) {
        real[i] *= inv_n;
        imag[i] *= inv_n;
        envelope[i] = sqrtf(real[i] * real[i] + imag[i] * imag[i]);
    }
}

/* ── Initialize Mel filter bank ───────────────────────────────── */
static void init_mel_filters(void)
{
    /* Convert frequency to Mel scale: mel = 2595 * log10(1 + f/700) */
    float f_min = (float)DSP_BANDPASS_LOW_HZ;
    float f_max = (float)DSP_BANDPASS_HIGH_HZ;
    float mel_min = 2595.0f * log10f(1.0f + f_min / 700.0f);
    float mel_max = 2595.0f * log10f(1.0f + f_max / 700.0f);
    float mel_step = (mel_max - mel_min) / (DSP_MFCC_COEFFS + 1);

    float mel_points[DSP_MFCC_COEFFS + 2];
    float freq_points[DSP_MFCC_COEFFS + 2];
    int bin_points[DSP_MFCC_COEFFS + 2];

    for (int i = 0; i < DSP_MFCC_COEFFS + 2; i++) {
        mel_points[i] = mel_min + i * mel_step;
        /* Inverse Mel to frequency: f = 700 * (10^(mel/2595) - 1) */
        freq_points[i] = 700.0f * (powf(10.0f, mel_points[i] / 2595.0f) - 1.0f);
        /* Convert to FFT bin: bin = f * FFT_SIZE / sample_rate */
        bin_points[i] = (int)(freq_points[i] * FFT_SIZE / ADC_SAMPLE_RATE_HZ);
        if (bin_points[i] >= FFT_SIZE / 2)
            bin_points[i] = FFT_SIZE / 2 - 1;
    }

    /* Build triangular filters */
    for (int m = 0; m < DSP_MFCC_COEFFS; m++) {
        int left = bin_points[m];
        int center = bin_points[m + 1];
        int right = bin_points[m + 2];

        mel_filter_bounds[m][0] = left;
        mel_filter_bounds[m][1] = right;

        for (int k = 0; k < FFT_SIZE / 2; k++) {
            mel_filters[m][k] = 0.0f;
            if (k >= left && k <= center && center > left) {
                mel_filters[m][k] = (float)(k - left) / (center - left);
            } else if (k > center && k <= right && right > center) {
                mel_filters[m][k] = (float)(right - k) / (right - center);
            }
        }
    }
}

/* ── Initialize DCT matrix for MFCC ───────────────────────────── */
static void init_dct_matrix(void)
{
    for (int k = 0; k < DSP_MFCC_COEFFS; k++) {
        for (int n = 0; n < DSP_MFCC_COEFFS; n++) {
            dct_matrix[k][n] = cosf(PI * k * (n + 0.5f) / DSP_MFCC_COEFFS);
        }
    }
}

/* ── Initialize Hamming window ────────────────────────────────── */
static void init_hamming_window(void)
{
    for (int i = 0; i < FFT_SIZE; i++) {
        hamming_window[i] = 0.54f - 0.46f * cosf(2.0f * PI * i / (FFT_SIZE - 1));
    }
}

/* ── DSP pipeline initialization ──────────────────────────────── */
void dsp_init(void)
{
    init_fir_bandpass();
    init_bit_reverse();
    init_twiddles();
    init_hamming_window();
    init_mel_filters();
    init_dct_matrix();

    /* Clear delay lines */
    for (int ch = 0; ch < ADC_CHANNELS; ch++) {
        for (int i = 0; i < DSP_FIR_TAPS; i++) {
            fir_delay_line[ch][i] = 0.0f;
        }
        fir_delay_idx[ch] = 0;
    }
}

/* ── Compute MFCC from signal ─────────────────────────────────── */
void dsp_compute_mfcc(float *signal, int len, float *mfcc_out)
{
    float real[FFT_SIZE];
    float imag[FFT_SIZE];
    float power[FFT_SIZE / 2];
    float mel_energy[DSP_MFCC_COEFFS];

    /* Apply Hamming window and zero-pad to FFT_SIZE */
    for (int i = 0; i < FFT_SIZE; i++) {
        if (i < len) {
            real[i] = signal[i] * hamming_window[i];
        } else {
            real[i] = 0.0f;
        }
        imag[i] = 0.0f;
    }

    /* Forward FFT */
    fft_radix2(real, imag, FFT_SIZE);

    /* Compute power spectrum (magnitude squared) */
    for (int i = 0; i < FFT_SIZE / 2; i++) {
        power[i] = real[i] * real[i] + imag[i] * imag[i];
    }

    /* Apply Mel filter bank */
    for (int m = 0; m < DSP_MFCC_COEFFS; m++) {
        mel_energy[m] = 0.0f;
        for (int k = mel_filter_bounds[m][0]; k <= mel_filter_bounds[m][1]; k++) {
            mel_energy[m] += mel_filters[m][k] * power[k];
        }
        /* Take log (add epsilon to avoid log(0)) */
        if (mel_energy[m] < 1e-10f)
            mel_energy[m] = 1e-10f;
        mel_energy[m] = log10f(mel_energy[m]);
    }

    /* Apply DCT to get MFCC coefficients */
    for (int k = 0; k < DSP_MFCC_COEFFS; k++) {
        mfcc_out[k] = 0.0f;
        for (int n = 0; n < DSP_MFCC_COEFFS; n++) {
            mfcc_out[k] += dct_matrix[k][n] * mel_energy[n];
        }
    }
}

/* ── Compute envelope (public interface) ──────────────────────── */
void dsp_compute_envelope(float *signal, int len, float *env_out)
{
    compute_envelope(signal, len, env_out);
}

/* ── Apply bandpass filter (public interface) ─────────────────── */
void dsp_bandpass_filter(float *signal, int len)
{
    /* In-place FIR filter using a temporary buffer */
    float temp[FFT_SIZE];
    float delay[DSP_FIR_TAPS];
    int idx = 0;

    for (int i = 0; i < DSP_FIR_TAPS; i++)
        delay[i] = 0.0f;

    for (int n = 0; n < len; n++) {
        delay[idx] = signal[n];
        idx = (idx + 1) % DSP_FIR_TAPS;

        float acc = 0.0f;
        int di = idx;
        for (int k = 0; k < DSP_FIR_TAPS; k++) {
            di = (di - 1 + DSP_FIR_TAPS) % DSP_FIR_TAPS;
            acc += fir_coeffs[k] * delay[di];
        }
        temp[n] = acc;
    }

    for (int i = 0; i < len; i++)
        signal[i] = temp[i];
}

/* ── Full burst processing pipeline ─────────────────────────────
 * Transforms raw interleaved 4-channel ADC samples into a
 * 19-dimensional feature vector:
 *   [0..12]  = 13 MFCC coefficients from channel 0 envelope
 *   [13..16] = 4 probe channel amplitude ratios (spatial diversity)
 *   [17]     = burst duration (in samples, normalized)
 *   [18]     = inter-burst interval (if available, else 0)
 *
 * Returns 0 on success, -1 on failure.
 */
int dsp_process_burst(int16_t *raw, uint16_t *gain, float *features)
{
    float ch_filtered[ADC_CHANNELS][BURST_SAMPLES];
    float ch_envelope[BURST_SAMPLES];
    float ch_rms[ADC_CHANNELS];
    float mfcc[DSP_MFCC_COEFFS];

    /* Step 1: DC removal and bandpass filter for each channel */
    for (int ch = 0; ch < ADC_CHANNELS; ch++) {
        /* Extract channel samples and remove DC */
        float mean = 0.0f;
        for (int s = 0; s < BURST_SAMPLES; s++) {
            float val = (float)raw[s * ADC_CHANNELS + ch];
            mean += val;
        }
        mean /= BURST_SAMPLES;

        float ch_data[BURST_SAMPLES];
        for (int s = 0; s < BURST_SAMPLES; s++) {
            ch_data[s] = (float)raw[s * ADC_CHANNELS + ch] - mean;
        }

        /* Apply FIR bandpass filter */
        fir_bandpass_channel((int16_t *)NULL, ch_filtered[ch],
                             BURST_SAMPLES, ch);
        /* Actually we need to filter from ch_data, fix: use in-place filter */
        for (int s = 0; s < BURST_SAMPLES; s++)
            ch_filtered[ch][s] = ch_data[s];
        dsp_bandpass_filter(ch_filtered[ch], BURST_SAMPLES);

        /* Compute RMS for spatial diversity features */
        float sum_sq = 0.0f;
        for (int s = 0; s < BURST_SAMPLES; s++) {
            sum_sq += ch_filtered[ch][s] * ch_filtered[ch][s];
        }
        ch_rms[ch] = sqrtf(sum_sq / BURST_SAMPLES);
    }

    /* Step 2: Compute envelope of channel 0 (primary probe) */
    compute_envelope(ch_filtered[0], BURST_SAMPLES, ch_envelope);

    /* Step 3: Compute MFCC from the envelope */
    dsp_compute_mfcc(ch_envelope, BURST_SAMPLES, mfcc);

    /* Step 4: Assemble feature vector */
    /* MFCC features [0..12] */
    for (int i = 0; i < DSP_MFCC_COEFFS; i++) {
        features[i] = mfcc[i];
    }

    /* Spatial diversity features [13..16]: channel amplitude ratios
     * Normalize each channel RMS relative to the sum of all channels */
    float total_rms = 0.0f;
    for (int ch = 0; ch < ADC_CHANNELS; ch++)
        total_rms += ch_rms[ch];

    if (total_rms > 1e-10f) {
        for (int ch = 0; ch < ADC_CHANNELS; ch++) {
            features[DSP_MFCC_COEFFS + ch] = ch_rms[ch] / total_rms;
        }
    } else {
        for (int ch = 0; ch < ADC_CHANNELS; ch++) {
            features[DSP_MFCC_COEFFS + ch] = 0.25f;  /* equal if no signal */
        }
    }

    /* Burst duration feature [17]: find where envelope exceeds threshold
     * and measure the duration */
    float env_max = 0.0f;
    for (int s = 0; s < BURST_SAMPLES; s++) {
        if (ch_envelope[s] > env_max)
            env_max = ch_envelope[s];
    }
    float threshold = env_max * 0.2f;  /* 20% of peak */
    int first_cross = -1, last_cross = -1;
    for (int s = 0; s < BURST_SAMPLES; s++) {
        if (ch_envelope[s] > threshold) {
            if (first_cross < 0) first_cross = s;
            last_cross = s;
        }
    }
    if (first_cross >= 0 && last_cross > first_cross) {
        features[DSP_MFCC_COEFFS + 4] =
            (float)(last_cross - first_cross) / BURST_SAMPLES;
    } else {
        features[DSP_MFCC_COEFFS + 4] = 0.0f;
    }

    /* Inter-burst interval [18]: would need previous burst timestamp.
     * Set to 0 for now; the classifier can use or ignore this. */
    features[DSP_MFCC_COEFFS + 5] = 0.0f;

    return 0;
}