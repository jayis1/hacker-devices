/*
 * relay.c - Relay and brownout control for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>
#include "relay.h"

void pw_relay_init(pw_relay_state_t *state)
{
    memset(state, 0, sizeof(*state));
    state->target_voltage_v = 52.0f;
}

int pw_relay_schedule_brownout(pw_relay_state_t *state, uint32_t duration_ms, float target_voltage_v, pw_event_t *event)
{
    if (duration_ms == 0 || duration_ms > PW_SAFE_BROWNOUT_MS || target_voltage_v < 32.0f) {
        return -1;
    }
    state->brownout_active = 1;
    state->brownout_remaining_ms = duration_ms;
    state->target_voltage_v = target_voltage_v;
    if (event) {
        event->timestamp_ms = 0;
        event->code = EVENT_BROWNOUT_EXECUTED;
        snprintf(event->message, sizeof(event->message), "Brownout armed for %ums target=%.1fV", duration_ms, target_voltage_v);
    }
    return 0;
}

void pw_relay_tick(pw_relay_state_t *state, pw_status_t *status, uint32_t step_ms)
{
    status->bypass_enabled = state->bypass_enabled;
    if (!state->brownout_active) {
        return;
    }
    if (state->brownout_remaining_ms <= step_ms) {
        state->brownout_remaining_ms = 0;
        state->brownout_active = 0;
        status->line_voltage_v = 52.0f;
    } else {
        state->brownout_remaining_ms -= step_ms;
        status->line_voltage_v = state->target_voltage_v;
    }
}

void pw_relay_force_bypass(pw_relay_state_t *state, pw_status_t *status, const char *reason, pw_event_t *event)
{
    state->bypass_enabled = 1;
    state->brownout_active = 0;
    state->brownout_remaining_ms = 0;
    status->bypass_enabled = 1;
    if (event) {
        event->timestamp_ms = 0;
        event->code = EVENT_ROLLBACK;
        snprintf(event->message, sizeof(event->message), "Bypass forced: %s", reason ? reason : "unspecified");
    }
}
