/*
 * SmartPack Phantom board definitions
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef SMARTPACK_BOARD_H
#define SMARTPACK_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SP_FW_VERSION_MAJOR 1u
#define SP_FW_VERSION_MINOR 0u
#define SP_FW_VERSION_PATCH 0u

#define SP_EVENT_LOG_CAPACITY 160u
#define SP_MAX_STRING 32u
#define SP_MAX_MANUF_BLOCK 40u
#define SP_MAX_ALERT_QUEUE 16u
#define SP_MAX_EXPORT_FRAMES 24u

typedef enum {
    SP_EVENT_BOOT = 0,
    SP_EVENT_STATUS,
    SP_EVENT_BUS,
    SP_EVENT_PROFILE,
    SP_EVENT_AUTH,
    SP_EVENT_SCENARIO_ARM,
    SP_EVENT_SCENARIO_TRIGGER,
    SP_EVENT_SCENARIO_COMPLETE,
    SP_EVENT_ALERT,
    SP_EVENT_TELEMETRY,
    SP_EVENT_EXPORT
} sp_event_type_t;

typedef enum {
    SP_PROFILE_ENTERPRISE_LAPTOP = 0,
    SP_PROFILE_RUGGED_TABLET,
    SP_PROFILE_DRONE_PACK,
    SP_PROFILE_SERVICE_PACK,
    SP_PROFILE_COUNTERFEIT_CLONE,
    SP_PROFILE_COUNT
} sp_profile_kind_t;

typedef enum {
    SP_SCENARIO_IDLE = 0,
    SP_SCENARIO_MAINTENANCE_MASK,
    SP_SCENARIO_LOW_SOC_BAIT,
    SP_SCENARIO_STALE_AUTH_REPLAY,
    SP_SCENARIO_SHIPPING_CONFUSION,
    SP_SCENARIO_THERMAL_TRIP_SPOOF,
    SP_SCENARIO_CHARGER_LIMIT_SWING
} sp_scenario_kind_t;

typedef enum {
    SP_ACTION_NONE = 0,
    SP_ACTION_ENABLE_MUTATION,
    SP_ACTION_DISABLE_MUTATION,
    SP_ACTION_SET_PROFILE,
    SP_ACTION_SET_SOC,
    SP_ACTION_SET_TEMP_C,
    SP_ACTION_SET_STATUS_WORD,
    SP_ACTION_SET_AUTH_MODE,
    SP_ACTION_ASSERT_ALERT,
    SP_ACTION_SET_CHARGE_LIMIT,
    SP_ACTION_COMPLETE
} sp_action_kind_t;

typedef enum {
    SP_AUTH_PASSTHROUGH = 0,
    SP_AUTH_SYNTHETIC,
    SP_AUTH_REPLAY_LAST,
    SP_AUTH_STALE_NONCE,
    SP_AUTH_FORCE_FAIL
} sp_auth_mode_t;

typedef struct {
    uint32_t timestamp_ms;
    sp_event_type_t type;
    uint32_t param0;
    uint32_t param1;
    char text[80];
} sp_event_t;

typedef struct {
    sp_action_kind_t kind;
    uint32_t deadline_ms;
    uint32_t value0;
    uint32_t value1;
    char text[SP_MAX_STRING];
} sp_script_action_t;

typedef struct {
    bool active;
    bool triggered;
    bool completed;
    uint32_t start_ms;
    uint32_t step_index;
    sp_scenario_kind_t kind;
    char name[SP_MAX_STRING];
} sp_scenario_state_t;

typedef struct {
    sp_profile_kind_t kind;
    char manufacturer[SP_MAX_STRING];
    char device_name[SP_MAX_STRING];
    char chemistry[SP_MAX_STRING];
    uint16_t design_voltage_mv;
    uint16_t full_charge_capacity_mah;
    uint16_t remaining_capacity_mah;
    int16_t current_ma;
    int16_t avg_current_ma;
    int16_t temperature_c;
    uint16_t cycle_count;
    uint16_t serial;
    uint16_t status_word;
    uint16_t charge_limit_ma;
    bool maintenance_flag;
    bool shipping_mode;
    bool permanent_failure;
} sp_battery_profile_t;

typedef struct {
    uint32_t host_queries;
    uint32_t pack_replies;
    uint32_t mutated_replies;
    uint32_t alert_assertions;
    uint32_t block_reads;
    uint32_t command_errors;
    uint16_t last_command;
    bool alert_line;
    bool transparent_bypass;
} sp_bus_stats_t;

typedef struct {
    uint32_t challenge_count;
    uint32_t replay_count;
    uint32_t failure_count;
    uint32_t synthetic_count;
    uint32_t last_challenge;
    uint32_t last_response;
    sp_auth_mode_t mode;
} sp_auth_state_t;

typedef struct {
    uint16_t host_voltage_mv;
    uint16_t pack_voltage_mv;
    int16_t pack_current_ma;
    int16_t board_temp_c;
    uint16_t target_current_ma;
    bool current_limit_tripped;
    bool emergency_cutoff;
} sp_telemetry_t;

typedef struct {
    uint32_t frames_sent;
    uint32_t exports_generated;
    uint32_t dropped_frames;
    char last_frame[96];
} sp_radio_state_t;

typedef struct {
    uint32_t flags;
    bool mutation_enabled;
    bool authorized_mode;
    bool passive_mode;
    bool pack_present;
    bool host_present;
    sp_battery_profile_t profile;
    sp_scenario_state_t scenario;
    sp_bus_stats_t bus;
    sp_auth_state_t auth;
    sp_telemetry_t telemetry;
    sp_radio_state_t radio;
} sp_runtime_t;

#define SP_FLAG_PASSIVE_PROXY      (1u << 0)
#define SP_FLAG_MUTATION_ARMED     (1u << 1)
#define SP_FLAG_TRIGGERED          (1u << 2)
#define SP_FLAG_PACK_AUTH_OK       (1u << 3)
#define SP_FLAG_ALERT_ACTIVE       (1u << 4)
#define SP_FLAG_SHIPPING_MODE      (1u << 5)
#define SP_FLAG_THERMAL_WARNING    (1u << 6)
#define SP_FLAG_CURRENT_LIMIT      (1u << 7)

#endif
