/*
 * board.h - MDIO Wraith shared definitions
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef MDIO_WRAITH_BOARD_H
#define MDIO_WRAITH_BOARD_H

#include <stddef.h>
#include <stdint.h>

#define MW_AUTHOR "jayis1"
#define MW_DEVICE_NAME "MDIO Wraith"
#define MW_FIRMWARE_VERSION "0.1.0"
#define MW_MAX_EVENTS 256
#define MW_MAX_MESSAGE 128
#define MW_MAX_PROFILE_NAME 48
#define MW_MAX_PHY_NAME 32
#define MW_MAX_COMMAND_BYTES 96
#define MW_MAX_SNAPSHOT_REGS 16
#define MW_CAPTURE_FRAMES 160
#define MW_CLAUSE45_DEVS 8
#define MW_TICK_MS 10u
#define MW_TEMP_LIMIT_C 72.0f
#define MW_CURRENT_LIMIT_MA 440.0f
#define MW_VOLTAGE_MIN_V 2.95f
#define MW_VOLTAGE_MAX_V 3.45f

typedef enum {
    MW_MODE_PASSIVE = 0,
    MW_MODE_GUARDED = 1,
    MW_MODE_ACTIVE = 2,
    MW_MODE_BYPASS = 3
} mw_mode_t;

typedef enum {
    MW_CHANNEL_MDIO = 0,
    MW_CHANNEL_STRAP = 1,
    MW_CHANNEL_TRIGGER = 2,
    MW_CHANNEL_POWER = 3,
    MW_CHANNEL_CONTROL = 4,
    MW_CHANNEL_RF = 5
} mw_channel_t;

typedef enum {
    MW_RISK_LOW = 0,
    MW_RISK_MEDIUM = 1,
    MW_RISK_HIGH = 2
} mw_risk_t;

typedef enum {
    MW_EVENT_BOOT = 1,
    MW_EVENT_PROFILE_LOAD,
    MW_EVENT_SCAN,
    MW_EVENT_READ,
    MW_EVENT_WRITE,
    MW_EVENT_STRAP_SWAP,
    MW_EVENT_GUARD_BLOCK,
    MW_EVENT_TRIGGER_HIT,
    MW_EVENT_CAPTURE,
    MW_EVENT_SAFETY,
    MW_EVENT_ROLLBACK,
    MW_EVENT_TELEMETRY,
    MW_EVENT_COMMAND,
    MW_EVENT_ANOMALY
} mw_event_code_t;

typedef enum {
    MW_OPERATION_READ = 0,
    MW_OPERATION_WRITE = 1,
    MW_OPERATION_SCAN = 2,
    MW_OPERATION_TRIGGER = 3
} mw_operation_t;

typedef struct {
    uint32_t timestamp_ms;
    mw_event_code_t code;
    mw_channel_t channel;
    mw_risk_t risk;
    char message[MW_MAX_MESSAGE];
} mw_event_t;

typedef struct {
    char name[MW_MAX_PROFILE_NAME];
    uint8_t allow_clause45;
    uint8_t allow_strap_swap;
    uint8_t allow_isolate_write;
    uint8_t allow_loopback_write;
    uint8_t require_arming;
    uint8_t write_budget;
    uint16_t settle_time_ms;
    uint16_t trigger_phy;
    uint16_t trigger_reg;
    uint16_t trigger_value_mask;
    uint16_t trigger_value_expected;
} mw_profile_t;

typedef struct {
    char phy_label[MW_MAX_PHY_NAME];
    uint8_t phy_addr;
    uint8_t oui_hi;
    uint16_t oui_mid;
    uint8_t has_clause45;
    uint8_t link_up;
    uint16_t bmsr;
    uint16_t bmcr;
    uint16_t phyidr1;
    uint16_t phyidr2;
    uint16_t vendor_regs[MW_MAX_SNAPSHOT_REGS];
} mw_phy_t;

typedef struct {
    uint32_t timestamp_ms;
    uint8_t phy_addr;
    uint8_t devad;
    uint16_t reg;
    uint16_t value;
    uint8_t is_write;
    uint8_t clause45;
    char note[48];
} mw_capture_frame_t;

typedef struct {
    mw_mode_t mode;
    uint8_t armed;
    uint8_t watchdog_ok;
    uint8_t target_present;
    uint8_t fail_safe_engaged;
    uint8_t trigger_seen;
    uint32_t writes_applied;
    uint32_t anomalies;
    uint16_t discovered_phys;
    float board_temp_c;
    float target_current_ma;
    float target_voltage_v;
    char active_profile[MW_MAX_PROFILE_NAME];
} mw_status_t;

typedef struct {
    uint32_t tick_ms;
    mw_status_t status;
    mw_event_t events[MW_MAX_EVENTS];
    size_t event_count;
    mw_capture_frame_t captures[MW_CAPTURE_FRAMES];
    size_t capture_count;
    mw_phy_t phys[8];
    size_t phy_count;
} mw_system_t;

#endif
