/*
 * board.h - eSPI Revenant shared definitions
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef ESPI_REVENANT_BOARD_H
#define ESPI_REVENANT_BOARD_H

#include <stddef.h>
#include <stdint.h>

#define ER_AUTHOR "jayis1"
#define ER_DEVICE_NAME "eSPI Revenant"
#define ER_MAX_EVENTS 192
#define ER_MAX_MESSAGE 120
#define ER_MAX_PROFILE_NAME 40
#define ER_MAX_NOTES 64
#define ER_MAX_TRACE_FRAMES 128
#define ER_MAX_COMMAND_BYTES 160
#define ER_MAX_CHANNEL_NAME 16
#define ER_TEMP_LIMIT_C 76.0f
#define ER_CURRENT_LIMIT_MA 420.0f
#define ER_FLASH_DELAY_MAX_NS 180u
#define ER_TICK_MS 5u

typedef enum {
    ER_MODE_PASSIVE = 0,
    ER_MODE_GUARDED = 1,
    ER_MODE_ACTIVE = 2,
    ER_MODE_BYPASS = 3
} er_mode_t;

typedef enum {
    ER_CH_VWIRE = 0,
    ER_CH_PERIPHERAL = 1,
    ER_CH_OOB = 2,
    ER_CH_FLASH = 3,
    ER_CH_GPIO = 4
} er_channel_t;

typedef enum {
    ER_EVENT_BOOT = 1,
    ER_EVENT_PROFILE_LOAD,
    ER_EVENT_TRACE,
    ER_EVENT_VWIRE_INJECT,
    ER_EVENT_FLASH_DELAY,
    ER_EVENT_PERIPH_REPLAY,
    ER_EVENT_ALERT_PULSE,
    ER_EVENT_SAFETY,
    ER_EVENT_ROLLBACK,
    ER_EVENT_COMMAND,
    ER_EVENT_ANOMALY
} er_event_code_t;

typedef enum {
    ER_RISK_LOW = 0,
    ER_RISK_MEDIUM = 1,
    ER_RISK_HIGH = 2
} er_risk_t;

typedef struct {
    uint32_t timestamp_ms;
    er_event_code_t code;
    er_channel_t channel;
    er_risk_t risk;
    char message[ER_MAX_MESSAGE];
} er_event_t;

typedef struct {
    char name[ER_MAX_PROFILE_NAME];
    uint8_t allow_vwire_inject;
    uint8_t allow_flash_delay;
    uint8_t allow_peripheral_replay;
    uint8_t trigger_on_resume;
    uint32_t flash_delay_ns;
    uint8_t max_vwire_pulses;
    uint8_t max_peripheral_ops;
    uint8_t passive_prearm;
} er_profile_t;

typedef struct {
    er_mode_t mode;
    uint8_t armed;
    uint8_t watchdog_ok;
    uint8_t bypass_enabled;
    uint8_t target_present;
    uint32_t trigger_count;
    uint32_t anomalies;
    float board_temp_c;
    float ec_current_ma;
    float target_voltage_v;
    char active_profile[ER_MAX_PROFILE_NAME];
} er_status_t;

typedef struct {
    uint32_t timestamp_ms;
    er_channel_t channel;
    uint8_t opcode;
    uint8_t tag;
    uint8_t length;
    uint8_t data[16];
} er_trace_frame_t;

#endif
