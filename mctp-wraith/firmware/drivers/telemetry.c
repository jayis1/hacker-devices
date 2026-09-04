/*
 * telemetry.c
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include "telemetry.h"
#include "../board.h"

void telemetry_init(void) {
    g_runtime.telemetry.rail_mv = 3300U;
    g_runtime.telemetry.rail_ma = 142U;
    g_runtime.telemetry.board_temp_c = 29;
    g_runtime.telemetry.capture_count = 0U;
    g_runtime.telemetry.mutation_count = 0U;
    g_runtime.telemetry.dropped_count = 0U;
    g_runtime.telemetry.alert_count = 0U;
    g_runtime.telemetry.signed_bundle_count = 0U;
}

void telemetry_tick(void) {
    g_runtime.telemetry.rail_ma = 142U + (g_now_ms % 17U);
    g_runtime.telemetry.board_temp_c = 29 + (int32_t)((g_now_ms / 120U) % 7U);
}

void telemetry_note_capture(void) {
    g_runtime.telemetry.capture_count++;
}

void telemetry_note_mutation(void) {
    g_runtime.telemetry.mutation_count++;
}

void telemetry_note_drop(void) {
    g_runtime.telemetry.dropped_count++;
}

void telemetry_note_alert(void) {
    g_runtime.telemetry.alert_count++;
}

void telemetry_note_signed_bundle(void) {
    g_runtime.telemetry.signed_bundle_count++;
}

void telemetry_print(void) {
    printf("[telemetry] rail=%umV current=%umA temp=%ldC captures=%u mutations=%u drops=%u alerts=%u signed=%u\n",
           g_runtime.telemetry.rail_mv,
           g_runtime.telemetry.rail_ma,
           (long)g_runtime.telemetry.board_temp_c,
           g_runtime.telemetry.capture_count,
           g_runtime.telemetry.mutation_count,
           g_runtime.telemetry.dropped_count,
           g_runtime.telemetry.alert_count,
           g_runtime.telemetry.signed_bundle_count);
}
