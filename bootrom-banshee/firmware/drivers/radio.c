/*
 * BootROM Banshee radio/control abstraction
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "radio.h"

static bb_radio_command_t g_queue[BB_MAX_COMMANDS];
static uint32_t g_head;
static uint32_t g_tail;

void radio_init(void) {
    memset(g_queue, 0, sizeof(g_queue));
    g_head = 0u;
    g_tail = 0u;
}

void radio_queue_demo_command(bb_radio_command_kind_t kind, uint32_t arg0, uint32_t arg1, const char *text) {
    bb_radio_command_t *slot = &g_queue[g_tail % BB_MAX_COMMANDS];
    slot->kind = kind;
    slot->arg0 = arg0;
    slot->arg1 = arg1;
    snprintf(slot->text, sizeof(slot->text), "%s", text ? text : "");
    g_tail++;
}

bool radio_poll_command(bb_radio_command_t *cmd) {
    if (cmd == NULL || g_head == g_tail) {
        return false;
    }
    *cmd = g_queue[g_head % BB_MAX_COMMANDS];
    g_head++;
    return true;
}

void radio_publish_status(const bb_runtime_t *runtime) {
    if (runtime == NULL) {
        return;
    }
    printf("[radio] scenario=%s active=%u triggered=%u overlay=%s current=%umA temp=%ldC mode=%u\n",
           runtime->scenario.name,
           runtime->scenario.active ? 1u : 0u,
           runtime->scenario.triggered ? 1u : 0u,
           runtime->active_overlay,
           runtime->telemetry.rail_current_ma,
           (long)runtime->telemetry.board_temp_c,
           runtime->bus_mode);
}
