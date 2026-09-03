/*
 * SmartPack Phantom export/radio model
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "radio.h"
#include "battery_profile.h"
#include "auth_chip.h"
#include "telemetry.h"

void radio_init(sp_radio_state_t *radio) {
    if (radio == 0) {
        return;
    }
    radio->frames_sent = 0u;
    radio->exports_generated = 0u;
    radio->dropped_frames = 0u;
    snprintf(radio->last_frame, sizeof(radio->last_frame), "boot");
}

void radio_emit_status(sp_runtime_t *runtime, char *buffer, size_t buffer_len) {
    if (runtime == 0 || buffer == 0 || buffer_len == 0u) {
        return;
    }
    snprintf(buffer,
             buffer_len,
             "{\"profile\":\"%s\",\"soc\":%u,\"tempC\":%d,\"auth\":\"%s\",\"health\":\"%s\",\"mutations\":%u}",
             battery_profile_name(runtime->profile.kind),
             battery_profile_relative_soc(&runtime->profile),
             runtime->profile.temperature_c,
             auth_chip_mode_name(runtime->auth.mode),
             telemetry_health_string(runtime),
             runtime->bus.mutated_replies);
    snprintf(runtime->radio.last_frame, sizeof(runtime->radio.last_frame), "%s", buffer);
    runtime->radio.frames_sent++;
}

void radio_emit_event_bundle(const sp_event_t *events,
                             size_t event_count,
                             sp_runtime_t *runtime,
                             char *buffer,
                             size_t buffer_len) {
    size_t i;
    size_t used;
    if (events == 0 || runtime == 0 || buffer == 0 || buffer_len == 0u) {
        return;
    }
    used = (size_t)snprintf(buffer,
                            buffer_len,
                            "{\"author\":\"jayis1\",\"exports\":%u,\"events\":[",
                            runtime->radio.exports_generated + 1u);
    for (i = 0u; i < event_count && i < 4u && used + 32u < buffer_len; ++i) {
        used += (size_t)snprintf(buffer + used,
                                 buffer_len - used,
                                 "%s{\"t\":%u,\"type\":%u,\"text\":\"%s\"}",
                                 i == 0u ? "" : ",",
                                 events[i].timestamp_ms,
                                 (unsigned)events[i].type,
                                 events[i].text);
    }
    if (used + 3u < buffer_len) {
        snprintf(buffer + used, buffer_len - used, "]}");
    }
    runtime->radio.exports_generated++;
    snprintf(runtime->radio.last_frame, sizeof(runtime->radio.last_frame), "bundle:%u", runtime->radio.exports_generated);
}
