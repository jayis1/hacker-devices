/*
 * MoCA Phantom Storage Simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>
#include <string.h>

#include "storage.h"

static mph_runtime_config_t g_last_config;
static mph_frame_t g_log[MPH_MAX_EVENTS * 2u];
static size_t g_log_count;

void mph_storage_init(void) {
    memset(&g_last_config, 0, sizeof(g_last_config));
    memset(g_log, 0, sizeof(g_log));
    g_log_count = 0u;
}

void mph_storage_store_config(const mph_runtime_config_t *config) {
    if (config == NULL) {
        return;
    }
    g_last_config = *config;
}

void mph_storage_append_captures(const mph_frame_t *frames, size_t count, mph_metrics_t *metrics) {
    size_t i;
    if (frames == NULL) {
        return;
    }

    for (i = 0u; i < count; ++i) {
        if (g_log_count < (sizeof(g_log) / sizeof(g_log[0]))) {
            g_log[g_log_count++] = frames[i];
        } else {
            memmove(&g_log[0], &g_log[1], (g_log_count - 1u) * sizeof(g_log[0]));
            g_log[g_log_count - 1u] = frames[i];
        }
    }

    if (metrics != NULL) {
        metrics->storage_records = (uint32_t)g_log_count;
    }
}

size_t mph_storage_log_count(void) {
    return g_log_count;
}

void mph_storage_dump_recent(size_t count) {
    size_t start = 0u;
    size_t i;

    if (count < g_log_count) {
        start = g_log_count - count;
    }

    printf("[storage] profile=%s mode=%d records=%zu\n",
           g_last_config.active_profile,
           (int)g_last_config.mode,
           g_log_count);

    for (i = start; i < g_log_count; ++i) {
        printf("  rec=%zu ts=%lu src=%u dst=%u class=%d ch=%u rate=%lu key=0x%lx anomaly=%s\n",
               i,
               (unsigned long)g_log[i].timestamp_ms,
               g_log[i].src_node,
               g_log[i].dst_node,
               (int)g_log[i].traffic_class,
               g_log[i].channel,
               (unsigned long)g_log[i].phy_rate_mbps,
               (unsigned long)g_log[i].privacy_key_id,
               g_log[i].anomaly ? "yes" : "no");
    }
}
