/*
 * FlexRay schedule baseline and anomaly scoring engine
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "detector.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define SCORE_NEW_SLOT 65u
#define SCORE_LENGTH_CHANGE 75u
#define SCORE_CYCLE_SHIFT 45u
#define SCORE_TIMING_DRIFT 55u
#define SCORE_PAYLOAD_CHANGE 25u
#define SCORE_CHANNEL_CHANGE 40u
#define SCORE_DUPLICATE 50u

static uint32_t slot_hash(uint16_t slot_id)
{
    return ((uint32_t)slot_id * 2654435761u) % DETECTOR_SLOT_TABLE_SIZE;
}

static detector_slot_profile_t *find_mutable_profile(detector_t *detector,
                                                      uint16_t slot_id,
                                                      bool create)
{
    uint32_t index = slot_hash(slot_id);
    uint32_t probe;
    for (probe = 0u; probe < DETECTOR_SLOT_TABLE_SIZE; ++probe) {
        detector_slot_profile_t *profile = &detector->slots[(index + probe) % DETECTOR_SLOT_TABLE_SIZE];
        if (profile->occupied && profile->slot_id == slot_id) {
            return profile;
        }
        if (!profile->occupied) {
            if (!create) {
                return NULL;
            }
            memset(profile, 0, sizeof(*profile));
            profile->occupied = true;
            profile->slot_id = slot_id;
            detector->learned_slots++;
            return profile;
        }
    }
    return NULL;
}

const detector_slot_profile_t *detector_find_profile(const detector_t *detector,
                                                      uint16_t slot_id)
{
    uint32_t index;
    uint32_t probe;
    if (detector == NULL || slot_id == 0u) {
        return NULL;
    }
    index = slot_hash(slot_id);
    for (probe = 0u; probe < DETECTOR_SLOT_TABLE_SIZE; ++probe) {
        const detector_slot_profile_t *profile = &detector->slots[(index + probe) % DETECTOR_SLOT_TABLE_SIZE];
        if (!profile->occupied) {
            return NULL;
        }
        if (profile->slot_id == slot_id) {
            return profile;
        }
    }
    return NULL;
}

void detector_init(detector_t *detector, const fs_config_t *config)
{
    if (detector == NULL) {
        return;
    }
    memset(detector, 0, sizeof(*detector));
    detector->config = config != NULL ? *config : fs_default_config();
}

void detector_reset(detector_t *detector)
{
    fs_config_t saved;
    if (detector == NULL) {
        return;
    }
    saved = detector->config;
    detector_init(detector, &saved);
}

void detector_freeze_baseline(detector_t *detector, bool frozen)
{
    if (detector != NULL) {
        detector->baseline_frozen = frozen;
    }
}

static uint64_t absolute_difference(uint64_t left, uint64_t right)
{
    return left >= right ? left - right : right - left;
}

static uint64_t frame_offset(const detector_t *detector, const fr_frame_t *frame)
{
    if (frame->timestamp_ticks < detector->cycle_start_tick) {
        return 0u;
    }
    return frame->timestamp_ticks - detector->cycle_start_tick;
}

static bool cycle_present(const detector_slot_profile_t *profile, uint8_t cycle)
{
    if (cycle < 32u) {
        return (profile->cycle_mask_low & (1u << cycle)) != 0u;
    }
    return (profile->cycle_mask_high & (1u << (cycle - 32u))) != 0u;
}

static void add_cycle(detector_slot_profile_t *profile, uint8_t cycle)
{
    if (cycle < 32u) {
        profile->cycle_mask_low |= 1u << cycle;
    } else {
        profile->cycle_mask_high |= 1u << (cycle - 32u);
    }
}

static void update_timing(detector_slot_profile_t *profile, uint64_t offset)
{
    uint64_t delta;
    uint64_t next_mean;
    uint64_t delta2;
    if (profile->observations == 0u) {
        profile->mean_offset_ticks = offset;
        profile->offset_m2 = 0u;
        return;
    }
    if (offset >= profile->mean_offset_ticks) {
        delta = offset - profile->mean_offset_ticks;
        next_mean = profile->mean_offset_ticks + delta / (profile->observations + 1u);
        delta2 = offset >= next_mean ? offset - next_mean : 0u;
        profile->offset_m2 += delta * delta2;
    } else {
        delta = profile->mean_offset_ticks - offset;
        next_mean = profile->mean_offset_ticks - delta / (profile->observations + 1u);
        delta2 = next_mean >= offset ? next_mean - offset : 0u;
        profile->offset_m2 += delta * delta2;
    }
    profile->mean_offset_ticks = next_mean;
}

static void initialize_profile(detector_slot_profile_t *profile,
                               const fr_frame_t *frame,
                               uint64_t offset)
{
    profile->expected_length = frame->payload_length;
    profile->expected_flags = frame->flags;
    profile->expected_channel = frame->channel;
    profile->payload_fingerprint = fr_payload_fingerprint(frame);
    profile->mean_offset_ticks = offset;
    profile->offset_m2 = 0u;
    profile->observations = 1u;
    add_cycle(profile, frame->cycle);
}

static void set_event(detector_event_t *event,
                      detector_event_kind_t kind,
                      const fr_frame_t *frame,
                      uint16_t score,
                      const char *detail)
{
    if (event == NULL) {
        return;
    }
    memset(event, 0, sizeof(*event));
    event->kind = kind;
    event->timestamp_ticks = frame->timestamp_ticks;
    event->slot_id = frame->slot_id;
    event->cycle = frame->cycle;
    event->score = score > 100u ? 100u : score;
    event->channel = frame->channel;
    (void)snprintf(event->detail, sizeof(event->detail), "%s", detail);
}

static uint16_t score_existing(detector_t *detector,
                               detector_slot_profile_t *profile,
                               const fr_frame_t *frame,
                               uint64_t offset,
                               detector_event_t *event)
{
    uint16_t score = 0u;
    detector_event_kind_t primary = DETECTOR_EVENT_NONE;
    char detail[DETECTOR_EVENT_TEXT];
    const uint64_t timing_delta = absolute_difference(offset, profile->mean_offset_ticks);
    const uint32_t fingerprint = fr_payload_fingerprint(frame);

    detail[0] = '\0';
    if (frame->payload_length != profile->expected_length) {
        score = (uint16_t)(score + SCORE_LENGTH_CHANGE);
        primary = DETECTOR_EVENT_LENGTH_CHANGE;
        (void)snprintf(detail, sizeof(detail), "length %u differs from learned %u",
                       frame->payload_length, profile->expected_length);
    }
    if (!cycle_present(profile, frame->cycle)) {
        score = (uint16_t)(score + SCORE_CYCLE_SHIFT);
        if (primary == DETECTOR_EVENT_NONE) {
            primary = DETECTOR_EVENT_CYCLE_SHIFT;
            (void)snprintf(detail, sizeof(detail), "first observation in cycle %u", frame->cycle);
        }
    }
    if (timing_delta > detector->config.timing_tolerance_ticks) {
        score = (uint16_t)(score + SCORE_TIMING_DRIFT);
        if (primary == DETECTOR_EVENT_NONE) {
            primary = DETECTOR_EVENT_TIMING_DRIFT;
            (void)snprintf(detail, sizeof(detail), "timing drift %" PRIu64 " ticks", timing_delta);
        }
    }
    if (profile->observations >= detector->config.minimum_observations &&
        fingerprint != profile->payload_fingerprint) {
        score = (uint16_t)(score + SCORE_PAYLOAD_CHANGE);
        if (primary == DETECTOR_EVENT_NONE) {
            primary = DETECTOR_EVENT_PAYLOAD_CHANGE;
            (void)snprintf(detail, sizeof(detail), "payload fingerprint changed");
        }
    }
    if (frame->channel != profile->expected_channel) {
        score = (uint16_t)(score + SCORE_CHANNEL_CHANGE);
        if (primary == DETECTOR_EVENT_NONE) {
            primary = DETECTOR_EVENT_CHANNEL_ASYMMETRY;
            (void)snprintf(detail, sizeof(detail), "frame moved to channel %c",
                           frame->channel == FS_CHANNEL_A ? 'A' : 'B');
        }
    }
    if (frame->cycle == detector->last_cycle && frame->slot_id == detector->last_slot) {
        score = (uint16_t)(score + SCORE_DUPLICATE);
        if (primary == DETECTOR_EVENT_NONE) {
            primary = DETECTOR_EVENT_DUPLICATE_SLOT;
            (void)snprintf(detail, sizeof(detail), "duplicate slot in one cycle");
        }
    }
    if (score >= detector->config.anomaly_threshold) {
        set_event(event, primary, frame, score, detail);
    }
    return score;
}

static void update_profile(detector_slot_profile_t *profile,
                           const fr_frame_t *frame,
                           uint64_t offset)
{
    update_timing(profile, offset);
    add_cycle(profile, frame->cycle);
    if (profile->observations < UINT32_MAX) {
        profile->observations++;
    }
    if (profile->observations < 4u) {
        profile->expected_length = frame->payload_length;
        profile->expected_flags = frame->flags;
        profile->expected_channel = frame->channel;
        profile->payload_fingerprint = fr_payload_fingerprint(frame);
    }
}

bool detector_process(detector_t *detector,
                      const fr_frame_t *frame,
                      detector_event_t *event)
{
    detector_slot_profile_t *profile;
    uint64_t offset;
    uint16_t score = 0u;
    bool newly_created;

    if (detector == NULL || frame == NULL || event == NULL) {
        return false;
    }
    memset(event, 0, sizeof(*event));
    if (detector->total_frames == 0u || frame->cycle != detector->last_cycle) {
        detector->cycle_start_tick = frame->timestamp_ticks;
    }
    offset = frame_offset(detector, frame);
    profile = find_mutable_profile(detector, frame->slot_id, !detector->baseline_frozen);
    newly_created = profile != NULL && profile->observations == 0u;

    if (profile == NULL) {
        set_event(event, DETECTOR_EVENT_NEW_SLOT, frame, SCORE_NEW_SLOT,
                  "slot absent from frozen baseline");
        score = SCORE_NEW_SLOT;
    } else if (newly_created) {
        initialize_profile(profile, frame, offset);
    } else {
        score = score_existing(detector, profile, frame, offset, event);
        if (!detector->baseline_frozen) {
            update_profile(profile, frame, offset);
        } else if (score >= detector->config.anomaly_threshold && profile->anomalies < UINT32_MAX) {
            profile->anomalies++;
        }
    }

    detector->last_frame_tick = frame->timestamp_ticks;
    detector->last_slot = frame->slot_id;
    detector->last_cycle = frame->cycle;
    detector->total_frames++;
    if (event->kind != DETECTOR_EVENT_NONE) {
        detector->total_events++;
    }
    return true;
}

uint16_t detector_profile_confidence(const detector_slot_profile_t *profile)
{
    uint32_t confidence;
    if (profile == NULL || !profile->occupied) {
        return 0u;
    }
    confidence = profile->observations * 8u;
    if (confidence > 100u) {
        confidence = 100u;
    }
    if (profile->anomalies > profile->observations / 4u) {
        confidence /= 2u;
    }
    return (uint16_t)confidence;
}

const char *detector_event_name(detector_event_kind_t kind)
{
    switch (kind) {
    case DETECTOR_EVENT_NONE: return "none";
    case DETECTOR_EVENT_NEW_SLOT: return "new-slot";
    case DETECTOR_EVENT_CYCLE_SHIFT: return "cycle-shift";
    case DETECTOR_EVENT_LENGTH_CHANGE: return "length-change";
    case DETECTOR_EVENT_TIMING_DRIFT: return "timing-drift";
    case DETECTOR_EVENT_PAYLOAD_CHANGE: return "payload-change";
    case DETECTOR_EVENT_CRC_ERROR: return "crc-error";
    case DETECTOR_EVENT_CHANNEL_ASYMMETRY: return "channel-asymmetry";
    case DETECTOR_EVENT_DUPLICATE_SLOT: return "duplicate-slot";
    default: return "unknown";
    }
}

size_t detector_export_json(const detector_t *detector, char *output, size_t capacity)
{
    size_t used = 0u;
    uint32_t index;
    int written;
    if (detector == NULL || output == NULL || capacity == 0u) {
        return 0u;
    }
    written = snprintf(output, capacity,
                       "{\"author\":\"jayis1\",\"frozen\":%s,\"slots\":[",
                       detector->baseline_frozen ? "true" : "false");
    if (written < 0 || (size_t)written >= capacity) {
        output[0] = '\0';
        return 0u;
    }
    used = (size_t)written;
    for (index = 0u; index < DETECTOR_SLOT_TABLE_SIZE; ++index) {
        const detector_slot_profile_t *profile = &detector->slots[index];
        if (!profile->occupied) {
            continue;
        }
        written = snprintf(output + used, capacity - used,
                           "%s{\"slot\":%u,\"length\":%u,\"seen\":%u,\"confidence\":%u}",
                           used > 45u ? "," : "", profile->slot_id, profile->expected_length,
                           profile->observations, detector_profile_confidence(profile));
        if (written < 0 || (size_t)written >= capacity - used) {
            output[0] = '\0';
            return 0u;
        }
        used += (size_t)written;
    }
    if (capacity - used < 3u) {
        output[0] = '\0';
        return 0u;
    }
    output[used++] = ']';
    output[used++] = '}';
    output[used] = '\0';
    return used;
}
