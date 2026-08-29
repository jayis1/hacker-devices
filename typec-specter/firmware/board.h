/*
 * Type-C Specter firmware board definitions
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef TYPEC_SPECTER_BOARD_H
#define TYPEC_SPECTER_BOARD_H

#include <stdint.h>
#include <stdbool.h>

#define TS_FW_VERSION_MAJOR 1U
#define TS_FW_VERSION_MINOR 0U
#define TS_FW_VERSION_PATCH 0U

#define TS_EVENT_LOG_CAPACITY 256U
#define TS_SCENARIO_NAME_LEN 32U
#define TS_PROFILE_NAME_LEN 32U
#define TS_STATUS_TEXT_LEN 96U

typedef enum {
    TS_EVENT_BOOT = 0,
    TS_EVENT_ATTACH,
    TS_EVENT_DETACH,
    TS_EVENT_PD_MESSAGE,
    TS_EVENT_SCENARIO_ARMED,
    TS_EVENT_SCENARIO_ACTION,
    TS_EVENT_SCENARIO_ABORT,
    TS_EVENT_POWER_SAMPLE,
    TS_EVENT_FAULT,
    TS_EVENT_REMOTE_COMMAND,
    TS_EVENT_PROFILE_LOAD
} ts_event_type_t;

typedef enum {
    TS_ROLE_NONE = 0,
    TS_ROLE_SOURCE,
    TS_ROLE_SINK,
    TS_ROLE_DRP,
    TS_ROLE_DEBUG_ACCESSORY
} ts_power_role_t;

typedef enum {
    TS_DATA_ROLE_NONE = 0,
    TS_DATA_ROLE_HOST,
    TS_DATA_ROLE_DEVICE,
    TS_DATA_ROLE_DEBUG
} ts_data_role_t;

typedef enum {
    TS_SCENARIO_IDLE = 0,
    TS_SCENARIO_DOCK_IDENTITY_FLIP,
    TS_SCENARIO_LATE_VCONN_CLAIM,
    TS_SCENARIO_ROLE_SWAP_RACE,
    TS_SCENARIO_DEBUG_ACCESSORY_PROBE,
    TS_SCENARIO_POWER_STARVE_THEN_RECOVER
} ts_scenario_kind_t;

typedef struct {
    uint32_t timestamp_ms;
    ts_event_type_t type;
    uint32_t param0;
    uint32_t param1;
    char text[TS_STATUS_TEXT_LEN];
} ts_event_t;

typedef struct {
    uint32_t seq;
    uint16_t header;
    uint32_t payload[7];
    uint8_t payload_len;
    bool mutated;
} ts_pd_packet_t;

typedef struct {
    char name[TS_SCENARIO_NAME_LEN];
    ts_scenario_kind_t kind;
    uint32_t step_index;
    uint32_t elapsed_ms;
    uint32_t mutation_budget_ms;
    bool armed;
    bool active;
    bool completed;
} ts_scenario_state_t;

typedef struct {
    uint32_t target_vbus_mv;
    uint32_t peer_vbus_mv;
    uint32_t target_ibus_ma;
    uint32_t peer_ibus_ma;
    int32_t board_temp_c;
    bool trip;
} ts_power_sample_t;

typedef struct {
    uint32_t flags;
    ts_power_role_t target_role;
    ts_power_role_t peer_role;
    ts_data_role_t data_role;
    ts_scenario_state_t scenario;
    ts_power_sample_t power;
    char active_profile[TS_PROFILE_NAME_LEN];
} ts_runtime_t;

#define TS_FLAG_PASSIVE_MONITOR   (1u << 0)
#define TS_FLAG_MUTATION_ARMED    (1u << 1)
#define TS_FLAG_USB2_ISOLATED     (1u << 2)
#define TS_FLAG_SBU_ISOLATED      (1u << 3)
#define TS_FLAG_PEER_PRESENT      (1u << 4)
#define TS_FLAG_TARGET_PRESENT    (1u << 5)
#define TS_FLAG_FAULTED           (1u << 6)
#define TS_FLAG_SAFE_MODE         (1u << 7)

#define TS_LIMIT_VBUS_MV_MAX 20000U
#define TS_LIMIT_VBUS_MV_MIN 4500U
#define TS_LIMIT_IBUS_MA_MAX 5000U
#define TS_LIMIT_TEMP_C_MAX 90U

#endif
