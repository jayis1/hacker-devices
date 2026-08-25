/*
 * MoCA Phantom Shared Firmware Types
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef MPH_BOARD_H
#define MPH_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MPH_AUTHOR "jayis1"
#define MPH_DEVICE_NAME "MoCA Phantom"
#define MPH_VERSION "1.0.0"
#define MPH_MAX_NODES 16u
#define MPH_MAX_EVENTS 64u
#define MPH_MAX_RULES 12u
#define MPH_MAX_CAPTURE_BYTES 128u
#define MPH_CAPTURE_DEPTH 32u
#define MPH_PROFILE_NAME_LEN 24u
#define MPH_LABEL_LEN 32u
#define MPH_MAX_CHANNELS 8u
#define MPH_DEFAULT_PRIVACY_KEY 0x4D50484FUL

typedef enum {
    MPH_MODE_SURVEY = 0,
    MPH_MODE_INLINE = 1,
    MPH_MODE_INJECT = 2,
    MPH_MODE_GUARD = 3
} mph_mode_t;

typedef enum {
    MPH_TRAFFIC_BEACON = 0,
    MPH_TRAFFIC_MGMT = 1,
    MPH_TRAFFIC_DATA = 2,
    MPH_TRAFFIC_PROBE = 3
} mph_traffic_class_t;

typedef enum {
    MPH_RULE_NONE = 0,
    MPH_RULE_DELAY = 1,
    MPH_RULE_DROP = 2,
    MPH_RULE_REWRITE_NODE = 3,
    MPH_RULE_REWRITE_PRIVACY = 4,
    MPH_RULE_FORCE_RATECAP = 5,
    MPH_RULE_TAG_ALERT = 6
} mph_rule_action_t;

typedef struct {
    uint8_t node_id;
    uint16_t network_id;
    uint32_t phy_rate_mbps;
    int8_t rssi_dbm;
    bool privacy_enabled;
    bool controller;
    bool visible;
    char label[MPH_LABEL_LEN];
} mph_node_t;

typedef struct {
    uint32_t timestamp_ms;
    uint8_t src_node;
    uint8_t dst_node;
    mph_traffic_class_t traffic_class;
    uint16_t ether_type;
    uint8_t channel;
    uint32_t phy_rate_mbps;
    uint32_t privacy_key_id;
    uint16_t payload_len;
    uint8_t payload[MPH_MAX_CAPTURE_BYTES];
    bool encrypted;
    bool management;
    bool anomaly;
} mph_frame_t;

typedef struct {
    uint8_t index;
    bool enabled;
    uint8_t match_src;
    uint8_t match_dst;
    mph_traffic_class_t match_class;
    uint8_t match_channel;
    mph_rule_action_t action;
    uint32_t param_a;
    uint32_t param_b;
    char label[MPH_LABEL_LEN];
} mph_rule_t;

typedef struct {
    bool matched;
    bool modified;
    bool dropped;
    bool alerted;
    uint32_t latency_added_us;
    const mph_rule_t *rule;
    uint8_t rewritten_src;
    uint8_t rewritten_dst;
    uint32_t rewritten_key;
    uint32_t rewritten_rate;
} mph_rule_result_t;

typedef struct {
    mph_frame_t items[MPH_CAPTURE_DEPTH];
    size_t head;
    size_t count;
} mph_capture_ring_t;

typedef struct {
    mph_mode_t mode;
    bool safe_mode;
    bool radio_enabled;
    bool inline_bridge_enabled;
    bool spectrum_watch_enabled;
    uint32_t target_network_id;
    uint32_t privacy_key;
    uint16_t latency_budget_us;
    uint8_t operator_channel;
    char active_profile[MPH_PROFILE_NAME_LEN];
} mph_runtime_config_t;

typedef struct {
    uint32_t frames_seen;
    uint32_t frames_modified;
    uint32_t frames_dropped;
    uint32_t alerts_raised;
    uint32_t nodes_discovered;
    uint32_t privacy_violations;
    uint32_t rate_cap_events;
    uint32_t spectrum_peaks;
    uint32_t radio_packets;
    uint32_t storage_records;
    uint32_t battery_warnings;
    uint32_t bypass_events;
} mph_metrics_t;

typedef struct {
    uint16_t battery_mv;
    uint8_t soc_percent;
    bool usb_present;
    bool charging;
    bool critical;
    uint16_t rail_3v3_mv;
    uint16_t rail_1v2_mv;
} mph_power_state_t;

typedef struct {
    uint8_t strongest_channel;
    int16_t strongest_energy_db;
    uint8_t suspicious_hops;
    bool leakage_suspected;
} mph_spectrum_snapshot_t;

#endif
