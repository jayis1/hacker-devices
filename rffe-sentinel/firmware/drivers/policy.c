/* RFFE Sentinel policy engine
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "policy.h"
#include "board.h"

#include <string.h>

static bool rule_matches(const struct rs_rule *rule,
                         const struct rffe_frame *frame)
{
    uint8_t command_bit;

    if (((frame->usid ^ rule->usid_value) & rule->usid_mask) != 0u) {
        return false;
    }
    command_bit = (uint8_t)(1u << (uint8_t)frame->command_class);
    if ((rule->command_mask & command_bit) == 0u) {
        return false;
    }
    if ((frame->register_address < rule->register_first) ||
        (frame->register_address > rule->register_last)) {
        return false;
    }
    if ((frame->data_length > 0u) &&
        (((frame->data[0] ^ rule->value_match) & rule->value_mask) != 0u)) {
        return false;
    }
    return true;
}

void policy_init(struct rs_policy *policy)
{
    if (policy == NULL) {
        return;
    }
    memset(policy, 0, sizeof(*policy));
    policy->default_alert = false;
}

bool policy_validate_rule(const struct rs_rule *rule)
{
    const uint8_t read_commands =
        (uint8_t)((1u << RFFE_CMD_REG_READ) |
                  (1u << RFFE_CMD_EXT_READ));

    if (rule == NULL) {
        return false;
    }
    if (rule->register_first > rule->register_last) {
        return false;
    }
    if (rule->command_mask == 0u) {
        return false;
    }
    if (rule->action > RS_ACTION_DELAY) {
        return false;
    }
    if ((rule->action == RS_ACTION_DELAY) &&
        (rule->delay_ns > RS_MAX_DELAY_NS)) {
        return false;
    }
    if ((rule->action == RS_ACTION_SUBSTITUTE) &&
        (rule->substitute_mask == 0u)) {
        return false;
    }
    if (((rule->action == RS_ACTION_DENY) ||
         (rule->action == RS_ACTION_SUBSTITUTE) ||
         (rule->action == RS_ACTION_DELAY)) &&
        ((rule->command_mask & read_commands) != 0u)) {
        return false;
    }
    if (((rule->action == RS_ACTION_DENY) ||
         (rule->action == RS_ACTION_SUBSTITUTE) ||
         (rule->action == RS_ACTION_DELAY)) &&
        (rule->usid_mask == 0u)) {
        return false;
    }
    return true;
}

bool policy_add_rule(struct rs_policy *policy, const struct rs_rule *rule)
{
    size_t position;
    size_t index;

    if ((policy == NULL) || !policy_validate_rule(rule) ||
        (policy->count >= RS_POLICY_CAPACITY)) {
        return false;
    }

    position = policy->count;
    for (index = 0u; index < policy->count; ++index) {
        if (rule->priority < policy->rules[index].priority) {
            position = index;
            break;
        }
    }
    for (index = policy->count; index > position; --index) {
        policy->rules[index] = policy->rules[index - 1u];
    }
    policy->rules[position] = *rule;
    ++policy->count;
    return true;
}

struct rs_decision policy_evaluate(const struct rs_policy *policy,
                                   const struct rffe_frame *frame,
                                   bool intervention_authorized)
{
    struct rs_decision decision;
    size_t index;

    memset(&decision, 0, sizeof(decision));
    decision.action = RS_ACTION_FORWARD;
    decision.rule_index = -1;
    if ((policy == NULL) || (frame == NULL)) {
        return decision;
    }
    if (frame->data_length > 0u) {
        decision.output_value = frame->data[0];
    }

    for (index = 0u; index < policy->count; ++index) {
        const struct rs_rule *rule = &policy->rules[index];
        if (!rule_matches(rule, frame)) {
            continue;
        }
        decision.rule_index = (int)index;
        decision.action = rule->action;
        decision.delay_ns = rule->delay_ns;
        if (rule->action == RS_ACTION_SUBSTITUTE) {
            decision.output_value =
                (uint8_t)((decision.output_value &
                           (uint8_t)~rule->substitute_mask) |
                          (rule->substitute_value & rule->substitute_mask));
        }
        if (!intervention_authorized &&
            (decision.action != RS_ACTION_ALERT) &&
            (decision.action != RS_ACTION_FORWARD)) {
            decision.action = RS_ACTION_ALERT;
            decision.delay_ns = 0u;
            decision.output_value = frame->data[0];
        }
        if (((frame->command_class == RFFE_CMD_REG_READ) ||
             (frame->command_class == RFFE_CMD_EXT_READ)) &&
            (decision.action == RS_ACTION_DELAY)) {
            decision.action = RS_ACTION_ALERT;
            decision.delay_ns = 0u;
        }
        return decision;
    }

    if (policy->default_alert) {
        decision.action = RS_ACTION_ALERT;
    }
    return decision;
}

const char *policy_action_name(enum rs_action action)
{
    switch (action) {
    case RS_ACTION_FORWARD:
        return "forward";
    case RS_ACTION_ALERT:
        return "alert";
    case RS_ACTION_DENY:
        return "deny";
    case RS_ACTION_SUBSTITUTE:
        return "substitute";
    case RS_ACTION_DELAY:
        return "delay";
    default:
        return "invalid";
    }
}
