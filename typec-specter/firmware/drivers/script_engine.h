/*
 * Type-C Specter scenario engine
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef TYPEC_SPECTER_SCRIPT_ENGINE_H
#define TYPEC_SPECTER_SCRIPT_ENGINE_H

#include <stdbool.h>
#include <stdint.h>
#include "../board.h"

typedef enum {
    TS_ACTION_NONE = 0,
    TS_ACTION_ENABLE_MUTATION,
    TS_ACTION_DISABLE_MUTATION,
    TS_ACTION_IDENTITY_PROFILE,
    TS_ACTION_REQUEST_ROLE_SWAP,
    TS_ACTION_ASSERT_DEBUG_ACCESSORY,
    TS_ACTION_LIMIT_CURRENT,
    TS_ACTION_USB2_ISOLATE,
    TS_ACTION_SBU_ISOLATE,
    TS_ACTION_HARD_RESET,
    TS_ACTION_COMPLETE
} ts_script_action_kind_t;

typedef struct {
    ts_script_action_kind_t kind;
    uint32_t value0;
    uint32_t value1;
    char text[TS_PROFILE_NAME_LEN];
} ts_script_action_t;

bool script_engine_arm(ts_scenario_kind_t kind, const char *name, ts_scenario_state_t *state);
void script_engine_abort(const char *reason);
bool script_engine_next_action(ts_runtime_t *runtime, uint32_t now_ms, ts_script_action_t *out);

#endif
