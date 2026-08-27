/*
 * poe_port.h - PoE port modeling and policy hooks
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef POE_WHISPER_POE_PORT_H
#define POE_WHISPER_POE_PORT_H

#include "../board.h"

typedef struct {
    char vendor[24];
    uint32_t detection_us;
    uint8_t bt_capable;
    float budget_w;
    float startup_voltage_v;
} pw_pse_fingerprint_t;

typedef struct {
    pw_poe_class_t presented_class;
    float requested_power_w;
    float delivered_voltage_v;
    float delivered_current_ma;
    uint8_t mps_valid;
    uint8_t negotiation_complete;
} pw_port_state_t;

void pw_port_init(pw_port_state_t *state);
void pw_port_apply_profile(pw_port_state_t *state, const pw_profile_t *profile);
pw_pse_fingerprint_t pw_port_fingerprint_pse(const char *vendor_hint);
void pw_port_negotiate(pw_port_state_t *state, const pw_pse_fingerprint_t *pse, pw_event_t *events, size_t *event_count, size_t max_events);
void pw_port_sample(pw_port_state_t *state, pw_status_t *status, uint32_t tick_ms);
float pw_port_estimate_budget(const pw_port_state_t *state, const pw_pse_fingerprint_t *pse);
const char *pw_port_class_name(pw_poe_class_t poe_class);

#endif
