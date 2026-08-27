/*
 * poe_port.c - PoE port modeling and policy hooks
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>
#include "poe_port.h"

static float class_to_nominal_power(pw_poe_class_t poe_class)
{
    switch (poe_class) {
        case PW_CLASS_1: return 4.0f;
        case PW_CLASS_2: return 7.0f;
        case PW_CLASS_3: return 15.4f;
        case PW_CLASS_4: return 30.0f;
        case PW_CLASS_5: return 45.0f;
        case PW_CLASS_6: return 60.0f;
        case PW_CLASS_7: return 75.0f;
        case PW_CLASS_8: return 90.0f;
        case PW_CLASS_0:
        default: return 13.0f;
    }
}

const char *pw_port_class_name(pw_poe_class_t poe_class)
{
    switch (poe_class) {
        case PW_CLASS_0: return "class-0";
        case PW_CLASS_1: return "class-1";
        case PW_CLASS_2: return "class-2";
        case PW_CLASS_3: return "class-3";
        case PW_CLASS_4: return "class-4";
        case PW_CLASS_5: return "class-5";
        case PW_CLASS_6: return "class-6";
        case PW_CLASS_7: return "class-7";
        case PW_CLASS_8: return "class-8";
        default: return "class-unknown";
    }
}

void pw_port_init(pw_port_state_t *state)
{
    memset(state, 0, sizeof(*state));
    state->presented_class = PW_CLASS_4;
    state->requested_power_w = PW_DEFAULT_BUDGET_W;
    state->delivered_voltage_v = 52.0f;
    state->delivered_current_ma = 210.0f;
    state->mps_valid = 1;
}

void pw_port_apply_profile(pw_port_state_t *state, const pw_profile_t *profile)
{
    if (!state || !profile) {
        return;
    }
    state->presented_class = profile->advertised_class;
    state->requested_power_w = profile->requested_power_w;
}

pw_pse_fingerprint_t pw_port_fingerprint_pse(const char *vendor_hint)
{
    pw_pse_fingerprint_t fp;
    memset(&fp, 0, sizeof(fp));
    if (vendor_hint && strstr(vendor_hint, "Cisco")) {
        snprintf(fp.vendor, sizeof(fp.vendor), "Cisco Catalyst");
        fp.detection_us = 1820;
        fp.bt_capable = 1;
        fp.budget_w = 60.0f;
        fp.startup_voltage_v = 54.1f;
    } else if (vendor_hint && strstr(vendor_hint, "Aruba")) {
        snprintf(fp.vendor, sizeof(fp.vendor), "Aruba Access");
        fp.detection_us = 2050;
        fp.bt_capable = 1;
        fp.budget_w = 45.0f;
        fp.startup_voltage_v = 53.4f;
    } else {
        snprintf(fp.vendor, sizeof(fp.vendor), "Generic PSE");
        fp.detection_us = 2400;
        fp.bt_capable = 0;
        fp.budget_w = 30.0f;
        fp.startup_voltage_v = 51.2f;
    }
    return fp;
}

float pw_port_estimate_budget(const pw_port_state_t *state, const pw_pse_fingerprint_t *pse)
{
    float class_power = class_to_nominal_power(state->presented_class);
    float headroom = pse->budget_w - class_power;
    if (headroom < 0.0f) {
        headroom = 0.0f;
    }
    if (state->requested_power_w < class_power) {
        return class_power;
    }
    if (state->requested_power_w > pse->budget_w) {
        return pse->budget_w;
    }
    return state->requested_power_w + (headroom * 0.15f);
}

static void push_event(pw_event_t *events, size_t *event_count, size_t max_events, pw_event_code_t code, const char *msg, uint32_t ts)
{
    if (*event_count >= max_events) {
        return;
    }
    events[*event_count].timestamp_ms = ts;
    events[*event_count].code = code;
    snprintf(events[*event_count].message, sizeof(events[*event_count].message), "%s", msg);
    (*event_count)++;
}

void pw_port_negotiate(pw_port_state_t *state, const pw_pse_fingerprint_t *pse, pw_event_t *events, size_t *event_count, size_t max_events)
{
    char line[112];
    snprintf(line, sizeof(line), "PSE fingerprint vendor=%s detect=%uus budget=%.1fW", pse->vendor, pse->detection_us, pse->budget_w);
    push_event(events, event_count, max_events, EVENT_PSE_FINGERPRINT, line, 6);

    snprintf(line, sizeof(line), "PD presents %s request=%.1fW", pw_port_class_name(state->presented_class), state->requested_power_w);
    push_event(events, event_count, max_events, EVENT_PD_CLASS_PRESENTED, line, 11);

    state->negotiation_complete = 1;
    state->delivered_voltage_v = pse->startup_voltage_v;
    state->delivered_current_ma = (state->requested_power_w * 1000.0f) / state->delivered_voltage_v;
    if (state->delivered_current_ma < 120.0f) {
        state->delivered_current_ma = 120.0f;
    }

    snprintf(line, sizeof(line), "Allocated %.1fW at %.1fV / %.0fmA", pw_port_estimate_budget(state, pse), state->delivered_voltage_v, state->delivered_current_ma);
    push_event(events, event_count, max_events, EVENT_PD_CLASS_PRESENTED, line, 18);

    if (state->requested_power_w > pse->budget_w) {
        snprintf(line, sizeof(line), "Requested power exceeds budget; PSE likely clamps to %.1fW", pse->budget_w);
        push_event(events, event_count, max_events, EVENT_OPERATOR_COMMAND, line, 22);
    }
}

void pw_port_sample(pw_port_state_t *state, pw_status_t *status, uint32_t tick_ms)
{
    float modulation = (float)((tick_ms / 10u) % 9u) * 4.5f;
    status->line_voltage_v = state->delivered_voltage_v;
    status->line_current_ma = state->delivered_current_ma + modulation;
    status->allocated_power_w = (status->line_voltage_v * status->line_current_ma) / 1000.0f;
    status->detected_class = state->presented_class;
    status->link_up = 1;
}
