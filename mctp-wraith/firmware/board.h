/*
 * MCTP Wraith board.h
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef MCTP_WRAITH_BOARD_H
#define MCTP_WRAITH_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MW_FW_VERSION_MAJOR 1U
#define MW_FW_VERSION_MINOR 0U
#define MW_FW_VERSION_PATCH 0U
#define MW_FW_VERSION_STRING "1.0.0"

#define MW_MAX_ENDPOINTS 16U
#define MW_MAX_ROUTES 24U
#define MW_MAX_POLICIES 16U
#define MW_MAX_EVENTS 128U
#define MW_MAX_TEXT 96U
#define MW_MAX_MEASUREMENTS 12U
#define MW_MAX_MESSAGE_BYTES 96U

#define MW_FLAG_PASSIVE          (1U << 0)
#define MW_FLAG_MUTATION_ARMED   (1U << 1)
#define MW_FLAG_BRIDGING_ENABLED (1U << 2)
#define MW_FLAG_ALERT_ASSERTED   (1U << 3)
#define MW_FLAG_EVIDENCE_READY   (1U << 4)
#define MW_FLAG_I3C_MODE         (1U << 5)
#define MW_FLAG_SAFE_MODE        (1U << 6)

typedef enum {
    MW_BUS_SMBUS = 0,
    MW_BUS_I3C = 1,
    MW_BUS_PCIE_VDM = 2
} mw_bus_kind_t;

typedef enum {
    MW_SCENARIO_IDLE = 0,
    MW_SCENARIO_ROUTE_POISON,
    MW_SCENARIO_SPDM_DOWNGRADE_PROBE,
    MW_SCENARIO_PLDM_STAGE_FUZZ,
    MW_SCENARIO_SENSOR_GHOST,
    MW_SCENARIO_ENDPOINT_CLONE
} mw_scenario_kind_t;

typedef enum {
    MW_EVENT_BOOT = 0,
    MW_EVENT_POLICY,
    MW_EVENT_ROUTE,
    MW_EVENT_MUTATION,
    MW_EVENT_TELEMETRY,
    MW_EVENT_CAPTURE,
    MW_EVENT_SCENARIO_START,
    MW_EVENT_SCENARIO_COMPLETE,
    MW_EVENT_ALERT
} mw_event_type_t;

typedef enum {
    MW_POLICY_PASS = 0,
    MW_POLICY_MIRROR,
    MW_POLICY_DELAY,
    MW_POLICY_REWRITE,
    MW_POLICY_DROP,
    MW_POLICY_ALERT
} mw_policy_action_t;

typedef struct {
    uint8_t eid;
    char name[24];
    char medium[24];
    bool present;
    bool bridge_visible;
    bool mutable_target;
    uint8_t bus_owner;
    uint8_t supports_spdm;
    uint8_t supports_pldm;
} mw_endpoint_t;

typedef struct {
    uint8_t src_eid;
    uint8_t dst_eid;
    uint8_t next_hop;
    uint8_t medium_tag;
    uint8_t ttl;
    bool poisoned;
} mw_route_t;

typedef struct {
    uint32_t timestamp_ms;
    mw_event_type_t type;
    uint32_t p0;
    uint32_t p1;
    char text[MW_MAX_TEXT];
} mw_event_t;

typedef struct {
    uint8_t eid_src;
    uint8_t eid_dst;
    uint8_t tag;
    uint8_t command_code;
    uint8_t integrity;
    uint8_t payload_len;
    uint8_t payload[MW_MAX_MESSAGE_BYTES];
} mw_message_t;

typedef struct {
    uint32_t rail_mv;
    uint32_t rail_ma;
    int32_t board_temp_c;
    uint32_t capture_count;
    uint32_t mutation_count;
    uint32_t dropped_count;
    uint32_t alert_count;
    uint32_t signed_bundle_count;
} mw_telemetry_t;

typedef struct {
    mw_policy_action_t action;
    uint8_t match_src;
    uint8_t match_dst;
    uint8_t match_cmd;
    uint32_t delay_ms;
    uint8_t rewrite_mask;
    uint8_t rewrite_value;
    bool enabled;
    char name[32];
} mw_policy_rule_t;

typedef struct {
    mw_scenario_kind_t kind;
    char name[32];
    bool active;
    bool triggered;
    bool completed;
    uint32_t start_ms;
    uint32_t step_index;
} mw_scenario_state_t;

typedef struct {
    uint32_t flags;
    mw_bus_kind_t bus_kind;
    mw_endpoint_t endpoints[MW_MAX_ENDPOINTS];
    size_t endpoint_count;
    mw_route_t routes[MW_MAX_ROUTES];
    size_t route_count;
    mw_policy_rule_t policies[MW_MAX_POLICIES];
    size_t policy_count;
    mw_telemetry_t telemetry;
    mw_scenario_state_t scenario;
} mw_runtime_t;

extern mw_runtime_t g_runtime;
extern mw_event_t g_events[MW_MAX_EVENTS];
extern uint32_t g_event_head;
extern uint32_t g_now_ms;

const char *mw_bus_name(mw_bus_kind_t kind);
const char *mw_scenario_name(mw_scenario_kind_t kind);
void mw_log_event(mw_event_type_t type, uint32_t p0, uint32_t p1, const char *text);

#endif
