/*
 * ACOUSTIC-PHANTOM — Event detector driver
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Energy-based onset detection for acoustic events. Scans the
 * beamformed audio stream for transient events (key clicks, relay
 * clicks, seek chirps) and extracts event windows for classification.
 */
#ifndef EVENT_DETECTOR_H
#define EVENT_DETECTOR_H

#include <stdint.h>
#include "board.h"
#include "audio_capture.h"

#define MAX_EVENTS_PER_FRAME  8
#define EVENT_WINDOW_SAMPLES  (AUDIO_SAMPLE_RATE * 300 / 1000) /* 300 ms */

typedef struct {
    int16_t  samples[EVENT_WINDOW_SAMPLES];  /* event audio window */
    uint16_t length;
    uint32_t onset_ms;                        /* timestamp of onset */
    float    energy;                          /* peak energy at onset */
    float    pre_energy;                      /* pre-onset baseline energy */
    uint8_t  channel;                         /* 0=beamformed, 1=piezo */
} event_t;

void  event_detector_init(attack_profile_t profile);
void  event_detector_set_profile(attack_profile_t profile);
int   event_detector_process(const audio_frame_t *frame,
                              event_t *events, int max_events);
void  event_detector_reset(void);

#endif /* EVENT_DETECTOR_H */