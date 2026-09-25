/*
 * SENT Sentinel trust detector interface
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef SENT_SENTINEL_DETECTOR_H
#define SENT_SENTINEL_DETECTOR_H

#include "sent.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DETECTOR_FLAG_TIMING (1u << 0)
#define DETECTOR_FLAG_STATUS (1u << 1)
#define DETECTOR_FLAG_RANGE (1u << 2)
#define DETECTOR_FLAG_ANALOG (1u << 3)
#define DETECTOR_FLAG_COUNTER (1u << 4)
#define DETECTOR_FLAG_IDENTITY (1u << 5)

typedef enum {
    DETECTOR_EVENT_NONE = 0,
    DETECTOR_EVENT_NEW_CHANNEL,
    DETECTOR_EVENT_TIMING_SHIFT,
    DETECTOR_EVENT_STATUS_CHANGE,
    DETECTOR_EVENT_RANGE_VIOLATION,
    DETECTOR_EVENT_ANALOG_MISMATCH,
    DETECTOR_EVENT_COUNTER_DISCONTINUITY,
    DETECTOR_EVENT_FINGERPRINT_CHANGE
} detector_event_kind_t;

typedef struct {
    bool occupied;
    ss_channel_t channel;
    uint32_t observations;
    uint64_t mean_period_us;
    uint64_t mean_tick_ns;
    uint64_t mean_analog_mv;
    uint64_t mean_fast_value;
    uint16_t minimum_value;
    uint16_t maximum_value;
    uint16_t last_value;
    uint32_t stable_fingerprint;
    uint32_t last_sequence;
    uint8_t expected_status;
} detector_profile_t;

typedef struct {
    detector_event_kind_t kind;
    uint64_t timestamp_us;
    ss_channel_t channel;
    uint16_t value;
    uint16_t analog_mv;
    uint8_t score;
    uint8_t flags;
    char detail[SS_EVENT_TEXT_LENGTH];
} detector_event_t;

typedef struct {
    ss_config_t config;
    detector_profile_t profiles[SS_PROFILE_CAPACITY];
    uint64_t last_timestamp_us[SS_CHANNEL_COUNT];
    bool baseline_frozen;
    uint32_t total_frames;
    uint32_t total_events;
    uint32_t suppressed_events;
} detector_t;

void detector_init(detector_t *detector, const ss_config_t *config);
void detector_freeze(detector_t *detector, bool frozen);
bool detector_process(detector_t *detector, const sent_frame_t *frame, detector_event_t *event);
size_t detector_export_json(const detector_t *detector, char *output, size_t capacity);
const char *detector_event_name(detector_event_kind_t kind);

#endif
