/*
 * BootROM Banshee firmware board definitions
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef BOOTROM_BANSHEE_BOARD_H
#define BOOTROM_BANSHEE_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#define BB_FW_VERSION_MAJOR 1U
#define BB_FW_VERSION_MINOR 0U
#define BB_FW_VERSION_PATCH 0U

#define BB_EVENT_LOG_CAPACITY 256U
#define BB_STATUS_TEXT_LEN 96U
#define BB_PROFILE_NAME_LEN 32U
#define BB_SCENARIO_NAME_LEN 32U
#define BB_MAX_CAPTURE_WORDS 8U
#define BB_MAX_OVERLAYS 12U
#define BB_MAX_COMMANDS 16U

typedef enum {
    BB_EVENT_BOOT = 0,
    BB_EVENT_ATTACH,
    BB_EVENT_CAPTURE,
    BB_EVENT_OVERLAY,
    BB_EVENT_GLITCH,
    BB_EVENT_SCENARIO_ARMED,
    BB_EVENT_SCENARIO_TRIGGERED,
    BB_EVENT_SCENARIO_COMPLETE,
    BB_EVENT_SCENARIO_ABORT,
    BB_EVENT_REMOTE_COMMAND,
    BB_EVENT_FAULT,
    BB_EVENT_STATUS
} bb_event_type_t;

typedef enum {
    BB_BUS_MODE_SPI = 0,
    BB_BUS_MODE_DSPI,
    BB_BUS_MODE_QSPI
} bb_bus_mode_t;

typedef enum {
    BB_SCENARIO_IDLE = 0,
    BB_SCENARIO_ROLLBACK_SHADOW,
    BB_SCENARIO_JEDEC_MASQUERADE,
    BB_SCENARIO_LATE_READY_STALL,
    BB_SCENARIO_HEADER_GHOST,
    BB_SCENARIO_STRAP_SIREN,
    BB_SCENARIO_CS_WHISPER
} bb_scenario_kind_t;

typedef enum {
    BB_ACTION_NONE = 0,
    BB_ACTION_ENABLE_MUTATION,
    BB_ACTION_DISABLE_MUTATION,
    BB_ACTION_SELECT_OVERLAY,
    BB_ACTION_SET_DELAY_US,
    BB_ACTION_SET_STRAP_MASK,
    BB_ACTION_SET_TRUNCATION,
    BB_ACTION_FORCE_JEDEC,
    BB_ACTION_COMPLETE
} bb_action_kind_t;

typedef enum {
    BB_RADIO_CMD_NONE = 0,
    BB_RADIO_CMD_GET_STATUS,
    BB_RADIO_CMD_ARM_SCENARIO,
    BB_RADIO_CMD_TRIGGER,
    BB_RADIO_CMD_ABORT,
    BB_RADIO_CMD_LOAD_OVERLAY,
    BB_RADIO_CMD_SET_LIMITS
} bb_radio_command_kind_t;

typedef struct {
    uint32_t timestamp_ms;
    bb_event_type_t type;
    uint32_t param0;
    uint32_t param1;
    char text[BB_STATUS_TEXT_LEN];
} bb_event_t;

typedef struct {
    uint32_t addr;
    uint8_t opcode;
    uint8_t lane_mode;
    uint8_t data_len;
    bool write_phase;
    bool mutated;
    uint32_t data[BB_MAX_CAPTURE_WORDS];
} bb_spi_frame_t;

typedef struct {
    char name[BB_PROFILE_NAME_LEN];
    uint32_t base_addr;
    uint32_t length;
    uint8_t bytes[64];
    bool enabled;
} bb_overlay_t;

typedef struct {
    uint32_t target_vccio_mv;
    uint32_t flash_vccio_mv;
    uint32_t rail_current_ma;
    int32_t board_temp_c;
    bool watchdog_alert;
    bool trip;
} bb_telemetry_sample_t;

typedef struct {
    char name[BB_SCENARIO_NAME_LEN];
    bb_scenario_kind_t kind;
    uint32_t step_index;
    uint32_t start_ms;
    uint32_t mutation_budget_ms;
    bool armed;
    bool active;
    bool triggered;
    bool completed;
} bb_scenario_state_t;

typedef struct {
    bb_action_kind_t kind;
    uint32_t deadline_ms;
    uint32_t value0;
    uint32_t value1;
    char text[BB_STATUS_TEXT_LEN];
} bb_script_action_t;

typedef struct {
    uint32_t flags;
    bb_bus_mode_t bus_mode;
    bb_scenario_state_t scenario;
    bb_telemetry_sample_t telemetry;
    char active_overlay[BB_PROFILE_NAME_LEN];
    uint32_t glitch_delay_us;
    uint32_t truncation_len;
    uint32_t strap_mask;
    bool mutation_enabled;
} bb_runtime_t;

typedef struct {
    bb_radio_command_kind_t kind;
    uint32_t arg0;
    uint32_t arg1;
    char text[BB_STATUS_TEXT_LEN];
} bb_radio_command_t;

#define BB_FLAG_PASSIVE_PROXY      (1u << 0)
#define BB_FLAG_MUTATION_ARMED     (1u << 1)
#define BB_FLAG_TRIGGERED          (1u << 2)
#define BB_FLAG_TARGET_PRESENT     (1u << 3)
#define BB_FLAG_FLASH_PRESENT      (1u << 4)
#define BB_FLAG_SAFETY_TRIP        (1u << 5)
#define BB_FLAG_STRAP_ACTIVE       (1u << 6)
#define BB_FLAG_CAPTURE_ACTIVE     (1u << 7)

#define BB_LIMIT_MAX_CURRENT_MA 750U
#define BB_LIMIT_MAX_TEMP_C 85U
#define BB_LIMIT_MIN_VCCIO_MV 1100U
#define BB_LIMIT_MAX_VCCIO_MV 3600U

#endif
