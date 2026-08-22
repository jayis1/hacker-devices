/*
 * EtherCAT parser and capture pipeline
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef ETHERCAT_PHANTOM_ETHERCAT_H
#define ETHERCAT_PHANTOM_ETHERCAT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

typedef struct {
    uint8_t slave_address;
    uint16_t object_index;
    uint8_t object_subindex;
    int32_t process_value;
    uint16_t working_counter;
    bool has_mailbox;
    bool is_write;
} eph_frame_view_t;

typedef struct {
    eph_capture_t ring[EPH_CAPTURE_DEPTH];
    size_t head;
    size_t count;
    uint32_t cycle_counter;
} eph_capture_ring_t;

void eph_ethercat_init(void);
void eph_ethercat_seed_demo_frames(void);
bool eph_ethercat_next_frame(eph_frame_view_t *frame);
size_t eph_ethercat_encode_frame(const eph_frame_view_t *frame, uint8_t *buffer, size_t capacity);
bool eph_ethercat_decode_frame(const uint8_t *buffer, size_t length, eph_frame_view_t *frame);
void eph_ethercat_capture(eph_capture_ring_t *ring, const eph_frame_view_t *before, int32_t after_value, bool modified);
size_t eph_ethercat_drain_captures(eph_capture_ring_t *ring, eph_capture_t *out, size_t max_items);
const char *eph_ethercat_object_name(uint16_t index, uint8_t subindex);

#endif
