/*
 * Fail-safe authorization and command policy engine
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "policy.h"

#include <string.h>

static const char scope_statement[] = "I HAVE AUTHORIZATION FOR THIS SD/SDIO TARGET";

static void audit_append(struct ss_policy *policy, const struct ss_frame *frame,
                         const struct ss_analysis *analysis,
                         enum ss_policy_action action, uint32_t now_ms)
{
    struct ss_audit_entry *entry = &policy->audit[policy->audit_head];
    entry->timestamp_ms = now_ms;
    entry->flags = analysis->flags;
    entry->argument = frame->argument;
    entry->sequence = frame->sequence;
    entry->command = frame->command;
    entry->action = (uint8_t)action;
    entry->severity = (uint8_t)analysis->severity;
    policy->audit_head = (policy->audit_head + 1u) % SS_AUDIT_CAPACITY;
    if (policy->audit_count < SS_AUDIT_CAPACITY) {
        policy->audit_count++;
    }
}

static void install_default_rule(struct ss_policy *policy, uint8_t command,
                                 uint32_t value, uint32_t mask,
                                 enum ss_policy_action action)
{
    struct ss_policy_rule rule;
    memset(&rule, 0, sizeof(rule));
    rule.command = command;
    rule.command_mask = 0x3Fu;
    rule.argument_value = value;
    rule.argument_mask = mask;
    rule.direction = SS_DIR_HOST_TO_CARD;
    rule.action = action;
    rule.enabled = true;
    (void)ss_policy_add_rule(policy, &rule);
}

static bool action_is_active(enum ss_policy_action action)
{
    return action == SS_ACTION_BLOCK || action == SS_ACTION_REQUIRE_PHYSICAL_ARM;
}

static bool authorization_current(const struct ss_policy *policy, uint32_t now_ms)
{
    return policy->operator_confirmed && policy->arm_switch_seen &&
           now_ms < policy->armed_until_ms &&
           now_ms - policy->session_started_ms < SS_MAX_ACTIVE_SECONDS * 1000u;
}

static bool rule_matches(const struct ss_policy_rule *rule,
                         const struct ss_frame *frame)
{
    if (!rule->enabled || rule->direction != frame->direction) {
        return false;
    }
    uint8_t command_delta = (uint8_t)((frame->command ^ rule->command) &
                                      rule->command_mask);
    uint32_t argument_delta = (frame->argument ^ rule->argument_value) &
                              rule->argument_mask;
    return command_delta == 0u && argument_delta == 0u;
}

void ss_policy_init(struct ss_policy *policy, uint32_t now_ms)
{
    if (policy == NULL) {
        return;
    }
    memset(policy, 0, sizeof(*policy));
    policy->mode = SS_MODE_SAFE_BYPASS;
    policy->session_started_ms = now_ms;
    policy->media_write_allowed = false;

    /* Destructive or identity-changing commands require a physical hold. */
    install_default_rule(policy, 27u, 0u, 0u, SS_ACTION_REQUIRE_PHYSICAL_ARM);
    install_default_rule(policy, 28u, 0u, 0u, SS_ACTION_REQUIRE_PHYSICAL_ARM);
    install_default_rule(policy, 29u, 0u, 0u, SS_ACTION_REQUIRE_PHYSICAL_ARM);
    install_default_rule(policy, 32u, 0u, 0u, SS_ACTION_REQUIRE_PHYSICAL_ARM);
    install_default_rule(policy, 33u, 0u, 0u, SS_ACTION_REQUIRE_PHYSICAL_ARM);
    install_default_rule(policy, 38u, 0u, 0u, SS_ACTION_REQUIRE_PHYSICAL_ARM);
    install_default_rule(policy, 42u, 0u, 0u, SS_ACTION_REQUIRE_PHYSICAL_ARM);
    install_default_rule(policy, 56u, 0u, 0u, SS_ACTION_REQUIRE_PHYSICAL_ARM);

    /* Writes to the first 4 MiB are logged and blocked unless explicitly armed. */
    install_default_rule(policy, 24u, 0u, 0xFFFFE000u,
                         SS_ACTION_REQUIRE_PHYSICAL_ARM);
    install_default_rule(policy, 25u, 0u, 0xFFFFE000u,
                         SS_ACTION_REQUIRE_PHYSICAL_ARM);

    /* SDIO function writes: bit 31 is the R/W flag for CMD52/CMD53. */
    install_default_rule(policy, 52u, 0x80000000u, 0x80000000u,
                         SS_ACTION_REQUIRE_PHYSICAL_ARM);
    install_default_rule(policy, 53u, 0x80000000u, 0x80000000u,
                         SS_ACTION_REQUIRE_PHYSICAL_ARM);
}

bool ss_policy_add_rule(struct ss_policy *policy, const struct ss_policy_rule *rule)
{
    if (policy == NULL || rule == NULL || policy->rule_count >= SS_POLICY_RULES ||
        rule->command >= 64u || rule->action > SS_ACTION_REQUIRE_PHYSICAL_ARM) {
        return false;
    }
    policy->rules[policy->rule_count] = *rule;
    policy->rule_count++;
    return true;
}

void ss_policy_clear_custom_rules(struct ss_policy *policy)
{
    if (policy == NULL) {
        return;
    }
    size_t defaults = policy->rule_count < 12u ? policy->rule_count : 12u;
    for (size_t i = defaults; i < policy->rule_count; ++i) {
        memset(&policy->rules[i], 0, sizeof(policy->rules[i]));
    }
    policy->rule_count = defaults;
}

void ss_policy_note_arm_switch(struct ss_policy *policy, bool pressed, uint32_t now_ms)
{
    if (policy == NULL) {
        return;
    }
    if (pressed) {
        if (policy->arm_started_ms == 0u) {
            policy->arm_started_ms = now_ms == 0u ? 1u : now_ms;
        }
        if (now_ms - policy->arm_started_ms >= SS_ARM_HOLD_MS) {
            policy->arm_switch_seen = true;
            if (policy->operator_confirmed) {
                policy->armed_until_ms = now_ms + SS_INACTIVITY_TIMEOUT_MS;
            }
        }
    } else {
        policy->arm_started_ms = 0u;
    }
}

bool ss_policy_confirm_operator(struct ss_policy *policy, const char *statement,
                                uint32_t now_ms)
{
    if (policy == NULL || statement == NULL) {
        return false;
    }
    size_t expected = sizeof(scope_statement) - 1u;
    size_t supplied = strlen(statement);
    if (supplied != expected || memcmp(statement, scope_statement, expected) != 0) {
        policy->operator_confirmed = false;
        policy->armed_until_ms = 0u;
        return false;
    }
    policy->operator_confirmed = true;
    policy->session_started_ms = now_ms;
    if (policy->arm_switch_seen) {
        policy->armed_until_ms = now_ms + SS_INACTIVITY_TIMEOUT_MS;
    }
    return true;
}

void ss_policy_tick(struct ss_policy *policy, uint32_t now_ms)
{
    if (policy == NULL) {
        return;
    }
    if (now_ms >= policy->armed_until_ms) {
        policy->arm_switch_seen = false;
        policy->armed_until_ms = 0u;
        policy->media_write_allowed = false;
        if (policy->mode == SS_MODE_ENFORCE_POLICY ||
            policy->mode == SS_MODE_LAB_EMULATION) {
            policy->mode = SS_MODE_PASSIVE_OBSERVE;
        }
    }
    if (now_ms - policy->session_started_ms >= SS_MAX_ACTIVE_SECONDS * 1000u) {
        policy->operator_confirmed = false;
        policy->mode = SS_MODE_SAFE_BYPASS;
    }
}

bool ss_policy_set_mode(struct ss_policy *policy, enum ss_mode requested,
                        uint32_t now_ms)
{
    if (policy == NULL || requested > SS_MODE_FAULT) {
        return false;
    }
    if (requested == SS_MODE_SAFE_BYPASS || requested == SS_MODE_PASSIVE_OBSERVE) {
        policy->mode = requested;
        return true;
    }
    if (requested == SS_MODE_FAULT || !authorization_current(policy, now_ms)) {
        return false;
    }
    policy->mode = requested;
    policy->media_write_allowed = requested == SS_MODE_LAB_EMULATION;
    policy->armed_until_ms = now_ms + SS_INACTIVITY_TIMEOUT_MS;
    return true;
}

enum ss_policy_action ss_policy_evaluate(struct ss_policy *policy,
                                         const struct ss_frame *frame,
                                         const struct ss_analysis *analysis,
                                         uint32_t now_ms)
{
    if (policy == NULL || frame == NULL || analysis == NULL) {
        return SS_ACTION_BLOCK;
    }
    ss_policy_tick(policy, now_ms);
    enum ss_policy_action selected = SS_ACTION_ALLOW;

    if (policy->mode == SS_MODE_SAFE_BYPASS) {
        selected = SS_ACTION_ALLOW;
    } else if (policy->mode == SS_MODE_PASSIVE_OBSERVE) {
        selected = analysis->severity >= SS_SEV_NOTICE ? SS_ACTION_ALLOW_AND_LOG
                                                       : SS_ACTION_ALLOW;
    } else {
        for (size_t i = 0u; i < policy->rule_count; ++i) {
            if (rule_matches(&policy->rules[i], frame)) {
                selected = policy->rules[i].action;
                break;
            }
        }
        if ((analysis->flags & SS_FLAG_WRITE_BOOT_REGION) != 0u &&
            selected == SS_ACTION_ALLOW) {
            selected = SS_ACTION_REQUIRE_PHYSICAL_ARM;
        }
        if (selected == SS_ACTION_REQUIRE_PHYSICAL_ARM) {
            if (authorization_current(policy, now_ms) && policy->media_write_allowed) {
                selected = SS_ACTION_ALLOW_AND_LOG;
                policy->armed_until_ms = now_ms + SS_INACTIVITY_TIMEOUT_MS;
            } else {
                selected = SS_ACTION_BLOCK;
            }
        }
    }

    if (selected == SS_ACTION_BLOCK) {
        policy->blocked_count++;
    } else {
        policy->allowed_count++;
    }
    if (selected != SS_ACTION_ALLOW || analysis->severity >= SS_SEV_NOTICE ||
        action_is_active(selected)) {
        audit_append(policy, frame, analysis, selected, now_ms);
    }
    return selected;
}

const struct ss_audit_entry *ss_policy_audit_at(const struct ss_policy *policy,
                                                size_t age)
{
    if (policy == NULL || age >= policy->audit_count) {
        return NULL;
    }
    size_t newest = (policy->audit_head + SS_AUDIT_CAPACITY - 1u) %
                    SS_AUDIT_CAPACITY;
    size_t index = (newest + SS_AUDIT_CAPACITY - age) % SS_AUDIT_CAPACITY;
    return &policy->audit[index];
}
