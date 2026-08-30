/*
 * board.h - EDID Phantom shared definitions
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef EDID_PHANTOM_BOARD_H
#define EDID_PHANTOM_BOARD_H

#include <stddef.h>
#include <stdint.h>

#define EP_AUTHOR "jayis1"
#define EP_DEVICE_NAME "EDID Phantom"
#define EP_VERSION "0.1"
#define EP_TICK_MS 10u
#define EP_MAX_EVENTS 256u
#define EP_MAX_EVENT_MESSAGE 128u
#define EP_MAX_PROFILE_NAME 40u
#define EP_MAX_COMMAND_LENGTH 96u
#define EP_MAX_EDID_BYTES 256u
#define EP_MAX_CEC_QUEUE 32u
#define EP_MAX_DDC_TRANSACTIONS 64u
#define EP_MAX_RADIO_CLIENTS 8u
#define EP_MAX_SCENARIO_NAME 32u
#define EP_SAFE_TEMP_C 74.0f
#define EP_SAFE_CURRENT_MA 680.0f
#define EP_HPD_PULSE_LIMIT 12u

typedef enum {
    EP_MODE_MONITOR = 0,
    EP_MODE_INLINE = 1,
    EP_MODE_GHOST = 2,
    EP_MODE_HARDENED = 3
} ep_mode_t;

typedef enum {
    EP_EVENT_BOOT = 1,
    EP_EVENT_PROFILE_LOADED,
    EP_EVENT_DDC_CAPTURE,
    EP_EVENT_EDID_MUTATION,
    EP_EVENT_HPD_PULSE,
    EP_EVENT_CEC_FRAME,
    EP_EVENT_RADIO_COMMAND,
    EP_EVENT_SAFETY,
    EP_EVENT_ALERT,
    EP_EVENT_ANALYTICS,
    EP_EVENT_STREAM_STATE,
    EP_EVENT_POLICY_BLOCK
} ep_event_code_t;

typedef enum {
    EP_RISK_INFO = 0,
    EP_RISK_LOW = 1,
    EP_RISK_MEDIUM = 2,
    EP_RISK_HIGH = 3
} ep_risk_t;

typedef enum {
    EP_DDC_IDLE = 0,
    EP_DDC_SNIFF = 1,
    EP_DDC_PROXY = 2,
    EP_DDC_EMULATE = 3
} ep_ddc_mode_t;

typedef struct {
    uint32_t timestamp_ms;
    ep_event_code_t code;
    ep_risk_t risk;
    char message[EP_MAX_EVENT_MESSAGE];
} ep_event_t;

typedef struct {
    char name[EP_MAX_PROFILE_NAME];
    ep_mode_t mode;
    ep_ddc_mode_t ddc_mode;
    uint8_t allow_hpd_glitch;
    uint8_t allow_cec_injection;
    uint8_t allow_edid_mutation;
    uint8_t preserve_vendor_block;
    uint8_t pulse_spacing_ms;
    uint8_t pulse_count;
    uint16_t mutate_seed;
    uint8_t preferred_audio_channels;
    uint8_t max_luminance_hint;
    uint8_t cec_rate_limit;
} ep_profile_t;

typedef struct {
    ep_mode_t mode;
    ep_ddc_mode_t ddc_mode;
    uint8_t target_connected;
    uint8_t sink_present;
    uint8_t hpd_asserted;
    uint8_t thermal_derated;
    uint8_t mutation_enabled;
    uint8_t cec_guard_enabled;
    uint32_t uptime_ms;
    uint32_t ddc_captures;
    uint32_t cec_frames;
    uint32_t hpd_pulses_sent;
    uint32_t policy_blocks;
    float board_temp_c;
    float current_draw_ma;
    char active_profile[EP_MAX_PROFILE_NAME];
} ep_status_t;

typedef struct {
    uint32_t start_ms;
    uint32_t stop_ms;
    uint8_t address;
    uint8_t offset;
    uint8_t length;
    uint8_t data[16];
    uint8_t acked;
    uint8_t checksum;
} ep_ddc_txn_t;

typedef struct {
    uint32_t timestamp_ms;
    uint8_t initiator;
    uint8_t destination;
    uint8_t opcode;
    uint8_t length;
    uint8_t payload[14];
    uint8_t valid;
} ep_cec_frame_t;

typedef struct {
    uint8_t bytes[EP_MAX_EDID_BYTES];
    size_t length;
    uint8_t checksum_valid;
    uint8_t extension_count;
    char monitor_name[16];
} ep_edid_image_t;

#endif
