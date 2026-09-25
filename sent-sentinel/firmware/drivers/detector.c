/*
 * SENT Sentinel explainable baseline and anomaly detector
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "detector.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static uint64_t difference_u64(uint64_t left, uint64_t right)
{
    return left > right ? left - right : right - left;
}

static detector_profile_t *find_profile(detector_t *detector, ss_channel_t channel)
{
    size_t index;
    detector_profile_t *free_profile = NULL;

    for (index = 0u; index < SS_PROFILE_CAPACITY; ++index) {
        detector_profile_t *profile = &detector->profiles[index];
        if (profile->occupied && profile->channel == channel) {
            return profile;
        }
        if (!profile->occupied && free_profile == NULL) {
            free_profile = profile;
        }
    }
    return free_profile;
}

static void event_clear(detector_event_t *event, const sent_frame_t *frame)
{
    memset(event, 0, sizeof(*event));
    event->kind = DETECTOR_EVENT_NONE;
    event->timestamp_us = frame->timestamp_us;
    event->channel = frame->channel;
    event->value = frame->fast_value;
    event->analog_mv = frame->analog_mv;
}

static void add_score(detector_event_t *event,
                      uint8_t amount,
                      uint8_t flag,
                      detector_event_kind_t kind)
{
    unsigned score = event->score + amount;
    event->score = (uint8_t)(score > 100u ? 100u : score);
    event->flags = (uint8_t)(event->flags | flag);
    if (event->kind == DETECTOR_EVENT_NONE || amount >= 25u) {
        event->kind = kind;
    }
}

static void update_mean(uint64_t *mean, uint64_t sample, uint32_t observations)
{
    if (observations <= 1u) {
        *mean = sample;
    } else if (sample >= *mean) {
        *mean += (sample - *mean) / observations;
    } else {
        *mean -= (*mean - sample) / observations;
    }
}

static bool exceeds_ppm(uint64_t observed, uint64_t expected, uint32_t tolerance_ppm)
{
    uint64_t difference;
    uint64_t allowed;

    if (expected == 0u) {
        return false;
    }
    difference = difference_u64(observed, expected);
    allowed = (expected * tolerance_ppm) / 1000000u;
    return difference > (allowed == 0u ? 1u : allowed);
}

static void initialize_profile(detector_profile_t *profile,
                               const sent_frame_t *frame,
                               uint64_t period_us)
{
    memset(profile, 0, sizeof(*profile));
    profile->occupied = true;
    profile->channel = frame->channel;
    profile->observations = 1u;
    profile->mean_period_us = period_us;
    profile->mean_tick_ns = frame->tick_time_ns;
    profile->mean_analog_mv = frame->analog_mv;
    profile->mean_fast_value = frame->fast_value;
    profile->minimum_value = frame->fast_value;
    profile->maximum_value = frame->fast_value;
    profile->last_value = frame->fast_value;
    profile->stable_fingerprint = sent_fingerprint(frame);
    profile->last_sequence = frame->sequence;
    profile->expected_status = frame->status;
}

static void learn_profile(detector_profile_t *profile,
                          const sent_frame_t *frame,
                          uint64_t period_us)
{
    profile->observations++;
    if (period_us != 0u) {
        update_mean(&profile->mean_period_us, period_us, profile->observations);
    }
    update_mean(&profile->mean_tick_ns, frame->tick_time_ns, profile->observations);
    update_mean(&profile->mean_analog_mv, frame->analog_mv, profile->observations);
    update_mean(&profile->mean_fast_value, frame->fast_value, profile->observations);
    if (frame->fast_value < profile->minimum_value) {
        profile->minimum_value = frame->fast_value;
    }
    if (frame->fast_value > profile->maximum_value) {
        profile->maximum_value = frame->fast_value;
    }
    profile->last_value = frame->fast_value;
    profile->last_sequence = frame->sequence;
}

static void score_timing(const detector_t *detector,
                         const detector_profile_t *profile,
                         const sent_frame_t *frame,
                         uint64_t period_us,
                         detector_event_t *event)
{
    if (period_us != 0u && exceeds_ppm(period_us,
                                       profile->mean_period_us,
                                       detector->config.timing_tolerance_ppm)) {
        add_score(event, 24u, DETECTOR_FLAG_TIMING, DETECTOR_EVENT_TIMING_SHIFT);
    }
    if (exceeds_ppm(frame->tick_time_ns,
                    profile->mean_tick_ns,
                    detector->config.timing_tolerance_ppm)) {
        add_score(event, 30u, DETECTOR_FLAG_TIMING, DETECTOR_EVENT_TIMING_SHIFT);
    }
}

static void score_semantics(const detector_t *detector,
                            const detector_profile_t *profile,
                            const sent_frame_t *frame,
                            detector_event_t *event)
{
    uint64_t analog_difference = difference_u64(frame->analog_mv, profile->mean_analog_mv);
    uint64_t value_difference = difference_u64(frame->fast_value, profile->mean_fast_value);
    uint64_t expected_analog_delta = (value_difference * SS_ANALOG_FULL_SCALE_MV) / 4095u;

    if (frame->status != profile->expected_status) {
        add_score(event, 22u, DETECTOR_FLAG_STATUS, DETECTOR_EVENT_STATUS_CHANGE);
    }
    if (frame->fast_value + 32u < profile->minimum_value ||
        frame->fast_value > (uint32_t)profile->maximum_value + 32u) {
        add_score(event, 28u, DETECTOR_FLAG_RANGE, DETECTOR_EVENT_RANGE_VIOLATION);
    }
    if (difference_u64(analog_difference, expected_analog_delta) > detector->config.analog_tolerance_mv) {
        add_score(event, 36u, DETECTOR_FLAG_ANALOG, DETECTOR_EVENT_ANALOG_MISMATCH);
    }
    if (frame->sequence != profile->last_sequence + 1u) {
        add_score(event, 18u, DETECTOR_FLAG_COUNTER, DETECTOR_EVENT_COUNTER_DISCONTINUITY);
    }
    if (sent_fingerprint(frame) != profile->stable_fingerprint) {
        add_score(event, 16u, DETECTOR_FLAG_IDENTITY, DETECTOR_EVENT_FINGERPRINT_CHANGE);
    }
}

void detector_init(detector_t *detector, const ss_config_t *config)
{
    if (detector == NULL) {
        return;
    }
    memset(detector, 0, sizeof(*detector));
    detector->config = config == NULL ? ss_default_config() : *config;
}

void detector_freeze(detector_t *detector, bool frozen)
{
    if (detector != NULL) {
        detector->baseline_frozen = frozen;
    }
}

bool detector_process(detector_t *detector, const sent_frame_t *frame, detector_event_t *event)
{
    detector_profile_t *profile;
    uint64_t previous_timestamp;
    uint64_t period_us;

    if (detector == NULL || frame == NULL || event == NULL || !ss_channel_valid(frame->channel)) {
        return false;
    }
    event_clear(event, frame);
    detector->total_frames++;
    previous_timestamp = detector->last_timestamp_us[frame->channel];
    period_us = previous_timestamp == 0u ? 0u : frame->timestamp_us - previous_timestamp;
    detector->last_timestamp_us[frame->channel] = frame->timestamp_us;
    profile = find_profile(detector, frame->channel);
    if (profile == NULL) {
        detector->suppressed_events++;
        return false;
    }
    if (!profile->occupied) {
        initialize_profile(profile, frame, period_us);
        if (detector->baseline_frozen) {
            add_score(event, 70u, DETECTOR_FLAG_IDENTITY, DETECTOR_EVENT_NEW_CHANNEL);
        }
    } else if (!detector->baseline_frozen || profile->observations < detector->config.minimum_observations) {
        learn_profile(profile, frame, period_us);
    } else {
        score_timing(detector, profile, frame, period_us, event);
        score_semantics(detector, profile, frame, event);
        profile->last_sequence = frame->sequence;
        profile->last_value = frame->fast_value;
    }
    if (event->score >= detector->config.score_threshold) {
        (void)snprintf(event->detail,
                       sizeof(event->detail),
                       "channel=%u value=%u analog=%umV flags=0x%02x",
                       (unsigned)frame->channel,
                       frame->fast_value,
                       frame->analog_mv,
                       event->flags);
        detector->total_events++;
    } else {
        event->kind = DETECTOR_EVENT_NONE;
    }
    return true;
}

size_t detector_export_json(const detector_t *detector, char *output, size_t capacity)
{
    size_t used = 0u;
    size_t index;
    int written;

    if (detector == NULL || output == NULL || capacity == 0u) {
        return 0u;
    }
    written = snprintf(output,
                       capacity,
                       "{\"author\":\"jayis1\",\"frozen\":%s,\"profiles\":[",
                       detector->baseline_frozen ? "true" : "false");
    if (written < 0 || (size_t)written >= capacity) {
        return 0u;
    }
    used = (size_t)written;
    for (index = 0u; index < SS_PROFILE_CAPACITY; ++index) {
        const detector_profile_t *profile = &detector->profiles[index];
        if (!profile->occupied) {
            continue;
        }
        written = snprintf(output + used,
                           capacity - used,
                           "%s{\"channel\":%u,\"samples\":%" PRIu32
                           ",\"period_us\":%" PRIu64 ",\"tick_ns\":%" PRIu64
                           ",\"min\":%u,\"max\":%u}",
                           used > 48u ? "," : "",
                           (unsigned)profile->channel,
                           profile->observations,
                           profile->mean_period_us,
                           profile->mean_tick_ns,
                           profile->minimum_value,
                           profile->maximum_value);
        if (written < 0 || (size_t)written >= capacity - used) {
            return 0u;
        }
        used += (size_t)written;
    }
    written = snprintf(output + used,
                       capacity - used,
                       "],\"frames\":%" PRIu32 ",\"events\":%" PRIu32 "}",
                       detector->total_frames,
                       detector->total_events);
    if (written < 0 || (size_t)written >= capacity - used) {
        return 0u;
    }
    return used + (size_t)written;
}

const char *detector_event_name(detector_event_kind_t kind)
{
    switch (kind) {
    case DETECTOR_EVENT_NONE: return "none";
    case DETECTOR_EVENT_NEW_CHANNEL: return "new-channel";
    case DETECTOR_EVENT_TIMING_SHIFT: return "timing-shift";
    case DETECTOR_EVENT_STATUS_CHANGE: return "status-change";
    case DETECTOR_EVENT_RANGE_VIOLATION: return "range-violation";
    case DETECTOR_EVENT_ANALOG_MISMATCH: return "analog-mismatch";
    case DETECTOR_EVENT_COUNTER_DISCONTINUITY: return "counter-discontinuity";
    case DETECTOR_EVENT_FINGERPRINT_CHANGE: return "fingerprint-change";
    default: return "unknown";
    }
}
