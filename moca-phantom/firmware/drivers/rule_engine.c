/*
 * MoCA Phantom Rule Engine Simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <string.h>

#include "rule_engine.h"

static mph_rule_t g_rules[MPH_MAX_RULES];
static size_t g_rule_count;

static bool mph_rule_matches(const mph_rule_t *rule, const mph_frame_t *frame) {
    if (!rule->enabled) {
        return false;
    }
    if (rule->match_src != 0u && rule->match_src != frame->src_node) {
        return false;
    }
    if (rule->match_dst != 0u && rule->match_dst != frame->dst_node) {
        return false;
    }
    if (rule->match_class != frame->traffic_class) {
        return false;
    }
    if (rule->match_channel != 0xFFu && rule->match_channel != frame->channel) {
        return false;
    }
    return true;
}

static void mph_apply_action(const mph_rule_t *rule,
                             mph_frame_t *frame,
                             mph_rule_result_t *result,
                             mph_metrics_t *metrics,
                             bool safe_mode) {
    result->matched = true;
    result->rule = rule;

    switch (rule->action) {
        case MPH_RULE_DELAY:
            result->latency_added_us = rule->param_a;
            result->modified = true;
            break;
        case MPH_RULE_DROP:
            if (!safe_mode) {
                result->dropped = true;
                result->modified = true;
                metrics->frames_dropped += 1u;
            }
            break;
        case MPH_RULE_REWRITE_NODE:
            if (!safe_mode) {
                result->rewritten_dst = (uint8_t)rule->param_a;
                frame->dst_node = result->rewritten_dst;
                result->modified = true;
            }
            break;
        case MPH_RULE_REWRITE_PRIVACY:
            if (!safe_mode) {
                result->rewritten_key = rule->param_a;
                frame->privacy_key_id = result->rewritten_key;
                frame->encrypted = true;
                result->modified = true;
            }
            break;
        case MPH_RULE_FORCE_RATECAP:
            result->rewritten_rate = rule->param_a;
            if (frame->phy_rate_mbps > result->rewritten_rate) {
                frame->phy_rate_mbps = result->rewritten_rate;
                result->modified = true;
                metrics->rate_cap_events += 1u;
            }
            break;
        case MPH_RULE_TAG_ALERT:
            result->alerted = true;
            frame->anomaly = true;
            metrics->alerts_raised += 1u;
            break;
        case MPH_RULE_NONE:
        default:
            break;
    }
}

void mph_rule_engine_init(void) {
    memset(g_rules, 0, sizeof(g_rules));
    g_rule_count = 0u;
}

void mph_rule_engine_load_defaults(void) {
    g_rule_count = 5u;

    g_rules[0] = (mph_rule_t){
        .index = 1u,
        .enabled = true,
        .match_src = 3u,
        .match_dst = 5u,
        .match_class = MPH_TRAFFIC_DATA,
        .match_channel = 0xFFu,
        .action = MPH_RULE_TAG_ALERT,
        .param_a = 0u,
        .param_b = 0u,
        .label = "unencrypted-bridge-watch"
    };

    g_rules[1] = (mph_rule_t){
        .index = 2u,
        .enabled = true,
        .match_src = 5u,
        .match_dst = 3u,
        .match_class = MPH_TRAFFIC_PROBE,
        .match_channel = 4u,
        .action = MPH_RULE_REWRITE_NODE,
        .param_a = 6u,
        .param_b = 0u,
        .label = "reroute-probe-to-camera"
    };

    g_rules[2] = (mph_rule_t){
        .index = 3u,
        .enabled = true,
        .match_src = 2u,
        .match_dst = 6u,
        .match_class = MPH_TRAFFIC_DATA,
        .match_channel = 0xFFu,
        .action = MPH_RULE_REWRITE_PRIVACY,
        .param_a = 0x99u,
        .param_b = 0u,
        .label = "force-fresh-privacy-key"
    };

    g_rules[3] = (mph_rule_t){
        .index = 4u,
        .enabled = true,
        .match_src = 4u,
        .match_dst = 1u,
        .match_class = MPH_TRAFFIC_DATA,
        .match_channel = 2u,
        .action = MPH_RULE_FORCE_RATECAP,
        .param_a = 960u,
        .param_b = 0u,
        .label = "rate-cap-lab-laptop"
    };

    g_rules[4] = (mph_rule_t){
        .index = 5u,
        .enabled = true,
        .match_src = 1u,
        .match_dst = 7u,
        .match_class = MPH_TRAFFIC_PROBE,
        .match_channel = 1u,
        .action = MPH_RULE_DELAY,
        .param_a = 220u,
        .param_b = 0u,
        .label = "delay-leakage-probe"
    };
}

void mph_rule_engine_apply(mph_frame_t *frame, mph_rule_result_t *result, mph_metrics_t *metrics, bool safe_mode) {
    size_t i;
    memset(result, 0, sizeof(*result));

    if (frame->privacy_key_id == 0u || !frame->encrypted) {
        frame->anomaly = true;
        metrics->privacy_violations += 1u;
    }

    for (i = 0u; i < g_rule_count; ++i) {
        if (mph_rule_matches(&g_rules[i], frame)) {
            mph_apply_action(&g_rules[i], frame, result, metrics, safe_mode);
            break;
        }
    }
}

const mph_rule_t *mph_rule_engine_rules(void) {
    return g_rules;
}

size_t mph_rule_engine_rule_count(void) {
    return g_rule_count;
}

uint32_t mph_rule_engine_hash(void) {
    size_t i;
    uint32_t hash = 2166136261u;
    for (i = 0u; i < g_rule_count; ++i) {
        hash ^= g_rules[i].index;
        hash *= 16777619u;
        hash ^= g_rules[i].param_a;
        hash *= 16777619u;
        hash ^= g_rules[i].action;
        hash *= 16777619u;
    }
    return hash;
}
