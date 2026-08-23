/*
 * Dockruptor Policy Engine
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DOCKRUPTOR_POLICY_H
#define DOCKRUPTOR_POLICY_H

#include "../board.h"

void dr_policy_init(dr_context_t *ctx);
void dr_policy_load_default_profile(dr_context_t *ctx);
void dr_policy_apply(dr_context_t *ctx);
const char *dr_policy_rule_name(dr_rule_type_t type);

#endif
