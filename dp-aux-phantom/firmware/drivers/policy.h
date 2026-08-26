/*
 * DP AUX Phantom policy engine
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DP_AUX_PHANTOM_POLICY_H
#define DP_AUX_PHANTOM_POLICY_H

#include "../board.h"

void policy_load_defaults(dpa_policy_t profiles[DPA_MAX_PROFILES],
                          dpa_rule_t rules[DPA_MAX_RULES]);
const dpa_policy_t *policy_find(const dpa_policy_t profiles[DPA_MAX_PROFILES], uint8_t profile_id);
void policy_print(const dpa_policy_t *policy);
void policy_apply_transaction(const dpa_policy_t *policy,
                              const dpa_rule_t rules[DPA_MAX_RULES],
                              dpa_aux_transaction_t *tx,
                              dpa_runtime_status_t *status,
                              dpa_sink_identity_t *sink);

#endif
