/*
 * policy_engine.c
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "policy_engine.h"
#include "sideband_bus.h"
#include "pldm_codec.h"
#include "telemetry.h"
#include "radio_link.h"

static void add_rule(const char *name,
                     mw_policy_action_t action,
                     uint8_t src,
                     uint8_t dst,
                     uint8_t cmd,
                     uint32_t delay_ms,
                     uint8_t rewrite_mask,
                     uint8_t rewrite_value) {
    if (g_runtime.policy_count >= MW_MAX_POLICIES) {
        return;
    }
    mw_policy_rule_t *rule = &g_runtime.policies[g_runtime.policy_count++];
    memset(rule, 0, sizeof(*rule));
    rule->action = action;
    rule->match_src = src;
    rule->match_dst = dst;
    rule->match_cmd = cmd;
    rule->delay_ms = delay_ms;
    rule->rewrite_mask = rewrite_mask;
    rule->rewrite_value = rewrite_value;
    rule->enabled = true;
    snprintf(rule->name, sizeof(rule->name), "%s", name != NULL ? name : "rule");
}

static bool match_u8(uint8_t rule_value, uint8_t actual_value) {
    return rule_value == 0xFFU || rule_value == actual_value;
}

static void reset_policy_table(void) {
    memset(g_runtime.policies, 0, sizeof(g_runtime.policies));
    g_runtime.policy_count = 0U;
}

void policy_engine_init(void) {
    reset_policy_table();
}

void policy_engine_load_defaults(void) {
    reset_policy_table();
    add_rule("mirror all host traffic", MW_POLICY_MIRROR, 8U, 0xFFU, 0xFFU, 0U, 0U, 0U);
    add_rule("alert on unsigned sensor data", MW_POLICY_ALERT, 0xFFU, 8U, 0x50U, 0U, 0U, 0U);
    add_rule("baseline pass for discovery", MW_POLICY_PASS, 0xFFU, 0xFFU, 0x0AU, 0U, 0U, 0U);
}

void policy_engine_arm_scenario(mw_scenario_kind_t kind) {
    memset(&g_runtime.scenario, 0, sizeof(g_runtime.scenario));
    g_runtime.scenario.kind = kind;
    snprintf(g_runtime.scenario.name, sizeof(g_runtime.scenario.name), "%s", mw_scenario_name(kind));
    g_runtime.scenario.active = true;
    g_runtime.scenario.triggered = true;
    g_runtime.scenario.completed = false;
    g_runtime.scenario.start_ms = g_now_ms;
    g_runtime.scenario.step_index = 0U;
    g_runtime.flags |= MW_FLAG_MUTATION_ARMED;
    g_runtime.flags &= ~MW_FLAG_PASSIVE;
    sideband_bus_enable_mutation(true);
    mw_log_event(MW_EVENT_SCENARIO_START, (uint32_t)kind, 0U, g_runtime.scenario.name);

    switch (kind) {
        case MW_SCENARIO_ROUTE_POISON:
            add_rule("route poison delay", MW_POLICY_DELAY, 44U, 8U, 0x0AU, 18U, 0U, 0U);
            add_rule("route poison rewrite", MW_POLICY_REWRITE, 44U, 8U, 0x0AU, 0U, 0x0FU, 0x0CU);
            break;
        case MW_SCENARIO_SPDM_DOWNGRADE_PROBE:
            add_rule("spdm version floor", MW_POLICY_REWRITE, 8U, 20U, 0x84U, 0U, 0xF0U, 0x10U);
            add_rule("spdm hold-open", MW_POLICY_DELAY, 8U, 20U, 0x84U, 35U, 0U, 0U);
            break;
        case MW_SCENARIO_PLDM_STAGE_FUZZ:
            add_rule("firmware stage scramble", MW_POLICY_REWRITE, 8U, 33U, 0x91U, 0U, 0xAAU, 0x55U);
            add_rule("stage mirror", MW_POLICY_MIRROR, 8U, 33U, 0x91U, 0U, 0U, 0U);
            break;
        case MW_SCENARIO_SENSOR_GHOST:
            add_rule("sensor falsify", MW_POLICY_REWRITE, 33U, 8U, 0x50U, 0U, 0x00U, 0x05U);
            break;
        case MW_SCENARIO_ENDPOINT_CLONE:
            add_rule("clone advert mutate", MW_POLICY_REWRITE, 44U, 8U, 0x0AU, 0U, 0x7FU, 0x3EU);
            add_rule("clone alert", MW_POLICY_ALERT, 8U, 44U, 0x92U, 0U, 0U, 0U);
            break;
        case MW_SCENARIO_IDLE:
        default:
            break;
    }
}

void policy_engine_tick(void) {
    if (!g_runtime.scenario.active || g_runtime.scenario.completed) {
        return;
    }

    const uint32_t elapsed = g_now_ms - g_runtime.scenario.start_ms;
    if (elapsed > 140U && g_runtime.scenario.step_index == 0U) {
        radio_link_send_status("scenario", "mutation-active");
        g_runtime.scenario.step_index = 1U;
    }
    if (elapsed > 260U && g_runtime.scenario.step_index == 1U) {
        radio_link_send_status("scenario", "capture-signed");
        telemetry_note_signed_bundle();
        g_runtime.flags |= MW_FLAG_EVIDENCE_READY;
        g_runtime.scenario.step_index = 2U;
    }
    if (elapsed > 320U && g_runtime.scenario.step_index == 2U) {
        g_runtime.scenario.active = false;
        g_runtime.scenario.completed = true;
        g_runtime.flags &= ~MW_FLAG_MUTATION_ARMED;
        g_runtime.flags |= MW_FLAG_PASSIVE;
        sideband_bus_enable_mutation(false);
        mw_log_event(MW_EVENT_SCENARIO_COMPLETE, (uint32_t)g_runtime.scenario.kind, elapsed, g_runtime.scenario.name);
    }
}

bool policy_engine_apply(mw_message_t *message) {
    bool forward = true;
    if (message == NULL) {
        return false;
    }

    for (size_t i = 0; i < g_runtime.policy_count; ++i) {
        mw_policy_rule_t *rule = &g_runtime.policies[i];
        if (!rule->enabled) {
            continue;
        }
        if (!match_u8(rule->match_src, message->eid_src) ||
            !match_u8(rule->match_dst, message->eid_dst) ||
            !match_u8(rule->match_cmd, message->command_code)) {
            continue;
        }

        switch (rule->action) {
            case MW_POLICY_PASS:
                mw_log_event(MW_EVENT_POLICY, i, rule->action, rule->name);
                break;
            case MW_POLICY_MIRROR:
                sideband_bus_capture_message(message, "mirror");
                telemetry_note_capture();
                mw_log_event(MW_EVENT_CAPTURE, i, message->command_code, rule->name);
                break;
            case MW_POLICY_DELAY:
                sideband_bus_set_delay_budget(rule->delay_ms);
                mw_log_event(MW_EVENT_POLICY, i, rule->delay_ms, rule->name);
                break;
            case MW_POLICY_REWRITE:
                if (message->command_code == 0x84U) {
                    pldm_codec_rewrite_version_floor(message, rule->rewrite_value);
                } else if (message->command_code == 0x50U) {
                    pldm_codec_rewrite_sensor(message, rule->rewrite_value);
                } else if (message->command_code == 0x91U) {
                    pldm_codec_scramble_payload(message, rule->rewrite_value);
                } else if (message->payload_len > 0U) {
                    message->payload[0] = (uint8_t)((message->payload[0] & (uint8_t)~rule->rewrite_mask) |
                                                    (rule->rewrite_value & rule->rewrite_mask));
                }
                telemetry_note_mutation();
                mw_log_event(MW_EVENT_MUTATION, i, pldm_codec_measure(message), rule->name);
                break;
            case MW_POLICY_DROP:
                telemetry_note_drop();
                mw_log_event(MW_EVENT_POLICY, i, rule->action, rule->name);
                forward = false;
                break;
            case MW_POLICY_ALERT:
                g_runtime.flags |= MW_FLAG_ALERT_ASSERTED;
                telemetry_note_alert();
                mw_log_event(MW_EVENT_ALERT, i, message->command_code, rule->name);
                radio_link_send_status("alert", rule->name);
                break;
            default:
                break;
        }
    }

    sideband_bus_capture_message(message, forward ? "post-policy" : "dropped");
    telemetry_note_capture();
    return forward;
}

void policy_engine_dump(void) {
    printf("[policy] rules=%zu scenario=%s\n", g_runtime.policy_count, g_runtime.scenario.name);
    for (size_t i = 0; i < g_runtime.policy_count; ++i) {
        const mw_policy_rule_t *rule = &g_runtime.policies[i];
        printf("  [%02zu] enabled=%u action=%u src=%u dst=%u cmd=0x%02x delay=%u name=%s\n",
               i,
               rule->enabled ? 1U : 0U,
               rule->action,
               rule->match_src,
               rule->match_dst,
               rule->match_cmd,
               rule->delay_ms,
               rule->name);
    }
}
