/*
 * policy_engine.h
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef MCTP_WRAITH_POLICY_ENGINE_H
#define MCTP_WRAITH_POLICY_ENGINE_H

#include <stdbool.h>
#include "../board.h"

void policy_engine_init(void);
void policy_engine_load_defaults(void);
void policy_engine_arm_scenario(mw_scenario_kind_t kind);
void policy_engine_tick(void);
bool policy_engine_apply(mw_message_t *message);
void policy_engine_dump(void);

#endif
