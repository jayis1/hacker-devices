/*
 * BootROM Banshee glitch engine
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "glitch_engine.h"

static uint32_t g_delay_us;
static uint32_t g_truncation;
static uint32_t g_strap_mask;

void glitch_engine_init(void) {
    g_delay_us = 0u;
    g_truncation = 0u;
    g_strap_mask = 0u;
}

void glitch_engine_set_delay_us(uint32_t delay_us) {
    g_delay_us = delay_us;
}

void glitch_engine_set_truncation(uint32_t len) {
    g_truncation = len;
}

void glitch_engine_set_strap_mask(uint32_t mask) {
    g_strap_mask = mask;
}

uint32_t glitch_engine_delay_us(void) {
    return g_delay_us;
}

uint32_t glitch_engine_truncation(void) {
    return g_truncation;
}

uint32_t glitch_engine_strap_mask(void) {
    return g_strap_mask;
}

const char *glitch_engine_strap_summary(void) {
    switch (g_strap_mask & 0x3u) {
        case 0u: return "normal-boot";
        case 1u: return "uart-recovery-bias";
        case 2u: return "usb-download-bias";
        default: return "vendor-fallback-bias";
    }
}

static void apply_delay_signature(bb_spi_frame_t *frame) {
    if (g_delay_us == 0u || frame->data_len == 0u) {
        return;
    }
    frame->data[BB_MAX_CAPTURE_WORDS - 1u] ^= (g_delay_us << 4u);
    frame->mutated = true;
}

static void apply_strap_signature(bb_runtime_t *runtime, bb_spi_frame_t *frame) {
    if (g_strap_mask == 0u) {
        runtime->flags &= ~BB_FLAG_STRAP_ACTIVE;
        return;
    }
    runtime->flags |= BB_FLAG_STRAP_ACTIVE;
    if (frame->opcode == 0x9Fu || frame->addr == 0u) {
        frame->data[0] ^= (0xABCD0000u | g_strap_mask);
        frame->mutated = true;
    }
}

static void apply_truncation_signature(bb_spi_frame_t *frame) {
    if (g_truncation == 0u) {
        return;
    }
    if (frame->data_len > g_truncation) {
        frame->data_len = (uint8_t)g_truncation;
        frame->mutated = true;
    }
}

void glitch_engine_process(bb_runtime_t *runtime, bb_spi_frame_t *frame, uint32_t now_ms, char *status_text, uint32_t status_len) {
    if (runtime == NULL || frame == NULL || status_text == NULL || status_len == 0u) {
        return;
    }

    apply_delay_signature(frame);
    apply_strap_signature(runtime, frame);
    apply_truncation_signature(frame);

    if (runtime->mutation_enabled && runtime->scenario.active && runtime->scenario.triggered) {
        snprintf(status_text, status_len,
                 "scenario=%s delay=%uus trunc=%u strap=%s t=%ums",
                 runtime->scenario.name,
                 g_delay_us,
                 g_truncation,
                 glitch_engine_strap_summary(),
                 now_ms);
    } else {
        snprintf(status_text, status_len,
                 "passive delay=%uus trunc=%u strap=%s",
                 g_delay_us,
                 g_truncation,
                 glitch_engine_strap_summary());
    }
}
