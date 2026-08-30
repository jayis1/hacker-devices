/*
 * radio.c - EDID Phantom radio/control plane simulator
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "radio.h"
#include "log.h"
#include "../registers.h"

#include <stdio.h>
#include <string.h>

static ep_profile_t g_profile;
static char g_transcript[EP_MAX_EVENTS][EP_MAX_COMMAND_LENGTH];
static size_t g_transcript_count;

static void transcript_push(const char *line) {
    size_t index = g_transcript_count;
    if (index >= EP_MAX_EVENTS) {
        memmove(&g_transcript[0], &g_transcript[1], sizeof(g_transcript[0]) * (EP_MAX_EVENTS - 1u));
        index = EP_MAX_EVENTS - 1u;
    } else {
        g_transcript_count++;
    }
    snprintf(g_transcript[index], sizeof(g_transcript[index]), "%s", line);
}

void radio_init(void) {
    memset(&g_profile, 0, sizeof(g_profile));
    memset(g_transcript, 0, sizeof(g_transcript));
    g_transcript_count = 0;
    reg_set_bits(REG_RADIO_STATUS, RADIO_STATUS_CONNECTED);
    transcript_push("ble control plane online");
    log_event(EP_EVENT_STREAM_STATE, EP_RISK_INFO, "radio initialized with BLE+Wi-Fi sideband");
}

void radio_set_profile(const ep_profile_t *profile) {
    char line[EP_MAX_COMMAND_LENGTH];
    if (profile == NULL) {
        return;
    }
    g_profile = *profile;
    snprintf(line, sizeof(line), "profile:%s mode=%u ddc=%u", profile->name, (unsigned)profile->mode, (unsigned)profile->ddc_mode);
    transcript_push(line);
    log_event(EP_EVENT_RADIO_COMMAND, EP_RISK_INFO, "radio synced profile '%s'", profile->name);
}

void radio_service(uint32_t now_ms, ep_status_t *status) {
    char line[EP_MAX_COMMAND_LENGTH];
    if (status == NULL) {
        return;
    }

    if ((now_ms % 40u) == 0u) {
        snprintf(line, sizeof(line), "telemetry temp=%.1f current=%.1f ddc=%u cec=%u",
                 status->board_temp_c,
                 status->current_draw_ma,
                 (unsigned)status->ddc_captures,
                 (unsigned)status->cec_frames);
        transcript_push(line);
        reg_set_bits(REG_RADIO_STATUS, RADIO_STATUS_STREAMING);
    }

    if ((now_ms % 90u) == 0u) {
        snprintf(line, sizeof(line), "ui suggestion scenario=%s mutation=%u pulses=%u",
                 g_profile.name,
                 (unsigned)status->mutation_enabled,
                 (unsigned)status->hpd_pulses_sent);
        transcript_push(line);
        log_event(EP_EVENT_ANALYTICS, EP_RISK_LOW, "radio analytics snapshot emitted");
    }
}

void radio_emit_status(const ep_status_t *status) {
    if (status == NULL) {
        return;
    }

    printf("RADIO STATUS author=%s mode=%u ddc=%u temp=%.1f current=%.1f profile=%s\n",
           EP_AUTHOR,
           (unsigned)status->mode,
           (unsigned)status->ddc_mode,
           status->board_temp_c,
           status->current_draw_ma,
           status->active_profile);
}

void radio_emit_transcript(void) {
    size_t index;
    printf("---- Radio Transcript (%zu lines) ----\n", g_transcript_count);
    for (index = 0; index < g_transcript_count; ++index) {
        printf("%s\n", g_transcript[index]);
    }
}
