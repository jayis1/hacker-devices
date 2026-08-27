/*
 * relay.h - Relay and brownout control for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef POE_WHISPER_RELAY_H
#define POE_WHISPER_RELAY_H

#include "../board.h"

typedef struct {
    uint8_t bypass_enabled;
    uint8_t brownout_active;
    uint32_t brownout_remaining_ms;
    float target_voltage_v;
} pw_relay_state_t;

void pw_relay_init(pw_relay_state_t *state);
int pw_relay_schedule_brownout(pw_relay_state_t *state, uint32_t duration_ms, float target_voltage_v, pw_event_t *event);
void pw_relay_tick(pw_relay_state_t *state, pw_status_t *status, uint32_t step_ms);
void pw_relay_force_bypass(pw_relay_state_t *state, pw_status_t *status, const char *reason, pw_event_t *event);

#endif
