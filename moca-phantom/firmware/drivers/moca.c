/*
 * MoCA Phantom Frame Engine Simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>
#include <string.h>

#include "moca.h"

static mph_frame_t g_demo_frames[MPH_MAX_EVENTS];
static size_t g_demo_count;
static size_t g_demo_cursor;

static void mph_fill_payload(mph_frame_t *frame, uint8_t seed) {
    size_t i;
    for (i = 0u; i < frame->payload_len && i < MPH_MAX_CAPTURE_BYTES; ++i) {
        frame->payload[i] = (uint8_t)(seed + (uint8_t)i + (uint8_t)frame->src_node);
    }
}

void mph_moca_init(mph_runtime_config_t *config, mph_metrics_t *metrics) {
    g_demo_count = 0u;
    g_demo_cursor = 0u;
    if (config != NULL) {
        config->target_network_id = 0x241u;
    }
    if (metrics != NULL) {
        metrics->nodes_discovered = 0u;
    }
}

void mph_moca_seed_demo_frames(void) {
    size_t i;
    static const uint8_t srcs[12] = {1u, 2u, 4u, 3u, 5u, 2u, 1u, 4u, 6u, 3u, 5u, 1u};
    static const uint8_t dsts[12] = {2u, 1u, 1u, 5u, 3u, 6u, 4u, 1u, 2u, 1u, 4u, 7u};
    static const mph_traffic_class_t classes[12] = {
        MPH_TRAFFIC_BEACON, MPH_TRAFFIC_MGMT, MPH_TRAFFIC_DATA, MPH_TRAFFIC_DATA,
        MPH_TRAFFIC_PROBE, MPH_TRAFFIC_DATA, MPH_TRAFFIC_MGMT, MPH_TRAFFIC_DATA,
        MPH_TRAFFIC_DATA, MPH_TRAFFIC_BEACON, MPH_TRAFFIC_DATA, MPH_TRAFFIC_PROBE
    };
    static const uint16_t etypes[12] = {
        0x88E1u, 0x88E1u, 0x0800u, 0x86DDu,
        0x88E1u, 0x888Eu, 0x88E1u, 0x0800u,
        0x0800u, 0x88E1u, 0x88B5u, 0x88E1u
    };

    g_demo_count = 12u;
    g_demo_cursor = 0u;

    for (i = 0u; i < g_demo_count; ++i) {
        mph_frame_t *frame = &g_demo_frames[i];
        memset(frame, 0, sizeof(*frame));
        frame->timestamp_ms = (uint32_t)(100u * (uint32_t)(i + 1u));
        frame->src_node = srcs[i];
        frame->dst_node = dsts[i];
        frame->traffic_class = classes[i];
        frame->ether_type = etypes[i];
        frame->channel = (uint8_t)(i % 5u);
        frame->phy_rate_mbps = 900u + (uint32_t)(i * 85u);
        frame->privacy_key_id = (i == 6u) ? 0u : (0x40u + (uint32_t)(i % 3u));
        frame->payload_len = (uint16_t)(24u + (i * 6u));
        frame->encrypted = (i % 4u) != 0u;
        frame->management = frame->traffic_class != MPH_TRAFFIC_DATA;
        frame->anomaly = (i == 6u || i == 10u);
        mph_fill_payload(frame, (uint8_t)(0x20u + i));
    }
}

bool mph_moca_next_frame(mph_frame_t *frame) {
    if (frame == NULL || g_demo_cursor >= g_demo_count) {
        return false;
    }

    *frame = g_demo_frames[g_demo_cursor++];
    return true;
}

void mph_moca_discover_nodes(mph_node_t *nodes, size_t *count, uint32_t network_id, mph_metrics_t *metrics) {
    static const mph_node_t seed_nodes[] = {
        {1u, 0x241u, 2500u, -38, true, true, true, "gateway-cpe"},
        {2u, 0x241u, 1820u, -47, true, false, true, "livingroom-stb"},
        {3u, 0x241u, 1560u, -53, false, false, true, "bridge-extender"},
        {4u, 0x241u, 1710u, -49, true, false, true, "lab-laptop"},
        {5u, 0x241u, 980u, -61, false, false, true, "rogue-splitter"},
        {6u, 0x241u, 890u, -66, true, false, true, "ip-camera"},
        {7u, 0x242u, 1210u, -74, false, false, true, "adjacent-unit-leak"}
    };
    size_t i;
    size_t out_count = 0u;

    if (nodes == NULL || count == NULL) {
        return;
    }

    for (i = 0u; i < sizeof(seed_nodes) / sizeof(seed_nodes[0]); ++i) {
        if (seed_nodes[i].network_id == network_id || seed_nodes[i].network_id == (network_id + 1u)) {
            if (out_count < MPH_MAX_NODES) {
                nodes[out_count++] = seed_nodes[i];
            }
        }
    }

    *count = out_count;
    if (metrics != NULL) {
        metrics->nodes_discovered = (uint32_t)out_count;
        for (i = 0u; i < out_count; ++i) {
            if (!nodes[i].privacy_enabled) {
                metrics->privacy_violations += 1u;
            }
        }
    }
}

const char *mph_moca_class_name(mph_traffic_class_t traffic_class) {
    switch (traffic_class) {
        case MPH_TRAFFIC_BEACON:
            return "beacon";
        case MPH_TRAFFIC_MGMT:
            return "management";
        case MPH_TRAFFIC_DATA:
            return "data";
        case MPH_TRAFFIC_PROBE:
            return "probe";
        default:
            return "unknown";
    }
}

void mph_moca_capture(mph_capture_ring_t *ring, const mph_frame_t *frame, bool modified) {
    size_t index;
    if (ring == NULL || frame == NULL) {
        return;
    }

    index = (ring->head + ring->count) % MPH_CAPTURE_DEPTH;
    ring->items[index] = *frame;
    if (modified) {
        ring->items[index].anomaly = true;
    }

    if (ring->count < MPH_CAPTURE_DEPTH) {
        ring->count += 1u;
    } else {
        ring->head = (ring->head + 1u) % MPH_CAPTURE_DEPTH;
    }
}

size_t mph_moca_drain_captures(mph_capture_ring_t *ring, mph_frame_t *out, size_t max_items) {
    size_t drained = 0u;
    if (ring == NULL || out == NULL || max_items == 0u) {
        return 0u;
    }

    while (ring->count > 0u && drained < max_items) {
        out[drained++] = ring->items[ring->head];
        ring->head = (ring->head + 1u) % MPH_CAPTURE_DEPTH;
        ring->count -= 1u;
    }

    return drained;
}

size_t mph_moca_encode_frame(const mph_frame_t *frame, uint8_t *out, size_t out_len) {
    size_t needed;
    size_t i;
    if (frame == NULL || out == NULL) {
        return 0u;
    }

    needed = 12u + (size_t)frame->payload_len;
    if (out_len < needed) {
        return 0u;
    }

    out[0] = frame->src_node;
    out[1] = frame->dst_node;
    out[2] = (uint8_t)frame->traffic_class;
    out[3] = frame->channel;
    out[4] = (uint8_t)(frame->ether_type >> 8);
    out[5] = (uint8_t)(frame->ether_type & 0xFFu);
    out[6] = (uint8_t)(frame->phy_rate_mbps >> 8);
    out[7] = (uint8_t)(frame->phy_rate_mbps & 0xFFu);
    out[8] = (uint8_t)(frame->privacy_key_id >> 8);
    out[9] = (uint8_t)(frame->privacy_key_id & 0xFFu);
    out[10] = (uint8_t)(frame->payload_len >> 8);
    out[11] = (uint8_t)(frame->payload_len & 0xFFu);

    for (i = 0u; i < frame->payload_len; ++i) {
        out[12u + i] = frame->payload[i];
    }

    return needed;
}

void mph_moca_print_topology(const mph_node_t *nodes, size_t count) {
    size_t i;
    printf("[topology] discovered %zu nodes\n", count);
    for (i = 0u; i < count; ++i) {
        printf("  node=%u net=0x%03X rate=%4lu Mbps rssi=%d privacy=%s role=%s label=%s\n",
               nodes[i].node_id,
               nodes[i].network_id,
               (unsigned long)nodes[i].phy_rate_mbps,
               (int)nodes[i].rssi_dbm,
               nodes[i].privacy_enabled ? "on" : "off",
               nodes[i].controller ? "controller" : "endpoint",
               nodes[i].label);
    }
}
