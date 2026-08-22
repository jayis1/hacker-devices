/*
 * BLE/Wi-Fi control plane simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "radio.h"

#include <stdio.h>

static bool g_enabled;
static bool g_connected;
static uint32_t g_last_tick;

void eph_radio_init(const eph_runtime_config_t *config) {
    g_enabled = (config != NULL) ? config->radio_enabled : true;
    g_connected = g_enabled;
    g_last_tick = 0u;
}

void eph_radio_set_enabled(bool enabled) {
    g_enabled = enabled;
    if (!enabled) {
        g_connected = false;
    } else {
        g_connected = true;
    }
}

bool eph_radio_is_connected(void) {
    return g_enabled && g_connected;
}

void eph_radio_send_status(const eph_runtime_config_t *config, const eph_metrics_t *metrics) {
    if (!eph_radio_is_connected() || config == NULL || metrics == NULL) {
        return;
    }

    printf("[radio] status role=%d safe=%s logging=%s frames=%lu modified=%lu rules=%lu wc_faults=%lu\n",
           (int)config->role,
           config->safe_mode ? "yes" : "no",
           config->logging_enabled ? "yes" : "no",
           (unsigned long)metrics->frames_seen,
           (unsigned long)metrics->frames_modified,
           (unsigned long)metrics->rules_triggered,
           (unsigned long)metrics->working_counter_faults);
}

void eph_radio_send_captures(const eph_capture_t *captures, size_t count) {
    size_t i;
    if (!eph_radio_is_connected() || captures == NULL) {
        return;
    }

    for (i = 0u; i < count; ++i) {
        printf("[radio] capture cycle=%lu slave=%u idx=0x%04X:%u %ld->%ld modified=%s\n",
               (unsigned long)captures[i].cycle_counter,
               captures[i].slave_address,
               captures[i].index,
               captures[i].subindex,
               (long)captures[i].value_before,
               (long)captures[i].value_after,
               captures[i].modified ? "yes" : "no");
    }
}

void eph_radio_tick(uint32_t epoch_ms, eph_metrics_t *metrics) {
    if (!eph_radio_is_connected() || metrics == NULL) {
        return;
    }
    if ((epoch_ms - g_last_tick) >= 250u) {
        metrics->radio_packets += 1u;
        g_last_tick = epoch_ms;
    }
}
