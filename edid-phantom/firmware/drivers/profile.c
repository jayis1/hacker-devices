/*
 * profile.c - EDID Phantom operating profiles
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "profile.h"
#include "log.h"

#include <stdio.h>
#include <string.h>

static ep_profile_t g_profiles[] = {
    {
        .name = "conference-mirror",
        .mode = EP_MODE_INLINE,
        .ddc_mode = EP_DDC_PROXY,
        .allow_hpd_glitch = 1,
        .allow_cec_injection = 1,
        .allow_edid_mutation = 1,
        .preserve_vendor_block = 1,
        .pulse_spacing_ms = 80,
        .pulse_count = 3,
        .mutate_seed = 0x1347u,
        .preferred_audio_channels = 8,
        .max_luminance_hint = 180,
        .cec_rate_limit = 4
    },
    {
        .name = "kiosk-reality-check",
        .mode = EP_MODE_GHOST,
        .ddc_mode = EP_DDC_EMULATE,
        .allow_hpd_glitch = 1,
        .allow_cec_injection = 0,
        .allow_edid_mutation = 1,
        .preserve_vendor_block = 0,
        .pulse_spacing_ms = 120,
        .pulse_count = 2,
        .mutate_seed = 0x2288u,
        .preferred_audio_channels = 2,
        .max_luminance_hint = 110,
        .cec_rate_limit = 2
    },
    {
        .name = "boardroom-observer",
        .mode = EP_MODE_MONITOR,
        .ddc_mode = EP_DDC_SNIFF,
        .allow_hpd_glitch = 0,
        .allow_cec_injection = 0,
        .allow_edid_mutation = 0,
        .preserve_vendor_block = 1,
        .pulse_spacing_ms = 0,
        .pulse_count = 0,
        .mutate_seed = 0x0001u,
        .preferred_audio_channels = 2,
        .max_luminance_hint = 100,
        .cec_rate_limit = 1
    },
    {
        .name = "training-lab-sandbox",
        .mode = EP_MODE_HARDENED,
        .ddc_mode = EP_DDC_PROXY,
        .allow_hpd_glitch = 1,
        .allow_cec_injection = 1,
        .allow_edid_mutation = 1,
        .preserve_vendor_block = 0,
        .pulse_spacing_ms = 160,
        .pulse_count = 4,
        .mutate_seed = 0x55AAu,
        .preferred_audio_channels = 6,
        .max_luminance_hint = 150,
        .cec_rate_limit = 3
    }
};

void profile_init(void) {
    log_event(EP_EVENT_PROFILE_LOADED, EP_RISK_INFO, "profile catalog initialized (%zu entries)", profile_count());
}

const ep_profile_t *profile_default(void) {
    return &g_profiles[0];
}

const ep_profile_t *profile_find(const char *name) {
    size_t index;
    if (name == NULL) {
        return profile_default();
    }

    for (index = 0; index < profile_count(); ++index) {
        if (strcmp(g_profiles[index].name, name) == 0) {
            return &g_profiles[index];
        }
    }

    log_event(EP_EVENT_ALERT, EP_RISK_LOW, "profile '%s' not found, defaulting", name);
    return profile_default();
}

size_t profile_count(void) {
    return sizeof(g_profiles) / sizeof(g_profiles[0]);
}

const ep_profile_t *profile_at(size_t index) {
    if (index >= profile_count()) {
        return NULL;
    }
    return &g_profiles[index];
}
