/*
 * EtherCAT Phantom Control Plane Firmware
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef ETHERCAT_PHANTOM_BOARD_H
#define ETHERCAT_PHANTOM_BOARD_H

#include <stdint.h>
#include <stdbool.h>

#define EPH_AUTHOR "jayis1"
#define EPH_DEVICE_NAME "ethercat-phantom"
#define EPH_PROTOCOL_VERSION 1u
#define EPH_MAX_RULES 16u
#define EPH_MAX_FRAME_BYTES 128u
#define EPH_CAPTURE_DEPTH 32u
#define EPH_RADIO_MTU 244u
#define EPH_STORAGE_SECTORS 64u
#define EPH_POWER_LOW_MV 3450u
#define EPH_POWER_CRITICAL_MV 3300u
#define EPH_DEFAULT_SESSION_KEY 0x45504831u

typedef enum {
    EPH_ROLE_BYPASS = 0,
    EPH_ROLE_OBSERVE = 1,
    EPH_ROLE_MANIPULATE = 2,
    EPH_ROLE_ACTIVE_TEST = 3
} eph_role_t;

typedef enum {
    EPH_RULE_DISABLED = 0,
    EPH_RULE_MONITOR_ONLY = 1,
    EPH_RULE_CLAMP_RANGE = 2,
    EPH_RULE_OFFSET_VALUE = 3,
    EPH_RULE_FORCE_VALUE = 4,
    EPH_RULE_BIT_TOGGLE = 5
} eph_rule_action_t;

typedef struct {
    uint8_t index;
    bool enabled;
    uint16_t matcher_index;
    uint16_t matcher_subindex;
    uint8_t matcher_slave;
    eph_rule_action_t action;
    int32_t param_a;
    int32_t param_b;
    char label[24];
} eph_rule_t;

typedef struct {
    uint32_t cycle_counter;
    uint16_t frame_length;
    uint8_t slave_address;
    uint16_t index;
    uint8_t subindex;
    int32_t value_before;
    int32_t value_after;
    bool modified;
    uint16_t working_counter;
} eph_capture_t;

typedef struct {
    uint32_t frames_seen;
    uint32_t frames_modified;
    uint32_t rules_triggered;
    uint32_t working_counter_faults;
    uint32_t malformed_frames;
    uint32_t mailbox_commands;
    uint32_t radio_packets;
    uint32_t brownouts;
} eph_metrics_t;

typedef struct {
    eph_role_t role;
    uint32_t cycle_budget_ns;
    uint32_t session_key;
    bool safe_mode;
    bool logging_enabled;
    bool radio_enabled;
} eph_runtime_config_t;

#endif
