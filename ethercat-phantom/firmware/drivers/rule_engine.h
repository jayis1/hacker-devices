/*
 * Rule engine for process-data manipulations
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef ETHERCAT_PHANTOM_RULE_ENGINE_H
#define ETHERCAT_PHANTOM_RULE_ENGINE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../board.h"
#include "ethercat.h"

typedef struct {
    bool matched;
    bool modified;
    int32_t value_after;
    const eph_rule_t *rule;
} eph_rule_result_t;

void eph_rule_engine_init(void);
void eph_rule_engine_load_defaults(void);
size_t eph_rule_engine_rule_count(void);
const eph_rule_t *eph_rule_engine_rules(void);
uint32_t eph_rule_engine_hash(void);
bool eph_rule_engine_add(const eph_rule_t *rule);
void eph_rule_engine_clear(void);
void eph_rule_engine_apply(const eph_frame_view_t *frame, eph_rule_result_t *result, eph_metrics_t *metrics, bool safe_mode);

#endif
