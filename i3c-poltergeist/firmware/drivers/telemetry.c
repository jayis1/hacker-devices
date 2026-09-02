/*
 * telemetry.c - I3C Poltergeist telemetry renderers
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>

#include "telemetry.h"

static const char *ip_mode_name(ip_mode_t mode)
{
    switch (mode) {
    case IP_MODE_OBSERVE:
        return "observe";
    case IP_MODE_GUARDED:
        return "guarded";
    case IP_MODE_ACTIVE:
        return "active";
    case IP_MODE_BYPASS:
        return "bypass";
    default:
        return "unknown";
    }
}

void ip_telemetry_status_line(const ip_system_t *sys, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0u) {
        return;
    }

    if (sys == NULL) {
        (void)snprintf(buffer, buffer_size, "status=unknown");
        return;
    }

    (void)snprintf(buffer,
                   buffer_size,
                   "mode=%s armed=%u budget=%u captures=%u anomalies=%u temp=%.1fC current=%.1fmA voltage=%.2fV profile=%s",
                   ip_mode_name(sys->status.mode),
                   (unsigned int)sys->status.armed,
                   (unsigned int)sys->status.remaining_write_budget,
                   (unsigned int)sys->capture_count,
                   (unsigned int)sys->status.anomalies,
                   sys->status.board_temp_c,
                   sys->status.target_current_ma,
                   sys->status.target_voltage_v,
                   sys->status.active_profile);
}

void ip_telemetry_render_json(const ip_system_t *sys, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0u) {
        return;
    }

    if (sys == NULL) {
        (void)snprintf(buffer, buffer_size, "{}");
        return;
    }

    (void)snprintf(buffer,
                   buffer_size,
                   "{\"device\":\"%s\",\"author\":\"%s\",\"mode\":\"%s\",\"armed\":%u,\"captures\":%u,\"targets\":%u,\"budget\":%u,\"anomalies\":%u,\"telemetry\":{\"temp_c\":%.1f,\"current_ma\":%.1f,\"voltage_v\":%.2f},\"profile\":\"%s\"}",
                   IP_DEVICE_NAME,
                   IP_AUTHOR,
                   ip_mode_name(sys->status.mode),
                   (unsigned int)sys->status.armed,
                   (unsigned int)sys->capture_count,
                   (unsigned int)sys->target_count,
                   (unsigned int)sys->status.remaining_write_budget,
                   (unsigned int)sys->status.anomalies,
                   sys->status.board_temp_c,
                   sys->status.target_current_ma,
                   sys->status.target_voltage_v,
                   sys->status.active_profile);
}

void ip_telemetry_render_events(const ip_system_t *sys, char *buffer, size_t buffer_size)
{
    size_t i;
    size_t used;

    if (buffer == NULL || buffer_size == 0u) {
        return;
    }

    if (sys == NULL) {
        (void)snprintf(buffer, buffer_size, "[]");
        return;
    }

    used = (size_t)snprintf(buffer, buffer_size, "[");
    for (i = 0u; i < sys->event_count && used + 8u < buffer_size; ++i) {
        int written = snprintf(buffer + used,
                               buffer_size - used,
                               "%s{\"t\":%u,\"code\":%u,\"risk\":%u,\"msg\":\"%s\"}",
                               i == 0u ? "" : ",",
                               (unsigned int)sys->events[i].timestamp_ms,
                               (unsigned int)sys->events[i].code,
                               (unsigned int)sys->events[i].risk,
                               sys->events[i].message);
        if (written < 0) {
            break;
        }
        used += (size_t)written;
    }

    if (used + 2u < buffer_size) {
        (void)snprintf(buffer + used, buffer_size - used, "]");
    } else {
        buffer[buffer_size - 1u] = '\0';
    }
}
