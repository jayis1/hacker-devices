/*
 * EtherCAT parser and capture pipeline
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ethercat.h"

#include <stdio.h>
#include <string.h>

#define DEMO_FRAME_COUNT 12u

typedef struct {
    eph_frame_view_t frames[DEMO_FRAME_COUNT];
    size_t cursor;
} eph_demo_stream_t;

static eph_demo_stream_t g_demo;

static const struct {
    uint16_t index;
    uint8_t subindex;
    const char *name;
} g_object_names[] = {
    {0x6040u, 0x00u, "Controlword"},
    {0x6041u, 0x00u, "Statusword"},
    {0x6060u, 0x00u, "Modes of operation"},
    {0x6061u, 0x00u, "Modes display"},
    {0x607Au, 0x00u, "Target position"},
    {0x60FFu, 0x00u, "Target velocity"},
    {0x6071u, 0x00u, "Target torque"},
    {0x7000u, 0x01u, "Conveyor speed setpoint"},
    {0x7000u, 0x02u, "Heater output setpoint"},
    {0x7010u, 0x01u, "Safety latch"},
    {0x7020u, 0x01u, "Valve position"}
};

void eph_ethercat_init(void) {
    memset(&g_demo, 0, sizeof(g_demo));
}

void eph_ethercat_seed_demo_frames(void) {
    const eph_frame_view_t seeds[DEMO_FRAME_COUNT] = {
        {0x02u, 0x7000u, 0x01u, 1200, 1u, false, true},
        {0x03u, 0x7000u, 0x02u, 75, 1u, false, true},
        {0x04u, 0x607Au, 0x00u, 50000, 1u, false, true},
        {0x04u, 0x60FFu, 0x00u, 800, 1u, false, true},
        {0x05u, 0x7020u, 0x01u, 34, 1u, false, true},
        {0x06u, 0x6040u, 0x00u, 0x000F, 1u, false, true},
        {0x07u, 0x7010u, 0x01u, 0, 1u, false, true},
        {0x08u, 0x6071u, 0x00u, 55, 1u, false, true},
        {0x04u, 0x6060u, 0x00u, 8, 1u, true, false},
        {0x04u, 0x6061u, 0x00u, 8, 1u, true, false},
        {0x02u, 0x7000u, 0x01u, 1600, 1u, false, true},
        {0x05u, 0x7020u, 0x01u, 82, 1u, false, true}
    };

    memcpy(g_demo.frames, seeds, sizeof(seeds));
    g_demo.cursor = 0u;
}

bool eph_ethercat_next_frame(eph_frame_view_t *frame) {
    if (g_demo.cursor >= DEMO_FRAME_COUNT) {
        return false;
    }
    *frame = g_demo.frames[g_demo.cursor++];
    return true;
}

size_t eph_ethercat_encode_frame(const eph_frame_view_t *frame, uint8_t *buffer, size_t capacity) {
    if (frame == NULL || buffer == NULL || capacity < 13u) {
        return 0u;
    }

    buffer[0] = frame->slave_address;
    buffer[1] = (uint8_t)(frame->object_index & 0xFFu);
    buffer[2] = (uint8_t)((frame->object_index >> 8) & 0xFFu);
    buffer[3] = frame->object_subindex;
    buffer[4] = (uint8_t)(frame->process_value & 0xFFu);
    buffer[5] = (uint8_t)((frame->process_value >> 8) & 0xFFu);
    buffer[6] = (uint8_t)((frame->process_value >> 16) & 0xFFu);
    buffer[7] = (uint8_t)((frame->process_value >> 24) & 0xFFu);
    buffer[8] = (uint8_t)(frame->working_counter & 0xFFu);
    buffer[9] = (uint8_t)((frame->working_counter >> 8) & 0xFFu);
    buffer[10] = frame->has_mailbox ? 1u : 0u;
    buffer[11] = frame->is_write ? 1u : 0u;
    buffer[12] = (uint8_t)(buffer[0] ^ buffer[1] ^ buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5] ^ buffer[6] ^ buffer[7] ^ buffer[8] ^ buffer[9] ^ buffer[10] ^ buffer[11]);
    return 13u;
}

bool eph_ethercat_decode_frame(const uint8_t *buffer, size_t length, eph_frame_view_t *frame) {
    uint8_t crc = 0u;
    size_t i;

    if (buffer == NULL || frame == NULL || length < 13u) {
        return false;
    }

    for (i = 0u; i < 12u; ++i) {
        crc ^= buffer[i];
    }
    if (crc != buffer[12]) {
        return false;
    }

    frame->slave_address = buffer[0];
    frame->object_index = (uint16_t)buffer[1] | ((uint16_t)buffer[2] << 8);
    frame->object_subindex = buffer[3];
    frame->process_value = (int32_t)((uint32_t)buffer[4] |
                           ((uint32_t)buffer[5] << 8) |
                           ((uint32_t)buffer[6] << 16) |
                           ((uint32_t)buffer[7] << 24));
    frame->working_counter = (uint16_t)buffer[8] | ((uint16_t)buffer[9] << 8);
    frame->has_mailbox = buffer[10] != 0u;
    frame->is_write = buffer[11] != 0u;
    return true;
}

void eph_ethercat_capture(eph_capture_ring_t *ring, const eph_frame_view_t *before, int32_t after_value, bool modified) {
    eph_capture_t *slot;

    if (ring == NULL || before == NULL) {
        return;
    }

    slot = &ring->ring[ring->head];
    ring->cycle_counter += 1u;
    slot->cycle_counter = ring->cycle_counter;
    slot->frame_length = 13u;
    slot->slave_address = before->slave_address;
    slot->index = before->object_index;
    slot->subindex = before->object_subindex;
    slot->value_before = before->process_value;
    slot->value_after = after_value;
    slot->modified = modified;
    slot->working_counter = before->working_counter;

    ring->head = (ring->head + 1u) % EPH_CAPTURE_DEPTH;
    if (ring->count < EPH_CAPTURE_DEPTH) {
        ring->count += 1u;
    }
}

size_t eph_ethercat_drain_captures(eph_capture_ring_t *ring, eph_capture_t *out, size_t max_items) {
    size_t emitted = 0u;
    size_t start;

    if (ring == NULL || out == NULL || max_items == 0u) {
        return 0u;
    }

    if (ring->count == 0u) {
        return 0u;
    }

    start = (ring->head + EPH_CAPTURE_DEPTH - ring->count) % EPH_CAPTURE_DEPTH;
    while (emitted < ring->count && emitted < max_items) {
        out[emitted] = ring->ring[(start + emitted) % EPH_CAPTURE_DEPTH];
        emitted++;
    }
    ring->count = 0u;
    return emitted;
}

const char *eph_ethercat_object_name(uint16_t index, uint8_t subindex) {
    size_t i;
    for (i = 0u; i < sizeof(g_object_names) / sizeof(g_object_names[0]); ++i) {
        if (g_object_names[i].index == index && g_object_names[i].subindex == subindex) {
            return g_object_names[i].name;
        }
    }
    return "Unknown object";
}
