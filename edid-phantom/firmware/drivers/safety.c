/*
 * safety.c - EDID Phantom safety and HPD management
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "safety.h"
#include "log.h"
#include "../registers.h"

#include <stdio.h>
#include <string.h>

static ep_profile_t g_profile;
static uint32_t g_last_pulse_ms;

void safety_init(void) {
    memset(&g_profile, 0, sizeof(g_profile));
    g_last_pulse_ms = 0;
}

void safety_apply_profile(const ep_profile_t *profile) {
    if (profile == NULL) {
        return;
    }
    g_profile = *profile;
    log_event(EP_EVENT_SAFETY, EP_RISK_INFO,
              "safety policy: hpd=%u cec=%u mutate=%u",
              profile->allow_hpd_glitch,
              profile->allow_cec_injection,
              profile->allow_edid_mutation);
}

static float pseudo_temp(uint32_t now_ms) {
    return 31.5f + (float)((now_ms / 70u) % 18u) * 1.35f;
}

static float pseudo_current(uint32_t now_ms) {
    return 210.0f + (float)((now_ms / 55u) % 22u) * 17.25f;
}

void safety_tick(uint32_t now_ms, ep_status_t *status) {
    if (status == NULL) {
        return;
    }

    status->board_temp_c = pseudo_temp(now_ms);
    status->current_draw_ma = pseudo_current(now_ms);
    status->thermal_derated = 0;

    reg_write(REG_SYS_TEMP_CX100, (uint32_t)(status->board_temp_c * 100.0f));
    reg_write(REG_SYS_CURRENT_MA, (uint32_t)status->current_draw_ma);

    if (status->board_temp_c > EP_SAFE_TEMP_C || status->current_draw_ma > EP_SAFE_CURRENT_MA) {
        status->thermal_derated = 1;
        reg_set_bits(REG_SYS_STATUS, SYS_STATUS_THERMAL_DERATE);
        log_event(EP_EVENT_SAFETY, EP_RISK_HIGH,
                  "derate: temp=%.1fC current=%.1fmA",
                  status->board_temp_c,
                  status->current_draw_ma);
    } else {
        reg_clear_bits(REG_SYS_STATUS, SYS_STATUS_THERMAL_DERATE);
    }
}

void safety_pulse_hpd(ep_status_t *status) {
    if (status == NULL) {
        return;
    }

    if (!g_profile.allow_hpd_glitch) {
        status->policy_blocks++;
        reg_set_bits(REG_POLICY_STATUS, 1u);
        log_event(EP_EVENT_POLICY_BLOCK, EP_RISK_MEDIUM, "blocked HPD pulse by profile policy");
        return;
    }

    if (status->hpd_pulses_sent >= EP_HPD_PULSE_LIMIT) {
        status->policy_blocks++;
        reg_set_bits(REG_POLICY_STATUS, 2u);
        log_event(EP_EVENT_POLICY_BLOCK, EP_RISK_HIGH, "blocked HPD pulse due to global pulse limit");
        return;
    }

    status->hpd_asserted = 0;
    reg_clear_bits(REG_HPD_STATUS, HPD_STATUS_ASSERTED);
    status->hpd_asserted = 1;
    reg_set_bits(REG_HPD_STATUS, HPD_STATUS_ASSERTED | HPD_STATUS_PULSE_ARMED);
    status->hpd_pulses_sent++;
    g_last_pulse_ms += g_profile.pulse_spacing_ms;
    log_event(EP_EVENT_HPD_PULSE, EP_RISK_MEDIUM,
              "HPD pulse emitted count=%u spacing=%ums logical_t=%u",
              (unsigned)status->hpd_pulses_sent,
              (unsigned)g_profile.pulse_spacing_ms,
              (unsigned)g_last_pulse_ms);
}

void safety_build_report(char *buffer, size_t length, const ep_status_t *status) {
    if (buffer == NULL || length == 0u || status == NULL) {
        return;
    }

    snprintf(buffer, length,
             "temp=%.1fC current=%.1fmA derated=%u pulses=%u policy_blocks=%u",
             status->board_temp_c,
             status->current_draw_ma,
             status->thermal_derated,
             (unsigned)status->hpd_pulses_sent,
             (unsigned)status->policy_blocks);
}
