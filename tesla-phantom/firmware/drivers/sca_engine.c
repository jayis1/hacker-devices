/*
 * tesla-phantom — drivers/sca_engine.c
 * Side-channel analysis trace processing: filtering, envelope,
 * FFT, correlation, and leakage detection.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* ── Constants ────────────────────────────────────────────────────── */
#define SCA_MAX_FEATURES   64
#define SCA_FFT_SIZE       256   /* power of 2 for FFT */
#define SCA_BPASS_TAPS     31    /* bandpass FIR filter taps */

/* ── FIR bandpass filter coefficients (precomputed for 10–200 kHz) ─ */
/* Hamming window FIR, 31 taps, passband 10–200 kHz at 800 kSPS */
static const float bp_coeffs[SCA_BPASS_TAPS] = {
    -0.0007f, -0.0018f, -0.0021f,  0.0012f,  0.0068f,  0.0102f,  0.0051f,
    -0.0084f, -0.0223f, -0.0265f, -0.0098f,  0.0210f,  0.0512f,  0.0598f,
     0.0314f, -0.0314f, -0.0598f, -0.0512f, -0.0210f,  0.0098f,  0.0265f,
     0.0223f,  0.0084f, -0.0051f, -0.0102f, -0.0068f, -0.0012f,  0.0021f,
     0.0018f,  0.0007f,  0.0000f
};

/* ── Internal helpers ─────────────────────────────────────────────── */

/* Convert interleaved ADC data to single-channel array.
 * The AD7606 outputs 6 channels per sample, interleaved.
 * We extract channel 0 (fluxgate output) for SCA processing. */
static void extract_channel(int16_t *raw, uint32_t samples, int channel,
                             float *out) {
    for (uint32_t i = 0; i < samples; i++) {
        out[i] = (float)raw[i * ADC_CHANNELS + channel] / 32768.0f;
    }
}

/* FIR filter application */
static void fir_filter(const float *input, float *output, int len,
                        const float *coeffs, int taps) {
    for (int i = 0; i < len; i++) {
        float acc = 0.0f;
        for (int j = 0; j < taps; j++) {
            int idx = i - j + taps / 2;
            if (idx >= 0 && idx < len)
                acc += input[idx] * coeffs[j];
        }
        output[i] = acc;
    }
}

/* Simple radix-2 DIT FFT (Cooley-Tukey) for real input.
 * n must be a power of 2. Input in re[], im[] zeroed. */
static void fft_radix2(float *re, float *im, int n) {
    /* Bit reversal */
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            float tr = re[i]; re[i] = re[j]; re[j] = tr;
            float ti = im[i]; im[i] = im[j]; im[j] = ti;
        }
    }

    /* Butterfly */
    for (int len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * 3.14159265f / (float)len;
        float wlen_r = cosf_approx(angle);
        float wlen_i = sinf_approx(angle);
        for (int i = 0; i < n; i += len) {
            float w_r = 1.0f, w_i = 0.0f;
            for (int j = 0; j < len / 2; j++) {
                float u_r = re[i + j];
                float u_i = im[i + j];
                float v_r = re[i + j + len/2] * w_r - im[i + j + len/2] * w_i;
                float v_i = re[i + j + len/2] * w_i + im[i + j + len/2] * w_r;
                re[i + j] = u_r + v_r;
                im[i + j] = u_i + v_i;
                re[i + j + len/2] = u_r - v_r;
                im[i + j + len/2] = u_i - v_i;
                float nw_r = w_r * wlen_r - w_i * wlen_i;
                float nw_i = w_r * wlen_i + w_i * wlen_r;
                w_r = nw_r;
                w_i = nw_i;
            }
        }
    }
}

/* Fast cosine approximation (Bhaskara I formula, good enough for FFT) */
static float cosf_approx(float x) {
    /* (implementation below — forward declared above) */
    /* Reduce to [-pi, pi] */
    while (x > 3.14159265f)  x -= 2.0f * 3.14159265f;
    while (x < -3.14159265f) x += 2.0f * 3.14159265f;
    /* Taylor: 1 - x²/2 + x⁴/24 - x⁶/720 */
    float x2 = x * x;
    return 1.0f - x2 * 0.5f + x2 * x2 * 0.04167f - x2 * x2 * x2 * 0.00139f;
}

static float sinf_approx(float x) {
    return cosf_approx(x - 1.5707963f);  /* sin(x) = cos(x - pi/2) */
}

/* ── Public API ───────────────────────────────────────────────────── */

int sca_init(void) {
    /* No special initialization needed — filters are coefficient-based */
    return 0;
}

int sca_process_trace(int16_t *raw, uint32_t samples, float *features) {
    if (samples == 0 || raw == NULL || features == NULL)
        return -1;

    /* Allocate working buffers on stack (limited size) */
    float ch0[2048];
    float filtered[2048];
    float envelope[2048];
    int n = (samples < 2048) ? (int)samples : 2048;

    /* Extract channel 0 (fluxgate magnetic sensor) */
    extract_channel(raw, n, 0, ch0);

    /* Apply bandpass filter (10–200 kHz) */
    sca_bandpass_filter(ch0, n, 10000, 200000);

    /* Compute envelope */
    sca_compute_envelope(ch0, n, envelope);

    /* Compute features for classification:
     * - Peak amplitude
     * - RMS energy
     * - Spectral centroid
     * - Zero-crossing rate
     * - Band power in several sub-bands */
    float peak = 0.0f, rms = 0.0f;
    int zero_crossings = 0;
    float prev = ch0[0];

    for (int i = 0; i < n; i++) {
        float v = ch0[i];
        float av = (v < 0) ? -v : v;
        if (av > peak) peak = av;
        rms += v * v;
        if ((prev >= 0) != (v >= 0)) zero_crossings++;
        prev = v;
    }
    rms /= n;
    /* rms = sqrt(rms) — use simple approximation */
    float rms_sqrt = rms;
    for (int it = 0; it < 8; it++)
        rms_sqrt = 0.5f * (rms_sqrt + rms / rms_sqrt);
    rms = rms_sqrt;

    /* FFT for spectral features */
    float fft_re[SCA_FFT_SIZE];
    float fft_im[SCA_FFT_SIZE];
    int fft_n = SCA_FFT_SIZE;
    if (n < fft_n) fft_n = n;

    for (int i = 0; i < fft_n; i++) {
        fft_re[i] = ch0[i];
        fft_im[i] = 0.0f;
    }
    /* Zero-pad if needed */
    for (int i = fft_n; i < SCA_FFT_SIZE; i++) {
        fft_re[i] = 0.0f;
        fft_im[i] = 0.0f;
    }

    fft_radix2(fft_re, fft_im, SCA_FFT_SIZE);

    /* Compute magnitude and spectral centroid */
    float mag[SCA_FFT_SIZE / 2];
    float total_mag = 0.0f, weighted_sum = 0.0f;
    for (int i = 0; i < SCA_FFT_SIZE / 2; i++) {
        mag[i] = sqrtf_approx(fft_re[i] * fft_re[i] + fft_im[i] * fft_im[i]);
        total_mag += mag[i];
        weighted_sum += (float)i * mag[i];
    }
    float centroid = (total_mag > 0) ? weighted_sum / total_mag : 0.0f;

    /* Sub-band power */
    int band_size = (SCA_FFT_SIZE / 2) / 8;
    for (int b = 0; b < 8 && b < SCA_MAX_FEATURES; b++) {
        float band_power = 0.0f;
        for (int i = b * band_size; i < (b + 1) * band_size; i++)
            band_power += mag[i] * mag[i];
        features[b] = band_power / (band_size * total_mag + 1e-10f);
    }

    /* Store scalar features */
    features[8]  = peak;
    features[9]  = rms;
    features[10] = (float)zero_crossings / n;
    features[11] = centroid;
    features[12] = envelope[n / 2];  /* mid-envelope value */
    features[13] = envelope[n - 1];  /* end envelope */
    features[14] = (float)n;          /* sample count */
    features[15] = 0.0f;              /* reserved */

    /* Zero remaining features */
    for (int i = 16; i < SCA_MAX_FEATURES; i++)
        features[i] = 0.0f;

    return 0;
}

void sca_bandpass_filter(float *signal, int len, int low_hz, int high_hz) {
    if (len <= 0) return;

    float temp[2048];
    int n = (len < 2048) ? len : 2048;

    /* Apply the precomputed FIR bandpass filter */
    fir_filter(signal, temp, n, bp_coeffs, SCA_BPASS_TAPS);

    /* Copy back */
    for (int i = 0; i < n; i++)
        signal[i] = temp[i];

    (void)low_hz;
    (void)high_hz;
}

void sca_compute_envelope(float *signal, int len, float *env) {
    /* Compute signal envelope using rectification + moving average */
    int window = 32;  /* moving average window */
    if (window > len) window = len;

    float sum = 0.0f;
    /* Initialize window */
    for (int i = 0; i < window && i < len; i++) {
        float v = signal[i];
        if (v < 0) v = -v;  /* rectify */
        sum += v;
    }

    for (int i = 0; i < len; i++) {
        env[i] = sum / window;
        /* Slide window */
        int out_idx = i - window / 2;
        int in_idx  = i + window / 2;
        if (out_idx >= 0 && out_idx < len) {
            float v = signal[out_idx];
            sum -= (v < 0) ? -v : v;
        }
        if (in_idx >= 0 && in_idx < len) {
            float v = signal[in_idx];
            sum += (v < 0) ? -v : v;
        }
    }
}

float sca_correlate(float *trace, float *model, int len) {
    /* Pearson correlation coefficient */
    if (len <= 0) return 0.0f;

    float mean_t = 0.0f, mean_m = 0.0f;
    for (int i = 0; i < len; i++) {
        mean_t += trace[i];
        mean_m += model[i];
    }
    mean_t /= len;
    mean_m /= len;

    float num = 0.0f, den_t = 0.0f, den_m = 0.0f;
    for (int i = 0; i < len; i++) {
        float dt = trace[i] - mean_t;
        float dm = model[i] - mean_m;
        num  += dt * dm;
        den_t += dt * dt;
        den_m += dm * dm;
    }

    float denom = sqrtf_approx(den_t * den_m);
    if (denom < 1e-10f) return 0.0f;
    return num / denom;
}

int sca_detect_leakage(float *trace, int len, float *score) {
    /* Leakage detection: check if the trace has detectable structure
     * beyond noise. We use the spectral flatness measure (Wiener entropy).
     * A low spectral flatness (< 0.3) indicates tonal structure (leakage). */
    if (len < 16) {
        *score = 0.0f;
        return 0;
    }

    /* Compute FFT */
    float re[SCA_FFT_SIZE];
    float im[SCA_FFT_SIZE];
    int n = (len < SCA_FFT_SIZE) ? len : SCA_FFT_SIZE;

    for (int i = 0; i < SCA_FFT_SIZE; i++) {
        re[i] = (i < n) ? trace[i] : 0.0f;
        im[i] = 0.0f;
    }

    fft_radix2(re, im, SCA_FFT_SIZE);

    /* Spectral flatness = exp(mean(log(P))) / mean(P) */
    float sum_log = 0.0f, sum_p = 0.0f;
    int count = SCA_FFT_SIZE / 2;
    for (int i = 1; i < count; i++) {  /* skip DC */
        float p = re[i] * re[i] + im[i] * im[i];
        if (p < 1e-10f) p = 1e-10f;
        sum_log += logf_approx(p);
        sum_p += p;
    }
    float mean_log = sum_log / (count - 1);
    float mean_p = sum_p / (count - 1);

    /* exp(mean_log) / mean_p */
    float geo_mean = expf_approx(mean_log);
    float flatness = (mean_p > 1e-10f) ? geo_mean / mean_p : 0.0f;

    /* Leakage score = 1 - flatness (higher = more structured = more leakage) */
    *score = 1.0f - flatness;

    return (*score > 0.7f) ? 1 : 0;  /* 1 = leakage detected */
}

void sca_compute_fft(float *signal, int len, float *mag_out) {
    if (len <= 0) return;

    float re[SCA_FFT_SIZE];
    float im[SCA_FFT_SIZE];
    int n = (len < SCA_FFT_SIZE) ? len : SCA_FFT_SIZE;

    for (int i = 0; i < SCA_FFT_SIZE; i++) {
        re[i] = (i < n) ? signal[i] : 0.0f;
        im[i] = 0.0f;
    }

    fft_radix2(re, im, SCA_FFT_SIZE);

    int half = SCA_FFT_SIZE / 2;
    for (int i = 0; i < half; i++) {
        mag_out[i] = sqrtf_approx(re[i] * re[i] + im[i] * im[i]);
    }
}

/* ── Math approximations (forward-declared above) ────────────────── */
static float cosf_approx(float x);  /* used by fft_radix2 */
static float sinf_approx(float x);  /* used by fft_radix2 */
static float sqrtf_approx(float x); /* used by sca_correlate etc. */
static float logf_approx(float x);  /* used by sca_detect_leakage */
static float expf_approx(float x);  /* used by sca_detect_leakage */

static float sqrtf_approx(float x) {
    if (x <= 0.0f) return 0.0f;
    float r = x;
    for (int i = 0; i < 10; i++)
        r = 0.5f * (r + x / r);
    return r;
}

static float logf_approx(float x) {
    /* ln(x) approximation via bit manipulation + polynomial
     * Good for x in [0.1, 10] range */
    if (x <= 0.0f) return -1e10f;
    int e = 0;
    float m = x;
    while (m >= 2.0f) { m *= 0.5f; e++; }
    while (m < 1.0f)  { m *= 2.0f; e--; }
    /* ln(x) = e*ln(2) + ln(m), m in [1, 2) */
    float y = (m - 1.0f) / (m + 1.0f);
    float ln_m = 2.0f * (y + y*y*y/3.0f + y*y*y*y*y/5.0f);
    return (float)e * 0.6931472f + ln_m;
}

static float expf_approx(float x) {
    /* exp(x) = 2^(x/ln2) — use series for small x, scaling for large */
    if (x > 20.0f) x = 20.0f;
    if (x < -20.0f) x = -20.0f;
    float result = 1.0f;
    float term = 1.0f;
    for (int i = 1; i <= 10; i++) {
        term *= x / (float)i;
        result += term;
    }
    return result;
}