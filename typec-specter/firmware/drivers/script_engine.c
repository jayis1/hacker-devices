/*
 * Type-C Specter scenario engine
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include <string.h>
#include "script_engine.h"

static ts_scenario_state_t g_state;
static char g_abort_reason[TS_STATUS_TEXT_LEN] = "";

static void action_reset(ts_script_action_t *out) {
    memset(out, 0, sizeof(*out));
    out->kind = TS_ACTION_NONE;
}

bool script_engine_arm(ts_scenario_kind_t kind, const char *name, ts_scenario_state_t *state) {
    memset(&g_state, 0, sizeof(g_state));
    g_state.kind = kind;
    g_state.armed = true;
    g_state.active = true;
    g_state.mutation_budget_ms = 2500u;
    snprintf(g_state.name, sizeof(g_state.name), "%s", (name && name[0]) ? name : "scenario");
    if (state != NULL) {
        *state = g_state;
    }
    g_abort_reason[0] = '\0';
    return kind != TS_SCENARIO_IDLE;
}

void script_engine_abort(const char *reason) {
    g_state.active = false;
    g_state.armed = false;
    g_state.completed = true;
    snprintf(g_abort_reason, sizeof(g_abort_reason), "%s", reason ? reason : "aborted");
}

static bool advance_after(uint32_t now_ms, uint32_t threshold_ms) {
    if (now_ms >= threshold_ms) {
        g_state.step_index++;
        g_state.elapsed_ms = now_ms;
        return true;
    }
    return false;
}

static bool next_dock_identity_flip(uint32_t now_ms, ts_script_action_t *out) {
    switch (g_state.step_index) {
        case 0:
            if (advance_after(now_ms, 300u)) { out->kind = TS_ACTION_ENABLE_MUTATION; return true; }
            break;
        case 1:
            if (advance_after(now_ms, 700u)) { out->kind = TS_ACTION_IDENTITY_PROFILE; snprintf(out->text, sizeof(out->text), "corp-dock-shadow"); return true; }
            break;
        case 2:
            if (advance_after(now_ms, 1300u)) { out->kind = TS_ACTION_USB2_ISOLATE; out->value0 = 1u; return true; }
            break;
        case 3:
            if (advance_after(now_ms, 1800u)) { out->kind = TS_ACTION_USB2_ISOLATE; out->value0 = 0u; return true; }
            break;
        case 4:
            if (advance_after(now_ms, 2200u)) { out->kind = TS_ACTION_DISABLE_MUTATION; return true; }
            break;
        case 5:
            if (advance_after(now_ms, 2400u)) { out->kind = TS_ACTION_COMPLETE; return true; }
            break;
    }
    return false;
}

static bool next_late_vconn_claim(uint32_t now_ms, ts_script_action_t *out) {
    switch (g_state.step_index) {
        case 0:
            if (advance_after(now_ms, 400u)) { out->kind = TS_ACTION_ENABLE_MUTATION; return true; }
            break;
        case 1:
            if (advance_after(now_ms, 1000u)) { out->kind = TS_ACTION_SBU_ISOLATE; out->value0 = 1u; return true; }
            break;
        case 2:
            if (advance_after(now_ms, 1500u)) { out->kind = TS_ACTION_IDENTITY_PROFILE; snprintf(out->text, sizeof(out->text), "late-vconn-claim"); return true; }
            break;
        case 3:
            if (advance_after(now_ms, 2200u)) { out->kind = TS_ACTION_SBU_ISOLATE; out->value0 = 0u; return true; }
            break;
        case 4:
            if (advance_after(now_ms, 2500u)) { out->kind = TS_ACTION_COMPLETE; return true; }
            break;
    }
    return false;
}

static bool next_role_swap_race(uint32_t now_ms, ts_script_action_t *out) {
    switch (g_state.step_index) {
        case 0:
            if (advance_after(now_ms, 250u)) { out->kind = TS_ACTION_ENABLE_MUTATION; return true; }
            break;
        case 1:
            if (advance_after(now_ms, 650u)) { out->kind = TS_ACTION_REQUEST_ROLE_SWAP; out->value0 = TS_ROLE_SOURCE; return true; }
            break;
        case 2:
            if (advance_after(now_ms, 850u)) { out->kind = TS_ACTION_HARD_RESET; return true; }
            break;
        case 3:
            if (advance_after(now_ms, 1250u)) { out->kind = TS_ACTION_REQUEST_ROLE_SWAP; out->value0 = TS_ROLE_SOURCE; return true; }
            break;
        case 4:
            if (advance_after(now_ms, 1800u)) { out->kind = TS_ACTION_COMPLETE; return true; }
            break;
    }
    return false;
}

static bool next_debug_accessory_probe(uint32_t now_ms, ts_script_action_t *out) {
    switch (g_state.step_index) {
        case 0:
            if (advance_after(now_ms, 200u)) { out->kind = TS_ACTION_ASSERT_DEBUG_ACCESSORY; out->value0 = 1u; return true; }
            break;
        case 1:
            if (advance_after(now_ms, 900u)) { out->kind = TS_ACTION_ENABLE_MUTATION; return true; }
            break;
        case 2:
            if (advance_after(now_ms, 1300u)) { out->kind = TS_ACTION_ASSERT_DEBUG_ACCESSORY; out->value0 = 0u; return true; }
            break;
        case 3:
            if (advance_after(now_ms, 1700u)) { out->kind = TS_ACTION_DISABLE_MUTATION; return true; }
            break;
        case 4:
            if (advance_after(now_ms, 1900u)) { out->kind = TS_ACTION_COMPLETE; return true; }
            break;
    }
    return false;
}

static bool next_power_starve_then_recover(uint32_t now_ms, ts_script_action_t *out) {
    switch (g_state.step_index) {
        case 0:
            if (advance_after(now_ms, 300u)) { out->kind = TS_ACTION_LIMIT_CURRENT; out->value0 = 1500u; return true; }
            break;
        case 1:
            if (advance_after(now_ms, 800u)) { out->kind = TS_ACTION_ENABLE_MUTATION; return true; }
            break;
        case 2:
            if (advance_after(now_ms, 1600u)) { out->kind = TS_ACTION_LIMIT_CURRENT; out->value0 = 3000u; return true; }
            break;
        case 3:
            if (advance_after(now_ms, 1900u)) { out->kind = TS_ACTION_DISABLE_MUTATION; return true; }
            break;
        case 4:
            if (advance_after(now_ms, 2100u)) { out->kind = TS_ACTION_COMPLETE; return true; }
            break;
    }
    return false;
}

bool script_engine_next_action(ts_runtime_t *runtime, uint32_t now_ms, ts_script_action_t *out) {
    (void)runtime;
    if (out == NULL || !g_state.active) {
        return false;
    }
    action_reset(out);
    switch (g_state.kind) {
        case TS_SCENARIO_DOCK_IDENTITY_FLIP: return next_dock_identity_flip(now_ms, out);
        case TS_SCENARIO_LATE_VCONN_CLAIM: return next_late_vconn_claim(now_ms, out);
        case TS_SCENARIO_ROLE_SWAP_RACE: return next_role_swap_race(now_ms, out);
        case TS_SCENARIO_DEBUG_ACCESSORY_PROBE: return next_debug_accessory_probe(now_ms, out);
        case TS_SCENARIO_POWER_STARVE_THEN_RECOVER: return next_power_starve_then_recover(now_ms, out);
        case TS_SCENARIO_IDLE:
        default: return false;
    }
}
