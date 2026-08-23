/*
 * Dockruptor Board Definitions
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DOCKRUPTOR_BOARD_H
#define DOCKRUPTOR_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DR_MAX_CAPABILITIES 8
#define DR_MAX_EVENTS 256
#define DR_MAX_RULES 16
#define DR_MAX_COMMANDS 16
#define DR_MAX_TEXT 96
#define DR_MAX_TRACE_SUMMARY 32

typedef enum {
    DR_ROLE_SINK = 0,
    DR_ROLE_SOURCE = 1,
    DR_ROLE_DRP = 2
} dr_role_t;

typedef enum {
    DR_MODE_USB = 0,
    DR_MODE_DP = 1,
    DR_MODE_USB4 = 2,
    DR_MODE_VENDOR = 3
} dr_mode_t;

typedef enum {
    DR_DIR_HOST_TO_DOCK = 0,
    DR_DIR_DOCK_TO_HOST = 1,
    DR_DIR_INTERNAL = 2
} dr_direction_t;

typedef enum {
    DR_RULE_NONE = 0,
    DR_RULE_CLAMP_POWER = 1,
    DR_RULE_REORDER_PDO = 2,
    DR_RULE_DELAY_ALTMODE = 3,
    DR_RULE_FORCE_CHARGE_ONLY = 4,
    DR_RULE_SPOOF_CABLE = 5,
    DR_RULE_TRIGGER_SOFT_RESET = 6
} dr_rule_type_t;

typedef enum {
    DR_EVENT_INFO = 0,
    DR_EVENT_PD_MESSAGE = 1,
    DR_EVENT_POLICY = 2,
    DR_EVENT_POWER = 3,
    DR_EVENT_SAFETY = 4,
    DR_EVENT_COMMAND = 5
} dr_event_type_t;

typedef struct {
    uint16_t millivolts;
    uint16_t milliamps;
    bool pps;
    char label[20];
} dr_capability_t;

typedef struct {
    dr_role_t power_role;
    dr_role_t data_role;
    dr_mode_t mode;
    bool attached;
    bool vconn_enabled;
    uint16_t negotiated_mv;
    uint16_t negotiated_ma;
    uint8_t capability_count;
    dr_capability_t capabilities[DR_MAX_CAPABILITIES];
    char identity[48];
    char cable_identity[48];
} dr_port_state_t;

typedef struct {
    dr_rule_type_t type;
    bool enabled;
    uint16_t limit_mv;
    uint16_t limit_ma;
    uint8_t target_index;
    uint16_t holdoff_ticks;
    char name[32];
} dr_rule_t;

typedef struct {
    uint32_t tick;
    dr_event_type_t type;
    dr_direction_t direction;
    char summary[DR_MAX_TEXT];
} dr_event_t;

typedef struct {
    uint8_t battery_percent;
    float battery_voltage;
    float board_temp_c;
    bool relay_bypass;
    bool wireless_enabled;
    bool safe_mode;
} dr_power_state_t;

typedef struct {
    dr_port_state_t host;
    dr_port_state_t dock;
    dr_rule_t rules[DR_MAX_RULES];
    size_t rule_count;
    dr_event_t events[DR_MAX_EVENTS];
    size_t event_count;
    dr_power_state_t power;
    uint32_t tick;
    bool observe_only;
} dr_context_t;

#endif
