/*
 * MoCA Phantom Frame Engine Interface
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef MPH_MOCA_H
#define MPH_MOCA_H

#include "../board.h"

void mph_moca_init(mph_runtime_config_t *config, mph_metrics_t *metrics);
void mph_moca_seed_demo_frames(void);
bool mph_moca_next_frame(mph_frame_t *frame);
void mph_moca_discover_nodes(mph_node_t *nodes, size_t *count, uint32_t network_id, mph_metrics_t *metrics);
const char *mph_moca_class_name(mph_traffic_class_t traffic_class);
void mph_moca_capture(mph_capture_ring_t *ring, const mph_frame_t *frame, bool modified);
size_t mph_moca_drain_captures(mph_capture_ring_t *ring, mph_frame_t *out, size_t max_items);
size_t mph_moca_encode_frame(const mph_frame_t *frame, uint8_t *out, size_t out_len);
void mph_moca_print_topology(const mph_node_t *nodes, size_t count);

#endif
