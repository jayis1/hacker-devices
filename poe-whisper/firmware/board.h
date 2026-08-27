/*
 * board.h - PoE Whisper board and shared definitions
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef POE_WHISPER_BOARD_H
#define POE_WHISPER_BOARD_H

#include <stdint.h>
#include <stddef.h>

#define PW_AUTHOR "jayis1"
#define PW_DEVICE_NAME "PoE Whisper"
#define PW_MAX_EVENTS 128
#define PW_MAX_PROFILE_NAME 32
#define PW_MAX_LLDP_PAYLOAD 128
#define PW_MAX_CURRENT_SAMPLES 64
#define PW_MAX_RADIO_FRAME 196
#define PW_SAFE_TEMP_C 78.0f
#define PW_SAFE_CURRENT_MA 1350.0f
#define PW_SAFE_BROWNOUT_MS 180u
#define PW_DEFAULT_BUDGET_W 30.0f
#define PW_BT_MAX_BUDGET_W 71.0f

typedef enum {
    PW_MODE_PASSIVE = 0,
    PW_MODE_PROFILED = 1,
    PW_MODE_ACTIVE = 2,
    PW_MODE_SAFE_ROLLBACK = 3
} pw_mode_t;

typedef enum {
    PW_CLASS_0 = 0,
    PW_CLASS_1 = 1,
    PW_CLASS_2 = 2,
    PW_CLASS_3 = 3,
    PW_CLASS_4 = 4,
    PW_CLASS_5 = 5,
    PW_CLASS_6 = 6,
    PW_CLASS_7 = 7,
    PW_CLASS_8 = 8
} pw_poe_class_t;

typedef enum {
    EVENT_BOOT = 1,
    EVENT_PSE_FINGERPRINT,
    EVENT_PD_CLASS_PRESENTED,
    EVENT_LLDP_CAPTURED,
    EVENT_LLDP_SPOOFED,
    EVENT_BROWNOUT_EXECUTED,
    EVENT_MPS_JITTER,
    EVENT_CURRENT_PATTERN,
    EVENT_TEMP_ALERT,
    EVENT_ROLLBACK,
    EVENT_OPERATOR_COMMAND
} pw_event_code_t;

typedef struct {
    uint32_t timestamp_ms;
    pw_event_code_t code;
    char message[112];
} pw_event_t;

typedef struct {
    char name[PW_MAX_PROFILE_NAME];
    pw_poe_class_t advertised_class;
    float requested_power_w;
    uint32_t brownout_ms;
    float brownout_target_v;
    uint8_t lldp_spoof;
    uint8_t mps_jitter;
    uint8_t passive_only;
} pw_profile_t;

typedef struct {
    float line_voltage_v;
    float line_current_ma;
    float board_temp_c;
    float allocated_power_w;
    pw_poe_class_t detected_class;
    pw_mode_t mode;
    uint8_t link_up;
    uint8_t bypass_enabled;
    uint8_t thermal_shutdown;
} pw_status_t;

#endif
