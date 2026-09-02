/*
 * board.h - I3C Poltergeist shared definitions
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef I3C_POLTERGEIST_BOARD_H
#define I3C_POLTERGEIST_BOARD_H

#include <stddef.h>
#include <stdint.h>

#define IP_AUTHOR "jayis1"
#define IP_DEVICE_NAME "I3C Poltergeist"
#define IP_FIRMWARE_VERSION "0.1.0"

#define IP_MAX_EVENTS 256
#define IP_MAX_MESSAGE 128
#define IP_MAX_PROFILE_NAME 48
#define IP_MAX_TARGET_NAME 32
#define IP_MAX_CCC_NAME 24
#define IP_MAX_CAPTURE_NOTE 48
#define IP_MAX_TARGETS 8
#define IP_MAX_CAPTURES 192
#define IP_MAX_EXPORT 1536

#define IP_TICK_MS 5u
#define IP_TEMP_LIMIT_C 74.0f
#define IP_CURRENT_LIMIT_MA 420.0f
#define IP_VOLTAGE_MIN_V 1.05f
#define IP_VOLTAGE_MAX_V 3.55f
#define IP_MAX_WRITE_BUDGET 8u

typedef enum {
    IP_MODE_OBSERVE = 0,
    IP_MODE_GUARDED = 1,
    IP_MODE_ACTIVE = 2,
    IP_MODE_BYPASS = 3
} ip_mode_t;

typedef enum {
    IP_LINK_USB = 0,
    IP_LINK_BLE = 1,
    IP_LINK_WIFI = 2
} ip_link_t;

typedef enum {
    IP_RISK_LOW = 0,
    IP_RISK_MEDIUM = 1,
    IP_RISK_HIGH = 2
} ip_risk_t;

typedef enum {
    IP_EVENT_BOOT = 1,
    IP_EVENT_PROFILE_LOAD,
    IP_EVENT_DISCOVERY,
    IP_EVENT_CCC,
    IP_EVENT_READ,
    IP_EVENT_WRITE,
    IP_EVENT_DOWNGRADE,
    IP_EVENT_HOTJOIN,
    IP_EVENT_IBI,
    IP_EVENT_FABRIC,
    IP_EVENT_POLICY,
    IP_EVENT_CAPTURE,
    IP_EVENT_EXPORT,
    IP_EVENT_ROLLBACK,
    IP_EVENT_COMMAND,
    IP_EVENT_ANOMALY
} ip_event_code_t;

typedef enum {
    IP_TARGET_SENSOR = 0,
    IP_TARGET_EC = 1,
    IP_TARGET_PMIC = 2,
    IP_TARGET_SECURE = 3,
    IP_TARGET_TOUCH = 4,
    IP_TARGET_HAPTIC = 5
} ip_target_class_t;

typedef struct {
    uint32_t timestamp_ms;
    ip_event_code_t code;
    ip_risk_t risk;
    char message[IP_MAX_MESSAGE];
} ip_event_t;

typedef struct {
    char name[IP_MAX_PROFILE_NAME];
    uint8_t allow_downgrade_probe;
    uint8_t allow_hotjoin_inject;
    uint8_t allow_ibi_replay;
    uint8_t allow_ccc_suppression;
    uint8_t require_arming;
    uint8_t write_budget;
    uint8_t target_selector;
    uint8_t jitter_ns;
    uint16_t settle_time_ms;
    uint16_t trigger_ccc;
} ip_profile_t;

typedef struct {
    char name[IP_MAX_TARGET_NAME];
    ip_target_class_t target_class;
    uint8_t static_address;
    uint8_t dynamic_address;
    uint8_t pid_hi;
    uint16_t pid_lo;
    uint8_t supports_i3c;
    uint8_t supports_legacy_i2c;
    uint8_t accepts_ibi;
    uint8_t allows_hotjoin;
    uint8_t secure_role;
    uint8_t awake;
    uint16_t last_reg;
    uint16_t last_value;
} ip_target_t;

typedef struct {
    uint32_t timestamp_ms;
    uint8_t target_index;
    uint8_t address;
    uint16_t ccc;
    uint16_t reg;
    uint16_t value;
    uint8_t is_write;
    uint8_t is_i2c_compat;
    char note[IP_MAX_CAPTURE_NOTE];
} ip_capture_t;

typedef struct {
    ip_mode_t mode;
    uint8_t armed;
    uint8_t bypass_engaged;
    uint8_t fabric_locked;
    uint8_t hotjoin_seen;
    uint8_t ibi_seen;
    uint8_t target_present;
    uint8_t link_state[3];
    uint8_t remaining_write_budget;
    uint32_t captures_taken;
    uint32_t anomalies;
    float board_temp_c;
    float target_current_ma;
    float target_voltage_v;
    char active_profile[IP_MAX_PROFILE_NAME];
} ip_status_t;

typedef struct {
    uint32_t tick_ms;
    ip_status_t status;
    ip_event_t events[IP_MAX_EVENTS];
    size_t event_count;
    ip_capture_t captures[IP_MAX_CAPTURES];
    size_t capture_count;
    ip_target_t targets[IP_MAX_TARGETS];
    size_t target_count;
} ip_system_t;

#endif
