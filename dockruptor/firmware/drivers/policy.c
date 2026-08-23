/*
 * Dockruptor Policy Engine
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>
#include <string.h>

#include "policy.h"
#include "capture.h"
#include "pd_engine.h"

const char *dr_policy_rule_name(dr_rule_type_t type)
{
    switch (type) {
    case DR_RULE_CLAMP_POWER:
        return "clamp-power";
    case DR_RULE_REORDER_PDO:
        return "reorder-pdo";
    case DR_RULE_DELAY_ALTMODE:
        return "delay-altmode";
    case DR_RULE_FORCE_CHARGE_ONLY:
        return "charge-only";
    case DR_RULE_SPOOF_CABLE:
        return "spoof-cable";
    case DR_RULE_TRIGGER_SOFT_RESET:
        return "soft-reset";
    default:
        return "none";
    }
}

void dr_policy_init(dr_context_t *ctx)
{
    ctx->rule_count = 0;
    memset(ctx->rules, 0, sizeof(ctx->rules));
}

void dr_policy_load_default_profile(dr_context_t *ctx)
{
    dr_rule_t *rule = NULL;

    ctx->rule_count = 4;

    rule = &ctx->rules[0];
    rule->type = DR_RULE_CLAMP_POWER;
    rule->enabled = true;
    rule->limit_mv = 9000U;
    rule->limit_ma = 2000U;
    snprintf(rule->name, sizeof(rule->name), "Travel Dock Power Clamp");

    rule = &ctx->rules[1];
    rule->type = DR_RULE_DELAY_ALTMODE;
    rule->enabled = true;
    rule->holdoff_ticks = 5U;
    snprintf(rule->name, sizeof(rule->name), "Delay DisplayPort Window");

    rule = &ctx->rules[2];
    rule->type = DR_RULE_SPOOF_CABLE;
    rule->enabled = true;
    snprintf(rule->name, sizeof(rule->name), "Downgrade Cable Identity");

    rule = &ctx->rules[3];
    rule->type = DR_RULE_TRIGGER_SOFT_RESET;
    rule->enabled = true;
    rule->holdoff_ticks = 9U;
    snprintf(rule->name, sizeof(rule->name), "Single Soft Reset After Attach");
}

static void apply_clamp_power(dr_context_t *ctx, dr_rule_t *rule)
{
    char buffer[DR_MAX_TEXT];
    size_t i = 0;

    for (i = 0; i < ctx->dock.capability_count; ++i) {
        dr_capability_t *cap = &ctx->dock.capabilities[i];
        if (cap->millivolts > rule->limit_mv) {
            cap->millivolts = rule->limit_mv;
        }
        if (cap->milliamps > rule->limit_ma) {
            cap->milliamps = rule->limit_ma;
        }
    }

    if (ctx->host.negotiated_mv > rule->limit_mv) {
        ctx->host.negotiated_mv = rule->limit_mv;
    }
    if (ctx->host.negotiated_ma > rule->limit_ma) {
        ctx->host.negotiated_ma = rule->limit_ma;
    }

    snprintf(buffer,
             sizeof(buffer),
             "%s applied %umV/%umA ceiling",
             rule->name,
             rule->limit_mv,
             rule->limit_ma);
    dr_capture_event(ctx, DR_EVENT_POLICY, DR_DIR_INTERNAL, buffer);
}

static void apply_delay_altmode(dr_context_t *ctx, dr_rule_t *rule)
{
    char buffer[DR_MAX_TEXT];

    if (ctx->tick < rule->holdoff_ticks) {
        ctx->host.mode = DR_MODE_USB;
        ctx->dock.mode = DR_MODE_USB;
        snprintf(buffer,
                 sizeof(buffer),
                 "%s holding alt-mode until tick %u",
                 rule->name,
                 rule->holdoff_ticks);
        dr_capture_event(ctx, DR_EVENT_POLICY, DR_DIR_INTERNAL, buffer);
    } else if (ctx->host.mode == DR_MODE_USB) {
        dr_pd_force_mode(ctx, DR_MODE_DP);
        snprintf(buffer, sizeof(buffer), "%s released DisplayPort entry", rule->name);
        dr_capture_event(ctx, DR_EVENT_POLICY, DR_DIR_INTERNAL, buffer);
        rule->enabled = false;
    }
}

static void apply_force_charge_only(dr_context_t *ctx, dr_rule_t *rule)
{
    char buffer[DR_MAX_TEXT];

    (void)rule;
    ctx->host.mode = DR_MODE_USB;
    ctx->dock.mode = DR_MODE_USB;
    ctx->host.data_role = DR_ROLE_SINK;
    ctx->dock.data_role = DR_ROLE_SOURCE;
    snprintf(buffer, sizeof(buffer), "charge-only policy locked data role out");
    dr_capture_event(ctx, DR_EVENT_POLICY, DR_DIR_INTERNAL, buffer);
}

static void apply_spoof_cable(dr_context_t *ctx, dr_rule_t *rule)
{
    char buffer[DR_MAX_TEXT];

    (void)rule;
    snprintf(ctx->host.cable_identity,
             sizeof(ctx->host.cable_identity),
             "%s",
             "Passive USB2 cable, no alt-mode assurance");
    snprintf(ctx->dock.cable_identity,
             sizeof(ctx->dock.cable_identity),
             "%s",
             "Passive USB2 cable, no alt-mode assurance");
    snprintf(buffer, sizeof(buffer), "cable identity spoofed to passive low-capability profile");
    dr_capture_event(ctx, DR_EVENT_POLICY, DR_DIR_INTERNAL, buffer);
}

static void apply_soft_reset(dr_context_t *ctx, dr_rule_t *rule)
{
    char buffer[DR_MAX_TEXT];

    if (ctx->tick == rule->holdoff_ticks) {
        dr_pd_soft_reset(ctx);
        snprintf(buffer, sizeof(buffer), "%s fired at tick %u", rule->name, rule->holdoff_ticks);
        dr_capture_event(ctx, DR_EVENT_POLICY, DR_DIR_INTERNAL, buffer);
        rule->enabled = false;
    }
}

void dr_policy_apply(dr_context_t *ctx)
{
    size_t i = 0;

    if (ctx->observe_only) {
        return;
    }

    for (i = 0; i < ctx->rule_count; ++i) {
        dr_rule_t *rule = &ctx->rules[i];
        if (!rule->enabled) {
            continue;
        }

        switch (rule->type) {
        case DR_RULE_CLAMP_POWER:
            apply_clamp_power(ctx, rule);
            rule->enabled = false;
            break;
        case DR_RULE_REORDER_PDO:
            break;
        case DR_RULE_DELAY_ALTMODE:
            apply_delay_altmode(ctx, rule);
            break;
        case DR_RULE_FORCE_CHARGE_ONLY:
            apply_force_charge_only(ctx, rule);
            break;
        case DR_RULE_SPOOF_CABLE:
            apply_spoof_cable(ctx, rule);
            rule->enabled = false;
            break;
        case DR_RULE_TRIGGER_SOFT_RESET:
            apply_soft_reset(ctx, rule);
            break;
        default:
            break;
        }
    }
}
