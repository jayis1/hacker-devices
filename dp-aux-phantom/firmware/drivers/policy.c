/*
 * DP AUX Phantom policy engine
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "policy.h"
#include "capture.h"

#include <stdio.h>
#include <string.h>

void policy_load_defaults(dpa_policy_t profiles[DPA_MAX_PROFILES],
                          dpa_rule_t rules[DPA_MAX_RULES])
{
    memset(profiles, 0, sizeof(dpa_policy_t) * DPA_MAX_PROFILES);
    memset(rules, 0, sizeof(dpa_rule_t) * DPA_MAX_RULES);

    profiles[0] = (dpa_policy_t){
        .profile_id = DPA_PROFILE_TRANSPARENT,
        .name = "transparent",
        .pass_through = true,
        .forced_link_rate = DPA_LINK_RATE_HBR3,
        .forced_lane_count = DPA_LANE_COUNT_4
    };
    profiles[1] = (dpa_policy_t){
        .profile_id = DPA_PROFILE_EDID_MASK,
        .name = "edid-mask",
        .mask_edid_serial = true,
        .forced_link_rate = DPA_LINK_RATE_HBR2,
        .forced_lane_count = DPA_LANE_COUNT_4
    };
    profiles[2] = (dpa_policy_t){
        .profile_id = DPA_PROFILE_LT_SLOWROLL,
        .name = "lt-slowroll",
        .slow_link_training = true,
        .forced_link_rate = DPA_LINK_RATE_HBR,
        .forced_lane_count = DPA_LANE_COUNT_2
    };
    profiles[3] = (dpa_policy_t){
        .profile_id = DPA_PROFILE_DOCK_EMULATOR,
        .name = "dock-emulator",
        .emulate_dock = true,
        .forced_link_rate = DPA_LINK_RATE_HBR2,
        .forced_lane_count = DPA_LANE_COUNT_4
    };
    profiles[4] = (dpa_policy_t){
        .profile_id = DPA_PROFILE_HPD_BOUNCE,
        .name = "hpd-bounce",
        .bounce_hpd = true,
        .hpd_pulse_ms = 120U,
        .forced_link_rate = DPA_LINK_RATE_HBR2,
        .forced_lane_count = DPA_LANE_COUNT_4
    };
    profiles[5] = (dpa_policy_t){
        .profile_id = DPA_PROFILE_AUX_FUZZ,
        .name = "aux-fuzz",
        .fuzz_aux = true,
        .fuzz_seed = 0x5AU,
        .forced_link_rate = DPA_LINK_RATE_HBR,
        .forced_lane_count = DPA_LANE_COUNT_2
    };

    rules[0] = (dpa_rule_t){ .name = "serial-read", .match = "edid-serial", .action = "mask", .severity = 1U, .enabled = true };
    rules[1] = (dpa_rule_t){ .name = "link-rate-downgrade", .match = "dpcd-link-rate", .action = "force-hbr", .severity = 2U, .enabled = true };
    rules[2] = (dpa_rule_t){ .name = "mst-probe", .match = "sink-count", .action = "tag", .severity = 1U, .enabled = true };
    rules[3] = (dpa_rule_t){ .name = "dock-capability", .match = "dock-edid", .action = "emulate", .severity = 1U, .enabled = true };
    rules[4] = (dpa_rule_t){ .name = "unexpected-edid-page", .match = "edid-page-wrap", .action = "alert", .severity = 3U, .enabled = true };
    rules[5] = (dpa_rule_t){ .name = "training-loop", .match = "training-repeat", .action = "pulse-hpd", .severity = 2U, .enabled = true };
}

const dpa_policy_t *policy_find(const dpa_policy_t profiles[DPA_MAX_PROFILES], uint8_t profile_id)
{
    size_t i = 0U;
    for (i = 0U; i < DPA_MAX_PROFILES; ++i) {
        if (profiles[i].profile_id == profile_id) {
            return &profiles[i];
        }
    }
    return NULL;
}

void policy_print(const dpa_policy_t *policy)
{
    if (policy == NULL) {
        return;
    }
    printf("policy id=%u name=%s passthrough=%s edid_mask=%s slow_training=%s emulate_dock=%s hpd_bounce=%s fuzz=%s link_rate=0x%02X lanes=%u pulse=%ums seed=0x%02X\n",
           policy->profile_id,
           policy->name,
           policy->pass_through ? "yes" : "no",
           policy->mask_edid_serial ? "yes" : "no",
           policy->slow_link_training ? "yes" : "no",
           policy->emulate_dock ? "yes" : "no",
           policy->bounce_hpd ? "yes" : "no",
           policy->fuzz_aux ? "yes" : "no",
           policy->forced_link_rate,
           policy->forced_lane_count,
           policy->hpd_pulse_ms,
           policy->fuzz_seed);
}

static void maybe_mask_edid(const dpa_policy_t *policy,
                            dpa_aux_transaction_t *tx,
                            dpa_runtime_status_t *status,
                            dpa_sink_identity_t *sink)
{
    if (!policy->mask_edid_serial || tx->address != 0x00050U || tx->length < 12U) {
        return;
    }
    tx->data[8] = 'J';
    tx->data[9] = 'A';
    tx->data[10] = 'Y';
    tx->data[11] = '1';
    tx->injected = true;
    ++status->edid_overrides;
    snprintf(sink->serial, sizeof(sink->serial), "MASKED-JAY1");
    snprintf(tx->note, sizeof(tx->note), "serial masked");
    capture_log(DPA_EVENT_RULE_HIT, "masked EDID serial field", tx);
}

static void maybe_force_link_rate(const dpa_policy_t *policy,
                                  dpa_aux_transaction_t *tx,
                                  dpa_runtime_status_t *status)
{
    if (tx->address != DPA_DPCD_LINK_BW_SET || tx->length == 0U) {
        return;
    }
    tx->data[0] = policy->forced_link_rate;
    tx->injected = true;
    status->negotiated_link_rate = policy->forced_link_rate;
    snprintf(tx->note, sizeof(tx->note), "forced link rate");
    capture_log(DPA_EVENT_RULE_HIT, "forced downstream link rate", tx);
}

static void maybe_force_lane_count(const dpa_policy_t *policy,
                                   dpa_aux_transaction_t *tx,
                                   dpa_runtime_status_t *status)
{
    if (tx->address != DPA_DPCD_LANE_COUNT_SET || tx->length == 0U) {
        return;
    }
    tx->data[0] = policy->forced_lane_count;
    tx->injected = true;
    status->negotiated_lane_count = policy->forced_lane_count;
    snprintf(tx->note, sizeof(tx->note), "forced lane count");
    capture_log(DPA_EVENT_RULE_HIT, "forced downstream lane count", tx);
}

static void maybe_emulate_dock(const dpa_policy_t *policy,
                               dpa_aux_transaction_t *tx,
                               dpa_sink_identity_t *sink)
{
    if (!policy->emulate_dock || tx->address != 0x00050U || tx->length < 6U) {
        return;
    }
    tx->data[0] = 0x00U;
    tx->data[1] = 0xFFU;
    tx->data[2] = 0xFFU;
    tx->data[3] = 0xFFU;
    tx->data[4] = 0xFFU;
    tx->data[5] = 0xFFU;
    tx->injected = true;
    snprintf(sink->sink_name, sizeof(sink->sink_name), "Conference Dock Emulator");
    snprintf(sink->vendor, sizeof(sink->vendor), "JY1");
    snprintf(tx->note, sizeof(tx->note), "dock emulation header");
    capture_log(DPA_EVENT_RULE_HIT, "emulated dock-like EDID page", tx);
}

static void maybe_fuzz(const dpa_policy_t *policy,
                       dpa_aux_transaction_t *tx,
                       dpa_runtime_status_t *status)
{
    size_t i = 0U;
    if (!policy->fuzz_aux || tx->length == 0U) {
        return;
    }
    for (i = 0U; i < tx->length; ++i) {
        tx->data[i] ^= (uint8_t)(policy->fuzz_seed + (uint8_t)i);
    }
    tx->injected = true;
    ++status->alerts;
    snprintf(tx->note, sizeof(tx->note), "deterministic fuzz applied");
    capture_log(DPA_EVENT_ALERT, "applied AUX fuzz payload", tx);
}

void policy_apply_transaction(const dpa_policy_t *policy,
                              const dpa_rule_t rules[DPA_MAX_RULES],
                              dpa_aux_transaction_t *tx,
                              dpa_runtime_status_t *status,
                              dpa_sink_identity_t *sink)
{
    (void)rules;
    if (policy == NULL || tx == NULL || status == NULL || sink == NULL) {
        return;
    }

    maybe_mask_edid(policy, tx, status, sink);
    maybe_force_link_rate(policy, tx, status);
    maybe_force_lane_count(policy, tx, status);
    maybe_emulate_dock(policy, tx, sink);
    maybe_fuzz(policy, tx, status);

    if (policy->slow_link_training && tx->address == DPA_DPCD_TRAINING_PATTERN_SET) {
        ++status->link_training_steps;
        snprintf(tx->note, sizeof(tx->note), "slow-roll training pattern step %u", status->link_training_steps);
        capture_log(DPA_EVENT_RULE_HIT, "observed and delayed training pattern step", tx);
    }

    if (policy->bounce_hpd && tx->address == DPA_DPCD_LANE0_1_STATUS) {
        ++status->alerts;
        snprintf(tx->note, sizeof(tx->note), "requested HPD bounce");
        capture_log(DPA_EVENT_ALERT, "triggered HPD bounce on status read", tx);
    }
}
