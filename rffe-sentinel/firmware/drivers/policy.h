/* Deterministic RFFE policy engine
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef RFFE_SENTINEL_POLICY_H
#define RFFE_SENTINEL_POLICY_H

#include "rffe.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum rs_action {
    RS_ACTION_FORWARD = 0,
    RS_ACTION_ALERT = 1,
    RS_ACTION_DENY = 2,
    RS_ACTION_SUBSTITUTE = 3,
    RS_ACTION_DELAY = 4
};

struct rs_rule {
    uint8_t priority;
    uint8_t usid_mask;
    uint8_t usid_value;
    uint8_t command_mask;
    uint16_t register_first;
    uint16_t register_last;
    uint8_t value_mask;
    uint8_t value_match;
    uint8_t substitute_mask;
    uint8_t substitute_value;
    enum rs_action action;
    uint16_t delay_ns;
    uint16_t rate_limit;
};

struct rs_policy {
    struct rs_rule rules[64];
    size_t count;
    bool default_alert;
};

struct rs_decision {
    enum rs_action action;
    int rule_index;
    uint8_t output_value;
    uint16_t delay_ns;
    bool rate_exceeded;
};

void policy_init(struct rs_policy *policy);
bool policy_add_rule(struct rs_policy *policy, const struct rs_rule *rule);
bool policy_validate_rule(const struct rs_rule *rule);
struct rs_decision policy_evaluate(const struct rs_policy *policy,
                                   const struct rffe_frame *frame,
                                   bool intervention_authorized);
const char *policy_action_name(enum rs_action action);

#endif
