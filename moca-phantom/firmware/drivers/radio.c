/*
 * MoCA Phantom Radio Telemetry Simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>

#include "radio.h"

static bool g_ble_ready;
static bool g_wifi_ready;

void mph_radio_init(const mph_runtime_config_t *config) {
    g_ble_ready = config != NULL && config->radio_enabled;
    g_wifi_ready = config != NULL && config->radio_enabled;
}

void mph_radio_tick(uint32_t epoch_ms, mph_metrics_t *metrics) {
    if (metrics == NULL) {
        return;
    }

    if (g_ble_ready && (epoch_ms % 200u) == 0u) {
        metrics->radio_packets += 1u;
    }
    if (g_wifi_ready && (epoch_ms % 300u) == 0u) {
        metrics->radio_packets += 1u;
    }
}

void mph_radio_send_status(const mph_runtime_config_t *config,
                           const mph_metrics_t *metrics,
                           const mph_spectrum_snapshot_t *spectrum) {
    if (config == NULL || metrics == NULL || spectrum == NULL || !config->radio_enabled) {
        return;
    }

    printf("[radio] profile=%s mode=%d seen=%lu modified=%lu alerts=%lu strongest_ch=%u strongest_db=%d leakage=%s\n",
           config->active_profile,
           (int)config->mode,
           (unsigned long)metrics->frames_seen,
           (unsigned long)metrics->frames_modified,
           (unsigned long)metrics->alerts_raised,
           spectrum->strongest_channel,
           (int)spectrum->strongest_energy_db,
           spectrum->leakage_suspected ? "yes" : "no");
}

void mph_radio_send_captures(const mph_frame_t *frames, size_t count, mph_metrics_t *metrics) {
    size_t i;
    if (frames == NULL || metrics == NULL || count == 0u) {
        return;
    }

    printf("[radio] uplink %zu captures\n", count);
    for (i = 0u; i < count && i < 3u; ++i) {
        printf("         cap ts=%lu src=%u dst=%u type=%d anomaly=%s\n",
               (unsigned long)frames[i].timestamp_ms,
               frames[i].src_node,
               frames[i].dst_node,
               (int)frames[i].traffic_class,
               frames[i].anomaly ? "yes" : "no");
    }
    metrics->radio_packets += (uint32_t)count;
}
