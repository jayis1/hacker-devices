/*
 * cec.c - EDID Phantom CEC subsystem
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "cec.h"
#include "log.h"
#include "../registers.h"

#include <stdio.h>
#include <string.h>

static ep_profile_t g_profile;
static ep_cec_frame_t g_frames[EP_MAX_CEC_QUEUE];
static size_t g_frame_count;

static void push_frame(uint32_t now_ms, uint8_t initiator, uint8_t destination, uint8_t opcode, const uint8_t *payload, uint8_t length) {
    ep_cec_frame_t *frame;
    size_t index = g_frame_count;
    if (length > sizeof(g_frames[0].payload)) {
        length = sizeof(g_frames[0].payload);
    }

    if (index >= EP_MAX_CEC_QUEUE) {
        memmove(&g_frames[0], &g_frames[1], sizeof(g_frames[0]) * (EP_MAX_CEC_QUEUE - 1u));
        index = EP_MAX_CEC_QUEUE - 1u;
    } else {
        g_frame_count++;
    }

    frame = &g_frames[index];
    memset(frame, 0, sizeof(*frame));
    frame->timestamp_ms = now_ms;
    frame->initiator = initiator;
    frame->destination = destination;
    frame->opcode = opcode;
    frame->length = length;
    frame->valid = 1u;
    if (payload != NULL && length > 0u) {
        memcpy(frame->payload, payload, length);
    }

    reg_set_bits(REG_CEC_STATUS, CEC_STATUS_RX_READY);
    reg_write(REG_CEC_LAST_OPCODE, opcode);
}

void cec_init(void) {
    memset(&g_profile, 0, sizeof(g_profile));
    memset(g_frames, 0, sizeof(g_frames));
    g_frame_count = 0;
    log_event(EP_EVENT_CEC_FRAME, EP_RISK_INFO, "cec subsystem initialized");
}

void cec_set_profile(const ep_profile_t *profile) {
    if (profile == NULL) {
        return;
    }
    g_profile = *profile;
    if (profile->allow_cec_injection) {
        reg_set_bits(REG_SYS_STATUS, SYS_STATUS_CEC_GUARD_ENABLED);
        reg_write(REG_CEC_CONTROL, 1u);
        log_event(EP_EVENT_CEC_FRAME, EP_RISK_LOW, "cec injection enabled limit=%u", profile->cec_rate_limit);
    } else {
        reg_clear_bits(REG_SYS_STATUS, SYS_STATUS_CEC_GUARD_ENABLED);
        reg_write(REG_CEC_CONTROL, 0u);
        log_event(EP_EVENT_CEC_FRAME, EP_RISK_INFO, "cec injection disabled by profile");
    }
}

void cec_service(uint32_t now_ms, ep_status_t *status) {
    uint8_t payload[3];
    uint8_t opcode;
    if (status == NULL) {
        return;
    }

    opcode = (uint8_t)(((now_ms / 30u) % 4u) == 0u ? 0x82u : 0x90u);
    payload[0] = (uint8_t)(0x10u + ((now_ms / 50u) % 5u));
    payload[1] = (uint8_t)(0x20u + ((now_ms / 70u) % 7u));
    payload[2] = (uint8_t)(status->ddc_captures & 0xFFu);

    push_frame(now_ms, 0x01u, 0x00u, opcode, payload, sizeof(payload));
    status->cec_frames++;
    log_event(EP_EVENT_CEC_FRAME, EP_RISK_INFO,
              "cec rx opcode=0x%02X frames=%u guard=%u",
              opcode,
              (unsigned)status->cec_frames,
              (unsigned)g_profile.allow_cec_injection);

    if (g_profile.allow_cec_injection && (status->cec_frames % (uint32_t)g_profile.cec_rate_limit) == 0u) {
        cec_inject_guarded_ping(status);
    }
}

void cec_inject_guarded_ping(ep_status_t *status) {
    uint8_t payload[2] = { 0x04u, 0x00u };
    if (status == NULL) {
        return;
    }

    if (!g_profile.allow_cec_injection) {
        status->policy_blocks++;
        reg_set_bits(REG_CEC_STATUS, CEC_STATUS_GUARD_BLOCK);
        log_event(EP_EVENT_POLICY_BLOCK, EP_RISK_MEDIUM, "blocked CEC ping by profile");
        return;
    }

    push_frame(status->uptime_ms, 0x04u, 0x0Fu, 0x44u, payload, sizeof(payload));
    reg_set_bits(REG_CEC_STATUS, CEC_STATUS_TX_READY);
    log_event(EP_EVENT_CEC_FRAME, EP_RISK_MEDIUM,
              "cec tx opcode=0x44 key=0x%02X after frame=%u",
              payload[0],
              (unsigned)status->cec_frames);
}

void cec_build_report(char *buffer, size_t length) {
    if (buffer == NULL || length == 0u) {
        return;
    }

    snprintf(buffer, length,
             "cec_frames=%u injection=%u last_opcode=0x%02X",
             (unsigned)g_frame_count,
             (unsigned)g_profile.allow_cec_injection,
             (unsigned)reg_read(REG_CEC_LAST_OPCODE));
}

const ep_cec_frame_t *cec_frame(size_t index) {
    if (index >= g_frame_count) {
        return NULL;
    }
    return &g_frames[index];
}

size_t cec_frame_count(void) {
    return g_frame_count;
}
