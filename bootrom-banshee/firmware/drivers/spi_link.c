/*
 * BootROM Banshee SPI link model
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "spi_link.h"
#include "overlay_store.h"

static bool g_mutation_enabled;
static bb_bus_mode_t g_bus_mode;
static uint32_t g_forced_jedec;
static uint32_t g_truncation_len;
static uint32_t g_delay_us;
static char g_overlay_name[BB_PROFILE_NAME_LEN];
static uint32_t g_sequence;

static void fill_words(bb_spi_frame_t *frame, uint32_t seed) {
    for (uint32_t i = 0; i < BB_MAX_CAPTURE_WORDS; ++i) {
        frame->data[i] = seed ^ (0x11111111u * (i + 1u));
    }
}

void spi_link_init(void) {
    g_mutation_enabled = false;
    g_bus_mode = BB_BUS_MODE_QSPI;
    g_forced_jedec = 0;
    g_truncation_len = 0;
    g_delay_us = 0;
    g_sequence = 0;
    snprintf(g_overlay_name, sizeof(g_overlay_name), "transparent");
}

void spi_link_set_mutation_enabled(bool enabled) {
    g_mutation_enabled = enabled;
}

void spi_link_set_bus_mode(bb_bus_mode_t mode) {
    g_bus_mode = mode;
}

void spi_link_force_jedec_id(uint32_t value) {
    g_forced_jedec = value;
}

void spi_link_set_truncation(uint32_t len) {
    g_truncation_len = len;
}

void spi_link_set_overlay_name(const char *name) {
    snprintf(g_overlay_name, sizeof(g_overlay_name), "%.31s", (name && name[0] != '\0') ? name : "transparent");
}

void spi_link_set_delay_us(uint32_t delay_us) {
    g_delay_us = delay_us;
}

const char *spi_link_bus_mode_name(bb_bus_mode_t mode) {
    switch (mode) {
        case BB_BUS_MODE_SPI: return "spi";
        case BB_BUS_MODE_DSPI: return "dspi";
        case BB_BUS_MODE_QSPI: return "qspi";
        default: return "unknown";
    }
}

static void make_jedec_frame(bb_spi_frame_t *frame) {
    frame->opcode = 0x9Fu;
    frame->addr = 0u;
    frame->lane_mode = (uint8_t)g_bus_mode;
    frame->data_len = 3u;
    frame->write_phase = false;
    frame->mutated = false;
    const uint32_t id = g_forced_jedec ? g_forced_jedec : 0x20BA19u;
    frame->data[0] = id;
    for (uint32_t i = 1; i < BB_MAX_CAPTURE_WORDS; ++i) {
        frame->data[i] = 0u;
    }
    if (g_mutation_enabled && g_forced_jedec != 0u) {
        frame->mutated = true;
    }
}

static void make_header_frame(bb_spi_frame_t *frame, uint32_t addr) {
    frame->opcode = 0xEBu;
    frame->addr = addr;
    frame->lane_mode = (uint8_t)g_bus_mode;
    frame->data_len = 32u;
    frame->write_phase = false;
    frame->mutated = false;
    fill_words(frame, 0xB0070000u | addr);
    if (g_mutation_enabled && g_overlay_name[0] != '\0') {
        if (overlay_store_apply(g_overlay_name, frame)) {
            frame->mutated = true;
        }
    }
}

static void make_status_frame(bb_spi_frame_t *frame) {
    frame->opcode = 0x05u;
    frame->addr = 0u;
    frame->lane_mode = (uint8_t)g_bus_mode;
    frame->data_len = 1u;
    frame->write_phase = false;
    frame->mutated = false;
    frame->data[0] = (g_delay_us > 200u) ? 0x01u : 0x00u;
    for (uint32_t i = 1; i < BB_MAX_CAPTURE_WORDS; ++i) {
        frame->data[i] = 0u;
    }
}

static void make_manifest_frame(bb_spi_frame_t *frame, uint32_t addr) {
    frame->opcode = 0x6Bu;
    frame->addr = addr;
    frame->lane_mode = (uint8_t)g_bus_mode;
    frame->data_len = 24u;
    frame->write_phase = false;
    frame->mutated = false;
    fill_words(frame, 0x4D414E00u | (addr & 0xFFu));
    if (g_mutation_enabled && overlay_store_apply(g_overlay_name, frame)) {
        frame->mutated = true;
    }
}

static void make_sfdp_frame(bb_spi_frame_t *frame) {
    frame->opcode = 0x5Au;
    frame->addr = 0u;
    frame->lane_mode = (uint8_t)g_bus_mode;
    frame->data_len = 16u;
    frame->write_phase = false;
    frame->mutated = false;
    frame->data[0] = 0x50444653u;
    frame->data[1] = 0x00010601u;
    frame->data[2] = 0xFF00FF00u;
    frame->data[3] = 0x00112233u;
    for (uint32_t i = 4; i < BB_MAX_CAPTURE_WORDS; ++i) {
        frame->data[i] = 0xAA550000u | i;
    }
}

static void maybe_truncate(bb_spi_frame_t *frame) {
    if (!g_mutation_enabled || g_truncation_len == 0u) {
        return;
    }
    if (frame->data_len > g_truncation_len) {
        frame->data_len = (uint8_t)g_truncation_len;
        frame->mutated = true;
    }
}

bool spi_link_poll_frame(uint32_t now_ms, bb_spi_frame_t *frame) {
    if (frame == NULL) {
        return false;
    }
    if ((now_ms % 100u) != 0u) {
        return false;
    }

    memset(frame, 0, sizeof(*frame));
    g_sequence++;
    switch (g_sequence % 7u) {
        case 1u:
            make_jedec_frame(frame);
            break;
        case 2u:
            make_sfdp_frame(frame);
            break;
        case 3u:
            make_header_frame(frame, 0x00000000u);
            break;
        case 4u:
            make_status_frame(frame);
            break;
        case 5u:
            make_manifest_frame(frame, 0x00001000u);
            break;
        case 6u:
            make_header_frame(frame, 0x00002000u);
            break;
        default:
            make_status_frame(frame);
            break;
    }

    maybe_truncate(frame);
    if (g_delay_us > 0u && !frame->write_phase) {
        frame->data[BB_MAX_CAPTURE_WORDS - 1u] = g_delay_us;
    }
    return true;
}
