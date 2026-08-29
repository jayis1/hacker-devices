/*
 * Type-C Specter operator radio abstraction
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "radio.h"
#include "vbus_meter.h"

#define TS_RADIO_QUEUE_DEPTH 16u

static ts_radio_command_t g_queue[TS_RADIO_QUEUE_DEPTH];
static uint32_t g_head;
static uint32_t g_tail;

void radio_init(void) {
    memset(g_queue, 0, sizeof(g_queue));
    g_head = 0u;
    g_tail = 0u;
}

void radio_queue_demo_command(ts_radio_command_kind_t kind, uint32_t arg0, const char *text) {
    ts_radio_command_t *slot = &g_queue[g_head % TS_RADIO_QUEUE_DEPTH];
    slot->kind = kind;
    slot->arg0 = arg0;
    snprintf(slot->text, sizeof(slot->text), "%s", text ? text : "");
    g_head++;
}

bool radio_poll_command(ts_radio_command_t *out) {
    if (g_tail == g_head || out == NULL) {
        return false;
    }
    *out = g_queue[g_tail % TS_RADIO_QUEUE_DEPTH];
    g_tail++;
    return true;
}

void radio_publish_status(const ts_runtime_t *runtime) {
    if (runtime == NULL) {
        return;
    }
    printf("[radio] profile=%s scenario=%s flags=0x%02x vbus=%u limit=%u\n",
           runtime->active_profile,
           runtime->scenario.name,
           runtime->flags,
           runtime->power.target_vbus_mv,
           vbus_meter_get_current_limit());
}
