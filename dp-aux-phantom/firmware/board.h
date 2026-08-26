/*
 * DP AUX Phantom firmware board definition
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DP_AUX_PHANTOM_BOARD_H
#define DP_AUX_PHANTOM_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DPA_DEVICE_NAME "DP AUX Phantom"
#define DPA_AUTHOR "jayis1"
#define DPA_VERSION_MAJOR 1U
#define DPA_VERSION_MINOR 0U
#define DPA_VERSION_PATCH 0U

#define DPA_TEXT_32 32U
#define DPA_TEXT_64 64U
#define DPA_TEXT_96 96U
#define DPA_TEXT_128 128U
#define DPA_TEXT_256 256U
#define DPA_TEXT_512 512U

#define DPA_MAX_EVENTS 256U
#define DPA_MAX_PROFILES 6U
#define DPA_MAX_SINKS 8U
#define DPA_MAX_TRACES 128U
#define DPA_MAX_RULES 12U
#define DPA_MAX_I2C_BYTES 16U
#define DPA_MAX_SIDEBAND_BYTES 48U

#define DPA_LINK_RATE_RBR 0x06U
#define DPA_LINK_RATE_HBR 0x0AU
#define DPA_LINK_RATE_HBR2 0x14U
#define DPA_LINK_RATE_HBR3 0x1EU
#define DPA_LANE_COUNT_1 1U
#define DPA_LANE_COUNT_2 2U
#define DPA_LANE_COUNT_4 4U

#define DPA_EVENT_INFO 0U
#define DPA_EVENT_RULE_HIT 1U
#define DPA_EVENT_ALERT 2U
#define DPA_EVENT_CAPTURE 3U

#define DPA_PROFILE_TRANSPARENT 0U
#define DPA_PROFILE_EDID_MASK 1U
#define DPA_PROFILE_LT_SLOWROLL 2U
#define DPA_PROFILE_DOCK_EMULATOR 3U
#define DPA_PROFILE_HPD_BOUNCE 4U
#define DPA_PROFILE_AUX_FUZZ 5U

#define DPA_AUX_NATIVE_READ 0x9U
#define DPA_AUX_NATIVE_WRITE 0x8U
#define DPA_AUX_I2C_READ 0x1U
#define DPA_AUX_I2C_WRITE 0x0U

#define DPA_DPCD_REV 0x00000U
#define DPA_DPCD_MAX_LINK_RATE 0x00001U
#define DPA_DPCD_MAX_LANE_COUNT 0x00002U
#define DPA_DPCD_TRAINING_PATTERN_SET 0x00102U
#define DPA_DPCD_LINK_BW_SET 0x00100U
#define DPA_DPCD_LANE_COUNT_SET 0x00101U
#define DPA_DPCD_LANE0_1_STATUS 0x00202U
#define DPA_DPCD_SINK_COUNT 0x00200U
#define DPA_DPCD_EDP_CONFIG_CAP 0x0000DU

#define DPA_AUX_ADDR_DPCD 0x00000U
#define DPA_AUX_ADDR_EDID 0x50U

typedef struct {
    uint32_t timestamp_ms;
    uint32_t address;
    uint8_t op;
    uint8_t length;
    uint8_t data[DPA_MAX_I2C_BYTES];
    bool injected;
    bool error;
    char note[DPA_TEXT_64];
} dpa_aux_transaction_t;

typedef struct {
    char sink_name[DPA_TEXT_64];
    char vendor[DPA_TEXT_32];
    char serial[DPA_TEXT_32];
    uint8_t dpcd_rev;
    uint8_t max_link_rate;
    uint8_t lane_count;
    bool mst_capable;
    bool hdcp_capable;
    bool hpd_high;
    bool edid_cached;
} dpa_sink_identity_t;

typedef struct {
    uint8_t profile_id;
    char name[DPA_TEXT_32];
    bool pass_through;
    bool mask_edid_serial;
    bool slow_link_training;
    bool emulate_dock;
    bool bounce_hpd;
    bool fuzz_aux;
    uint8_t forced_link_rate;
    uint8_t forced_lane_count;
    uint16_t hpd_pulse_ms;
    uint8_t fuzz_seed;
} dpa_policy_t;

typedef struct {
    uint32_t uptime_ms;
    uint32_t aux_reads;
    uint32_t aux_writes;
    uint32_t i2c_reads;
    uint32_t i2c_writes;
    uint32_t alerts;
    uint32_t rule_hits;
    uint32_t edid_overrides;
    uint32_t link_training_steps;
    uint8_t active_profile;
    uint8_t negotiated_link_rate;
    uint8_t negotiated_lane_count;
    bool sink_present;
    bool hpd_asserted;
    bool radio_connected;
    bool capture_armed;
} dpa_runtime_status_t;

typedef struct {
    char name[DPA_TEXT_32];
    char match[DPA_TEXT_64];
    char action[DPA_TEXT_64];
    uint8_t severity;
    bool enabled;
} dpa_rule_t;

#endif
