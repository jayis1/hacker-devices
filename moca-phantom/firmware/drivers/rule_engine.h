/*
 * MoCA Phantom Rule Engine Interface
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef MPH_RULE_ENGINE_H
#define MPH_RULE_ENGINE_H

#include "../board.h"

void mph_rule_engine_init(void);
void mph_rule_engine_load_defaults(void);
void mph_rule_engine_apply(mph_frame_t *frame, mph_rule_result_t *result, mph_metrics_t *metrics, bool safe_mode);
const mph_rule_t *mph_rule_engine_rules(void);
size_t mph_rule_engine_rule_count(void);
uint32_t mph_rule_engine_hash(void);

#endif
