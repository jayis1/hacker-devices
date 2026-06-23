/*
 * ACOUSTIC-PHANTOM — Beamformer implementation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Delay-and-sum beamformer for the 4-element linear MEMS mic array.
 * Computes per-mic time delays for a given steering angle, applies
 * fractional-sample delay via linear interpolation, sums the delayed
 * signals, and outputs a beamformed mono signal. Also scans all
 * steering directions to estimate the source bearing.
 */

#include <math.h>
#include <string.h>
#include "beamformer.h"

/* ---- State ------------------------------------------------------------- */
static int16_t s_steering_angle = 0;   /* current steering angle (degrees) */
static int16_t s_estimated_bearing = 0; /* auto-estimated source bearing */
static int16_t s_beam_output[FRAME_SAMPLES];

/* Precomputed delays (in samples) for each mic at current steering angle */
static float s_delays[NUM_MICS];

/* ---- Compute per-mic delays for a given steering angle ----------------- */
static void compute_delays(float angle_deg)
{
    /* Linear array, spacing = MIC_SPACING_MM
     * Delay for mic i = (i * d * sin(theta)) / c
     * where d = spacing, c = speed of sound, theta = steering angle
     */
    float theta_rad = angle_deg * 3.14159265f / 180.0f;
    float sin_theta = sinf(theta_rad);

    for (int i = 0; i < NUM_MICS; i++) {
        /* Center the array: mic positions at (i - (N-1)/2) * d */
        float pos_mm = (i - (NUM_MICS - 1) / 2.0f) * MIC_SPACING_MM;
        float delay_mm = pos_mm * sin_theta;
        s_delays[i] = delay_mm / SPEED_OF_SOUND * AUDIO_SAMPLE_RATE;
    }
}

/* ---- Linear interpolation for fractional sample delay ------------------ */
static inline int16_t interpolate(const int16_t *buf, float idx)
{
    int   i = (int)idx;
    float frac = idx - i;

    if (i < 0) return buf[0];
    if (i >= FRAME_SAMPLES - 1) return buf[FRAME_SAMPLES - 1];

    return (int16_t)(buf[i] * (1.0f - frac) + buf[i + 1] * frac);
}

/* ---- Delay-and-sum beamforming ----------------------------------------- */
static void delay_and_sum(const audio_frame_t *frame, int16_t *output)
{
    compute_delays((float)s_steering_angle);

    /* Use center mic (mic 1 or 2) as reference → delays relative to center */
    float ref_delay = s_delays[NUM_MICS / 2];

    for (int n = 0; n < FRAME_SAMPLES; n++) {
        int32_t sum = 0;
        for (int m = 0; m < NUM_MICS; m++) {
            float sample_idx = n - (s_delays[m] - ref_delay);
            sum += interpolate(frame->mic[m], sample_idx);
        }
        /* Average with rounding */
        output[n] = (int16_t)(sum / NUM_MICS);
    }
}

/* ---- Auto-steer: scan directions, pick max energy ---------------------- */
void beamformer_auto_steer(audio_frame_t *frame)
{
    float max_energy = 0.0f;
    int   best_angle = 0;

    /* Scan -45° to +45° in 5° steps */
    for (int angle = -45; angle <= 45; angle += 5) {
        compute_delays((float)angle);

        float ref_delay = s_delays[NUM_MICS / 2];
        float energy = 0.0f;

        /* Compute energy over first 200 samples (fast scan) */
        for (int n = 0; n < 200; n++) {
            int32_t sum = 0;
            for (int m = 0; m < NUM_MICS; m++) {
                float idx = n - (s_delays[m] - ref_delay);
                sum += interpolate(frame->mic[m], idx);
            }
            int16_t val = (int16_t)(sum / NUM_MICS);
            energy += (float)val * val;
        }

        if (energy > max_energy) {
            max_energy = energy;
            best_angle = angle;
        }
    }

    s_estimated_bearing = (int16_t)best_angle;
    s_steering_angle = (int16_t)best_angle;
}

/* ---- Public API -------------------------------------------------------- */
void beamformer_init(void)
{
    s_steering_angle = 0;
    s_estimated_bearing = 0;
    memset(s_beam_output, 0, sizeof(s_beam_output));
    compute_delays(0.0f);
}

void beamformer_process(audio_frame_t *frame)
{
    delay_and_sum(frame, s_beam_output);

    /* Periodically auto-steer toward the strongest source */
    static uint32_t last_steer = 0;
    if ((board_millis() - last_steer) > 2000) {  /* every 2 s */
        last_steer = board_millis();
        beamformer_auto_steer(frame);
    }
}

void beamformer_set_steering(int16_t angle_deg)
{
    if (angle_deg < -60) angle_deg = -60;
    if (angle_deg > 60)  angle_deg = 60;
    s_steering_angle = angle_deg;
}

int16_t beamformer_get_bearing(void)
{
    return s_estimated_bearing;
}

void beamformer_get_output(int16_t *out, uint16_t max_samples)
{
    uint16_t n = FRAME_SAMPLES;
    if (n > max_samples) n = max_samples;
    memcpy(out, s_beam_output, n * sizeof(int16_t));
}