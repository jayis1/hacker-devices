/*
 * ACOUSTIC-PHANTOM — Event detector implementation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Energy-based onset detection for acoustic events. Computes short-time
 * energy in a sliding window, compares against an adaptive noise floor
 * estimate, and flags onset events when energy exceeds the threshold.
 * Extracts a 300 ms event window (±150 ms around onset) for the
 * classifier.
 */

#include <string.h>
#include <math.h>
#include "event_detector.h"
#include "beamformer.h"

/* ---- State ------------------------------------------------------------- */
static attack_profile_t s_profile = PROFILE_KEYBOARD;

/* Sliding window energy buffer */
#define ENERGY_WINDOW 64
static float s_energy_history[ENERGY_WINDOW];
static int   s_energy_idx = 0;
static float s_noise_floor = 0.0f;
static float s_noise_floor_alpha = 0.001f;  /* slow adaptation */

/* Last event tracking (for minimum gap enforcement) */
static uint32_t s_last_event_ms = 0;

/* Running sample buffer for event window extraction */
#define RING_SIZE  (EVENT_WINDOW_SAMPLES + FRAME_SAMPLES)
static int16_t s_ring_buf[RING_SIZE];
static int     s_ring_write_idx = 0;

/* ---- Compute short-time energy of a frame ------------------------------ */
static float compute_frame_energy(const int16_t *samples, int count)
{
    float energy = 0.0f;
    for (int i = 0; i < count; i++) {
        float s = (float)samples[i] / 32768.0f;
        energy += s * s;
    }
    return energy / count;
}

/* ---- Push samples into ring buffer ------------------------------------- */
static void ring_push(const int16_t *samples, int count)
{
    for (int i = 0; i < count && i < FRAME_SAMPLES; i++) {
        s_ring_buf[s_ring_write_idx] = samples[i];
        s_ring_write_idx = (s_ring_write_idx + 1) % RING_SIZE;
    }
}

/* ---- Extract event window centered at the onset ------------------------ */
static void extract_event_window(event_t *event, int onset_offset)
{
    /* The event window is ±EVENT_CONTEXT_SAMP samples around onset.
     * We read from the ring buffer backwards from the write position. */
    int read_start = s_ring_write_idx - EVENT_CONTEXT_SAMP - onset_offset;
    if (read_start < 0) read_start += RING_SIZE;

    event->length = EVENT_WINDOW_SAMPLES;
    for (int i = 0; i < EVENT_WINDOW_SAMPLES; i++) {
        int idx = (read_start + i) % RING_SIZE;
        event->samples[i] = s_ring_buf[idx];
    }
}

/* ---- Update noise floor estimate (adaptive) ---------------------------- */
static void update_noise_floor(float energy)
{
    /* Exponential moving average of the noise floor.
     * Only update when energy is low (not during an event). */
    if (energy < s_noise_floor * 3.0f + 1e-6f) {
        s_noise_floor = s_noise_floor * (1.0f - s_noise_floor_alpha) +
                        energy * s_noise_floor_alpha;
    }
}

/* ---- Public API -------------------------------------------------------- */
void event_detector_init(attack_profile_t profile)
{
    s_profile = profile;
    memset(s_energy_history, 0, sizeof(s_energy_history));
    s_energy_idx = 0;
    s_noise_floor = 0.0f;
    s_last_event_ms = 0;
    memset(s_ring_buf, 0, sizeof(s_ring_buf));
    s_ring_write_idx = 0;
}

void event_detector_set_profile(attack_profile_t profile)
{
    s_profile = profile;
}

void event_detector_reset(void)
{
    memset(s_energy_history, 0, sizeof(s_energy_history));
    s_energy_idx = 0;
    s_noise_floor = 0.0f;
    s_last_event_ms = 0;
    memset(s_ring_buf, 0, sizeof(s_ring_buf));
    s_ring_write_idx = 0;
}

int event_detector_process(const audio_frame_t *frame,
                           event_t *events, int max_events)
{
    int num_events = 0;

    /* Select the input source based on profile */
    int16_t src[FRAME_SAMPLES];
    if (s_profile == PROFILE_HDD || s_profile == PROFILE_SMPS) {
        memcpy(src, frame->piezo, FRAME_SAMPLES * sizeof(int16_t));
    } else {
        beamformer_get_output(src, FRAME_SAMPLES);
    }

    /* Push into ring buffer for event window extraction */
    ring_push(src, FRAME_SAMPLES);

    /* Compute energy in sub-windows (4 sub-windows per frame, 300 samples each) */
    int sub_win = FRAME_SAMPLES / 4;

    for (int sw = 0; sw < 4 && num_events < max_events; sw++) {
        float energy = compute_frame_energy(src + sw * sub_win, sub_win);

        /* Store in history */
        s_energy_history[s_energy_idx] = energy;
        s_energy_idx = (s_energy_idx + 1) % ENERGY_WINDOW;

        /* Update noise floor */
        update_noise_floor(energy);

        /* Onset detection: energy must exceed threshold * noise floor */
        float threshold = s_noise_floor + ONSET_THRESHOLD;
        if (energy > threshold && s_noise_floor > 1e-8f) {

            uint32_t now = board_millis();
            if ((now - s_last_event_ms) < MIN_EVENT_GAP_MS) {
                continue;  /* too soon after last event */
            }

            /* Check that this is a genuine onset (rising edge) */
            int prev_idx = (s_energy_idx - 2 + ENERGY_WINDOW) % ENERGY_WINDOW;
            float prev_energy = s_energy_history[prev_idx];

            if (energy > prev_energy * 1.5f) {
                /* Genuine onset — extract event window */
                events[num_events].onset_ms = now;
                events[num_events].energy = energy;
                events[num_events].pre_energy = prev_energy;
                events[num_events].channel =
                    (s_profile == PROFILE_HDD || s_profile == PROFILE_SMPS) ? 1 : 0;

                extract_event_window(&events[num_events], sw * sub_win);

                num_events++;
                s_last_event_ms = now;
            }
        }
    }

    return num_events;
}