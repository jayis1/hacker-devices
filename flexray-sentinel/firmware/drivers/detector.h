/*
 * FlexRay schedule-learning anomaly detector API
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef FLEXRAY_SENTINEL_DETECTOR_H
#define FLEXRAY_SENTINEL_DETECTOR_H

#include "flexray.h"

#define DETECTOR_SLOT_TABLE_SIZE 257u
#define DETECTOR_EVENT_TEXT 72u

typedef enum {
    DETECTOR_EVENT_NONE = 0,
    DETECTOR_EVENT_NEW_SLOT,
    DETECTOR_EVENT_CYCLE_SHIFT,
    DETECTOR_EVENT_LENGTH_CHANGE,
    DETECTOR_EVENT_TIMING_DRIFT,
    DETECTOR_EVENT_PAYLOAD_CHANGE,
    DETECTOR_EVENT_CRC_ERROR,
    DETECTOR_EVENT_CHANNEL_ASYMMETRY,
    DETECTOR_EVENT_DUPLICATE_SLOT
} detector_event_kind_t;

typedef struct {
    bool occupied;
    uint16_t slot_id;
    uint16_t expected_length;
    uint32_t cycle_mask_low;
    uint32_t cycle_mask_high;
    uint64_t mean_offset_ticks;
    uint64_t offset_m2;
    uint32_t payload_fingerprint;
    uint32_t observations;
    uint32_t anomalies;
    uint8_t expected_flags;
    fs_channel_t expected_channel;
} detector_slot_profile_t;

typedef struct {
    detector_event_kind_t kind;
    uint64_t timestamp_ticks;
    uint16_t slot_id;
    uint8_t cycle;
    uint16_t score;
    fs_channel_t channel;
    char detail[DETECTOR_EVENT_TEXT];
} detector_event_t;

typedef struct {
    detector_slot_profile_t slots[DETECTOR_SLOT_TABLE_SIZE];
    fs_config_t config;
    uint64_t cycle_start_tick;
    uint64_t last_frame_tick;
    uint16_t last_slot;
    uint8_t last_cycle;
    uint32_t learned_slots;
    uint32_t total_frames;
    uint32_t total_events;
    bool baseline_frozen;
} detector_t;

void detector_init(detector_t *detector, const fs_config_t *config);
void detector_reset(detector_t *detector);
void detector_freeze_baseline(detector_t *detector, bool frozen);
bool detector_process(detector_t *detector,
                      const fr_frame_t *frame,
                      detector_event_t *event);
const detector_slot_profile_t *detector_find_profile(const detector_t *detector,
                                                      uint16_t slot_id);
uint16_t detector_profile_confidence(const detector_slot_profile_t *profile);
const char *detector_event_name(detector_event_kind_t kind);
size_t detector_export_json(const detector_t *detector, char *output, size_t capacity);

#endif
