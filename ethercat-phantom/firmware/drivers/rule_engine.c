/*
 * Rule engine for process-data manipulations
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "rule_engine.h"

#include <string.h>

static eph_rule_t g_rules[EPH_MAX_RULES];
static size_t g_rule_count;

static uint32_t eph_hash_bytes(const uint8_t *data, size_t len) {
    uint32_t hash = 2166136261u;
    size_t i;
    for (i = 0u; i < len; ++i) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

void eph_rule_engine_init(void) {
    memset(g_rules, 0, sizeof(g_rules));
    g_rule_count = 0u;
}

void eph_rule_engine_clear(void) {
    eph_rule_engine_init();
}

bool eph_rule_engine_add(const eph_rule_t *rule) {
    if (rule == NULL || g_rule_count >= EPH_MAX_RULES) {
        return false;
    }
    g_rules[g_rule_count++] = *rule;
    return true;
}

void eph_rule_engine_load_defaults(void) {
    eph_rule_t defaults[] = {
        {0u, true, 0x7000u, 0x01u, 0x02u, EPH_RULE_CLAMP_RANGE, 0, 1400, "Clamp conveyor"},
        {1u, true, 0x7000u, 0x02u, 0x03u, EPH_RULE_OFFSET_VALUE, -10, 0, "Derate heater"},
        {2u, true, 0x607Au, 0x00u, 0x04u, EPH_RULE_FORCE_VALUE, 42000, 0, "Spoof servo target"},
        {3u, true, 0x7010u, 0x01u, 0x07u, EPH_RULE_BIT_TOGGLE, 0, 0, "Flip safety latch"},
        {4u, true, 0x7020u, 0x01u, 0x05u, EPH_RULE_MONITOR_ONLY, 0, 0, "Log valve movement"}
    };
    size_t i;
    eph_rule_engine_clear();
    for (i = 0u; i < sizeof(defaults) / sizeof(defaults[0]); ++i) {
        eph_rule_engine_add(&defaults[i]);
    }
}

size_t eph_rule_engine_rule_count(void) {
    return g_rule_count;
}

const eph_rule_t *eph_rule_engine_rules(void) {
    return g_rules;
}

uint32_t eph_rule_engine_hash(void) {
    return eph_hash_bytes((const uint8_t *)g_rules, g_rule_count * sizeof(g_rules[0]));
}

static bool eph_rule_matches(const eph_rule_t *rule, const eph_frame_view_t *frame) {
    return rule->enabled &&
           rule->matcher_slave == frame->slave_address &&
           rule->matcher_index == frame->object_index &&
           rule->matcher_subindex == frame->object_subindex;
}

static int32_t eph_clamp_i32(int32_t value, int32_t low, int32_t high) {
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

static bool eph_is_sensitive_object(const eph_frame_view_t *frame) {
    return frame->object_index == 0x7010u || frame->object_index == 0x6040u;
}

void eph_rule_engine_apply(const eph_frame_view_t *frame, eph_rule_result_t *result, eph_metrics_t *metrics, bool safe_mode) {
    size_t i;

    if (frame == NULL || result == NULL || metrics == NULL) {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->value_after = frame->process_value;

    for (i = 0u; i < g_rule_count; ++i) {
        const eph_rule_t *rule = &g_rules[i];
        if (!eph_rule_matches(rule, frame)) {
            continue;
        }

        result->matched = true;
        result->rule = rule;
        metrics->rules_triggered += 1u;

        if (rule->action == EPH_RULE_MONITOR_ONLY) {
            return;
        }

        if (safe_mode && eph_is_sensitive_object(frame)) {
            return;
        }

        switch (rule->action) {
            case EPH_RULE_CLAMP_RANGE:
                result->value_after = eph_clamp_i32(frame->process_value, rule->param_a, rule->param_b);
                break;
            case EPH_RULE_OFFSET_VALUE:
                result->value_after = frame->process_value + rule->param_a;
                break;
            case EPH_RULE_FORCE_VALUE:
                result->value_after = rule->param_a;
                break;
            case EPH_RULE_BIT_TOGGLE:
                result->value_after = frame->process_value ^ (1 << (rule->param_a & 31));
                break;
            case EPH_RULE_DISABLED:
            case EPH_RULE_MONITOR_ONLY:
            default:
                result->value_after = frame->process_value;
                break;
        }

        result->modified = (result->value_after != frame->process_value);
        return;
    }
}
