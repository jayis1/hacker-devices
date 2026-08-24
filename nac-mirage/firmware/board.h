/*
 * nac-mirage firmware board definition
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NAC_MIRAGE_BOARD_H
#define NAC_MIRAGE_BOARD_H

#include <stdint.h>
#include <stdbool.h>

#define NM_DEVICE_NAME "NAC Mirage"
#define NM_AUTHOR "jayis1"
#define NM_VERSION_MAJOR 1U
#define NM_VERSION_MINOR 0U
#define NM_VERSION_PATCH 0U

#define NM_MAX_EVENTS 256U
#define NM_MAX_PORTS 2U
#define NM_MAX_NEIGHBORS 8U
#define NM_MAX_POLICIES 6U
#define NM_MAC_STRING_LEN 18U
#define NM_TEXT_32 32U
#define NM_TEXT_64 64U
#define NM_TEXT_128 128U
#define NM_TEXT_256 256U

#define NM_PORT_SWITCH 0U
#define NM_PORT_ENDPOINT 1U

#define NM_ETHERTYPE_LLDP 0x88CCU
#define NM_ETHERTYPE_EAPOL 0x888EU
#define NM_ETHERTYPE_IPV4 0x0800U
#define NM_ETHERTYPE_ARP  0x0806U

#define NM_POE_CLASS_0 0U
#define NM_POE_CLASS_1 1U
#define NM_POE_CLASS_2 2U
#define NM_POE_CLASS_3 3U
#define NM_POE_CLASS_4 4U
#define NM_POE_CLASS_6 6U

#define NM_CAPTURE_REASON_FORWARD 0U
#define NM_CAPTURE_REASON_MUTATE 1U
#define NM_CAPTURE_REASON_DROP 2U
#define NM_CAPTURE_REASON_ALERT 3U

#define NM_PROFILE_TRANSPARENT 0U
#define NM_PROFILE_LAB_PHANTOM_PHONE 1U
#define NM_PROFILE_CAMERA_BROWNOUT 2U
#define NM_PROFILE_VOICE_VLAN_DECOY 3U
#define NM_PROFILE_NAC_DELAY 4U
#define NM_PROFILE_STAGED_RELAY 5U

#define NM_PROFILE_COUNT 6U

typedef struct {
    uint8_t bytes[6];
} nm_mac_t;

typedef struct {
    char chassis_id[NM_TEXT_64];
    char port_id[NM_TEXT_64];
    char system_name[NM_TEXT_64];
    char system_desc[NM_TEXT_128];
    uint16_t vlan;
    uint16_t voice_vlan;
    uint16_t power_mw;
    bool dot1x_required;
    bool phone_capable;
} nm_neighbor_t;

typedef struct {
    uint8_t profile_id;
    char name[NM_TEXT_32];
    bool mutate_lldp;
    bool intercept_eapol;
    bool delay_link_up;
    bool enable_poe_glitch;
    bool rewrite_vlan;
    bool mirror_traffic;
    uint16_t forced_vlan;
    uint16_t forced_voice_vlan;
    uint16_t poe_budget_mw;
    uint16_t link_delay_ms;
} nm_policy_t;

typedef struct {
    uint32_t uptime_ms;
    uint32_t frames_seen;
    uint32_t frames_mutated;
    uint32_t frames_dropped;
    uint32_t alerts;
    uint16_t current_vlan;
    uint16_t advertised_vlan;
    uint16_t poe_budget_mw;
    bool relay_open;
    bool bypass_engaged;
    bool radio_connected;
} nm_runtime_status_t;

#endif
