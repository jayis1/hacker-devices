/*
 * drivers/telemetry.c - MDIO Wraith telemetry and reporting
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "telemetry.h"

#include <stdio.h>
#include <string.h>

static float mw_clamp(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

void mw_telemetry_seed(mw_system_t *sys)
{
    if (sys == NULL) {
        return;
    }

    sys->status.board_temp_c = 34.2f;
    sys->status.target_current_ma = 182.0f;
    sys->status.target_voltage_v = 3.29f;
}

void mw_telemetry_update(mw_system_t *sys, uint32_t step)
{
    float drift_temp;
    float drift_current;
    float drift_voltage;

    if (sys == NULL) {
        return;
    }

    drift_temp = ((float)((step % 7u) * 3u) - 8.0f) * 0.22f;
    drift_current = ((float)((step % 9u) * 5u) - 16.0f) * 1.7f;
    drift_voltage = ((float)((step % 5u) * 2u) - 4.0f) * 0.01f;

    sys->status.board_temp_c = mw_clamp(35.0f + drift_temp + (float)sys->status.writes_applied * 0.45f, 27.0f, 79.0f);
    sys->status.target_current_ma = mw_clamp(186.0f + drift_current + (float)sys->status.discovered_phys * 2.8f, 95.0f, 520.0f);
    sys->status.target_voltage_v = mw_clamp(3.28f + drift_voltage - (float)sys->status.anomalies * 0.005f, 2.80f, 3.42f);
}

void mw_telemetry_report(const mw_system_t *sys, char *buffer, size_t length)
{
    const char *mode;

    if (buffer == NULL || length == 0u) {
        return;
    }

    if (sys == NULL) {
        (void)snprintf(buffer, length, "telemetry=unavailable");
        return;
    }

    switch (sys->status.mode) {
    case MW_MODE_PASSIVE:
        mode = "passive";
        break;
    case MW_MODE_GUARDED:
        mode = "guarded";
        break;
    case MW_MODE_ACTIVE:
        mode = "active";
        break;
    default:
        mode = "bypass";
        break;
    }

    (void)snprintf(buffer,
                   length,
                   "mode=%s armed=%u phys=%u writes=%u anomalies=%u temp=%.1fC current=%.1fmA voltage=%.2fV profile=%s",
                   mode,
                   (unsigned int)sys->status.armed,
                   (unsigned int)sys->status.discovered_phys,
                   (unsigned int)sys->status.writes_applied,
                   (unsigned int)sys->status.anomalies,
                   (double)sys->status.board_temp_c,
                   (double)sys->status.target_current_ma,
                   (double)sys->status.target_voltage_v,
                   sys->status.active_profile);
}

void mw_telemetry_render_json(const mw_system_t *sys, char *buffer, size_t length)
{
    size_t offset = 0u;
    size_t i;

    if (buffer == NULL || length == 0u) {
        return;
    }

    if (sys == NULL) {
        (void)snprintf(buffer, length, "{}");
        return;
    }

    offset += (size_t)snprintf(buffer + offset,
                               length - offset,
                               "{\"author\":\"%s\",\"device\":\"%s\",\"profile\":\"%s\",\"armed\":%u,\"temperature_c\":%.2f,\"current_ma\":%.2f,\"voltage_v\":%.2f,\"phys\":[",
                               MW_AUTHOR,
                               MW_DEVICE_NAME,
                               sys->status.active_profile,
                               (unsigned int)sys->status.armed,
                               (double)sys->status.board_temp_c,
                               (double)sys->status.target_current_ma,
                               (double)sys->status.target_voltage_v);

    for (i = 0u; i < sys->phy_count && offset < length; ++i) {
        const mw_phy_t *phy = &sys->phys[i];
        offset += (size_t)snprintf(buffer + offset,
                                   length - offset,
                                   "%s{\"addr\":%u,\"label\":\"%s\",\"link\":%u,\"bmcr\":%u,\"bmsr\":%u}",
                                   i == 0u ? "" : ",",
                                   (unsigned int)phy->phy_addr,
                                   phy->phy_label,
                                   (unsigned int)phy->link_up,
                                   (unsigned int)phy->bmcr,
                                   (unsigned int)phy->bmsr);
    }

    if (offset < length) {
        (void)snprintf(buffer + offset, length - offset, "]}");
    } else {
        buffer[length - 1u] = '\0';
    }
}
