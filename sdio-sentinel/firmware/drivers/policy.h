/* SDIO Sentinel policy engine. Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SDIO_SENTINEL_POLICY_H
#define SDIO_SENTINEL_POLICY_H

#include "frame.h"

enum ss_policy_action {
    SS_ACTION_ALLOW = 0,
    SS_ACTION_ALLOW_AND_LOG,
    SS_ACTION_BLOCK,
    SS_ACTION_REQUIRE_PHYSICAL_ARM
};

struct ss_policy_rule {
    uint8_t command;
    uint8_t command_mask;
    uint32_t argument_value;
    uint32_t argument_mask;
    enum ss_direction direction;
    enum ss_policy_action action;
    bool enabled;
};

struct ss_audit_entry {
    uint32_t timestamp_ms;
    uint32_t flags;
    uint32_t argument;
    uint16_t sequence;
    uint8_t command;
    uint8_t action;
    uint8_t severity;
};

struct ss_policy {
    struct ss_policy_rule rules[SS_POLICY_RULES];
    struct ss_audit_entry audit[SS_AUDIT_CAPACITY];
    size_t rule_count;
    size_t audit_head;
    size_t audit_count;
    uint32_t arm_started_ms;
    uint32_t armed_until_ms;
    uint32_t session_started_ms;
    uint32_t blocked_count;
    uint32_t allowed_count;
    enum ss_mode mode;
    bool arm_switch_seen;
    bool operator_confirmed;
    bool media_write_allowed;
};

void ss_policy_init(struct ss_policy *policy, uint32_t now_ms);
bool ss_policy_add_rule(struct ss_policy *policy, const struct ss_policy_rule *rule);
void ss_policy_clear_custom_rules(struct ss_policy *policy);
void ss_policy_note_arm_switch(struct ss_policy *policy, bool pressed, uint32_t now_ms);
bool ss_policy_confirm_operator(struct ss_policy *policy, const char *statement,
                                uint32_t now_ms);
void ss_policy_tick(struct ss_policy *policy, uint32_t now_ms);
enum ss_policy_action ss_policy_evaluate(struct ss_policy *policy,
                                         const struct ss_frame *frame,
                                         const struct ss_analysis *analysis,
                                         uint32_t now_ms);
const struct ss_audit_entry *ss_policy_audit_at(const struct ss_policy *policy,
                                                size_t age);
bool ss_policy_set_mode(struct ss_policy *policy, enum ss_mode requested,
                        uint32_t now_ms);

#endif
