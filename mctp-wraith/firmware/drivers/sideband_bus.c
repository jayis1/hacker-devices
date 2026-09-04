/*
 * sideband_bus.c
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "sideband_bus.h"

static mw_bus_state_t g_bus;
static mw_message_t g_captures[48];
static char g_capture_reason[48][24];
static size_t g_capture_count;

static mw_message_t make_message(uint8_t src, uint8_t dst, uint8_t tag, uint8_t cmd,
                                 uint8_t integrity, const uint8_t *payload, uint8_t len) {
    mw_message_t message;
    memset(&message, 0, sizeof(message));
    message.eid_src = src;
    message.eid_dst = dst;
    message.tag = tag;
    message.command_code = cmd;
    message.integrity = integrity;
    message.payload_len = len;
    if (payload != NULL && len > 0U) {
        memcpy(message.payload, payload, len);
    }
    return message;
}

void sideband_bus_init(void) {
    memset(&g_bus, 0, sizeof(g_bus));
    g_bus.capture_enabled = true;
    g_bus.mutate_enabled = false;
    g_bus.delay_budget_ms = 0U;
    g_capture_count = 0U;
}

void sideband_bus_set_kind(mw_bus_kind_t kind) {
    g_runtime.bus_kind = kind;
}

void sideband_bus_enable_mutation(bool enabled) {
    g_bus.mutate_enabled = enabled;
}

void sideband_bus_enable_capture(bool enabled) {
    g_bus.capture_enabled = enabled;
}

void sideband_bus_set_delay_budget(uint32_t delay_ms) {
    g_bus.delay_budget_ms = delay_ms;
}

void sideband_bus_seed_demo_traffic(void) {
    static const uint8_t spdm_get_version[] = {0x10U, 0x84U, 0x00U, 0x00U};
    static const uint8_t pldm_get_tid[] = {0x01U, 0x02U, 0xF1U};
    static const uint8_t pldm_fw_stage[] = {0x05U, 0x80U, 0x02U, 0x00U, 0x11U, 0x22U};
    static const uint8_t sensor_reading[] = {0x02U, 0x11U, 0x00U, 0x00U, 0x2AU};
    static const uint8_t endpoint_advert[] = {0x7EU, 0x03U, 0x01U, 0x09U};
    static const uint8_t clone_probe[] = {0x3AU, 0x44U, 0x55U, 0x66U};

    g_bus.count = 0U;
    g_bus.read_index = 0U;
    g_bus.ring[g_bus.count++] = make_message(8U, 20U, 1U, 0x84U, 1U, spdm_get_version, (uint8_t)sizeof(spdm_get_version));
    g_bus.ring[g_bus.count++] = make_message(20U, 8U, 1U, 0x04U, 1U, pldm_get_tid, (uint8_t)sizeof(pldm_get_tid));
    g_bus.ring[g_bus.count++] = make_message(8U, 33U, 2U, 0x91U, 1U, pldm_fw_stage, (uint8_t)sizeof(pldm_fw_stage));
    g_bus.ring[g_bus.count++] = make_message(33U, 8U, 2U, 0x50U, 0U, sensor_reading, (uint8_t)sizeof(sensor_reading));
    g_bus.ring[g_bus.count++] = make_message(44U, 8U, 3U, 0x0AU, 0U, endpoint_advert, (uint8_t)sizeof(endpoint_advert));
    g_bus.ring[g_bus.count++] = make_message(8U, 44U, 3U, 0x92U, 1U, clone_probe, (uint8_t)sizeof(clone_probe));
}

bool sideband_bus_next_message(mw_message_t *message) {
    if (message == NULL || g_bus.read_index >= g_bus.count) {
        return false;
    }

    *message = g_bus.ring[g_bus.read_index++];
    g_bus.seq++;
    if (g_bus.delay_budget_ms > 0U) {
        g_now_ms += g_bus.delay_budget_ms;
    }
    return true;
}

void sideband_bus_inject_message(const mw_message_t *message) {
    if (message == NULL || g_bus.count >= (sizeof(g_bus.ring) / sizeof(g_bus.ring[0]))) {
        return;
    }

    g_bus.ring[g_bus.count++] = *message;
    mw_log_event(MW_EVENT_MUTATION, message->eid_src, message->eid_dst, "message injected into sideband ring");
}

void sideband_bus_capture_message(const mw_message_t *message, const char *reason) {
    if (!g_bus.capture_enabled || message == NULL || g_capture_count >= (sizeof(g_captures) / sizeof(g_captures[0]))) {
        return;
    }

    g_captures[g_capture_count] = *message;
    snprintf(g_capture_reason[g_capture_count], sizeof(g_capture_reason[g_capture_count]), "%s", reason != NULL ? reason : "capture");
    g_capture_count++;
}

void sideband_bus_print_capture_summary(void) {
    printf("[capture] total=%zu bus=%s mutate=%u delay=%ums\n",
           g_capture_count,
           mw_bus_name(g_runtime.bus_kind),
           g_bus.mutate_enabled ? 1U : 0U,
           g_bus.delay_budget_ms);

    for (size_t i = 0; i < g_capture_count; ++i) {
        const mw_message_t *message = &g_captures[i];
        printf("  [%02zu] %s src=%u dst=%u cmd=0x%02x len=%u integ=%u\n",
               i,
               g_capture_reason[i],
               message->eid_src,
               message->eid_dst,
               message->command_code,
               message->payload_len,
               message->integrity);
    }
}

const mw_bus_state_t *sideband_bus_state(void) {
    return &g_bus;
}
