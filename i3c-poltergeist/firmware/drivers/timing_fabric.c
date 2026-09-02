/*
 * timing_fabric.c - I3C Poltergeist timing fabric controls
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>

#include "timing_fabric.h"
#include "../registers.h"

void ip_fabric_init(ip_system_t *sys)
{
    if (sys == NULL) {
        return;
    }

    sys->status.fabric_locked = 1u;
}

void ip_fabric_apply_profile(ip_system_t *sys, const ip_profile_t *profile)
{
    if (sys == NULL || profile == NULL) {
        return;
    }

    sys->status.fabric_locked = (profile->jitter_ns <= 18u) ? 1u : 0u;
    if (!sys->status.fabric_locked) {
        sys->status.anomalies++;
    }
}

int ip_fabric_match_trigger(const ip_profile_t *profile, uint16_t ccc)
{
    if (profile == NULL) {
        return 0;
    }

    return (profile->trigger_ccc == IP_TRIGGER_ANY_CCC || profile->trigger_ccc == ccc) ? 1 : 0;
}

uint32_t ip_fabric_estimate_delay_ns(const ip_profile_t *profile, uint16_t ccc, uint8_t target_index)
{
    uint32_t base;

    if (profile == NULL) {
        return 0u;
    }

    base = (uint32_t)profile->jitter_ns * 4u;
    base += (uint32_t)((ccc & 0x0Fu) * 3u);
    base += (uint32_t)(target_index * 5u);

    if (ccc == IP_CCC_ENTDAA || ccc == IP_CCC_RSTDAA) {
        base += 12u;
    }
    if (ccc == IP_CCC_ENEC || ccc == IP_CCC_DISEC) {
        base += 6u;
    }

    return base;
}

int ip_fabric_should_mirror_ccc(const ip_profile_t *profile, uint16_t ccc, uint8_t target_index)
{
    if (profile == NULL) {
        return 0;
    }

    if (!profile->allow_ccc_suppression) {
        return 0;
    }

    if (profile->target_selector != IP_TARGET_ANY && profile->target_selector != target_index) {
        return 0;
    }

    return (ccc == IP_CCC_ENEC || ccc == IP_CCC_SETMRL || ccc == IP_CCC_SETMWL) ? 1 : 0;
}

void ip_fabric_record_health(ip_system_t *sys, char *buffer, size_t buffer_size)
{
    const char *integrity;
    const char *link;

    if (buffer == NULL || buffer_size == 0u) {
        return;
    }

    if (sys == NULL) {
        (void)snprintf(buffer, buffer_size, "fabric=unknown");
        return;
    }

    integrity = sys->status.fabric_locked ? "locked" : "drift";
    if (sys->status.mode == IP_MODE_BYPASS) {
        link = "relay-bypass";
    } else if (sys->status.mode == IP_MODE_ACTIVE) {
        link = "inline-active";
    } else if (sys->status.mode == IP_MODE_GUARDED) {
        link = "inline-armed";
    } else {
        link = "inline-observe";
    }

    (void)snprintf(buffer,
                   buffer_size,
                   "fabric=%s mode=%s temp=%.1fC current=%.1fmA voltage=%.2fV anomalies=%u",
                   integrity,
                   link,
                   sys->status.board_temp_c,
                   sys->status.target_current_ma,
                   sys->status.target_voltage_v,
                   (unsigned int)sys->status.anomalies);
}
