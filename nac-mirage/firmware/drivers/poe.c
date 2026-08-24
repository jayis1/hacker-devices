/*
 * nac-mirage PoE engine simulator
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "poe.h"

#include <stdio.h>
#include <string.h>

static nm_poe_state_t g_state;
static nm_policy_t g_policy_cache;

void poe_init(void)
{
    memset(&g_state, 0, sizeof(g_state));
    memset(&g_policy_cache, 0, sizeof(g_policy_cache));
    g_state.detected_class = NM_POE_CLASS_3;
    g_state.allocated_mw = 13000U;
    g_state.instantaneous_mw = 4200U;
}

void poe_configure(const nm_policy_t *policy)
{
    if (policy == NULL) {
        return;
    }

    g_policy_cache = *policy;
    if (policy->poe_budget_mw != 0U) {
        g_state.allocated_mw = policy->poe_budget_mw;
    }
    g_state.glitch_armed = policy->enable_poe_glitch;
    g_state.brownout_active = false;
    g_state.glitch_at_ms = 0U;
}

void poe_force_class(uint8_t class_id, uint16_t budget_mw)
{
    g_state.detected_class = class_id;
    g_state.allocated_mw = budget_mw;
}

void poe_observe_draw(uint16_t milliwatts, uint32_t now_ms)
{
    g_state.instantaneous_mw = milliwatts;
    if (g_state.glitch_armed && milliwatts > (g_state.allocated_mw - 1000U) && g_state.glitch_at_ms == 0U) {
        g_state.glitch_at_ms = now_ms + 500U;
    }
}

void poe_tick(uint32_t now_ms, nm_runtime_status_t *status)
{
    if (status == NULL) {
        return;
    }

    if (g_policy_cache.enable_poe_glitch && g_state.glitch_at_ms != 0U && now_ms >= g_state.glitch_at_ms) {
        g_state.brownout_active = true;
        if (g_state.allocated_mw > 4000U) {
            g_state.allocated_mw -= 4000U;
        }
        g_state.glitch_armed = false;
    }

    if (g_state.brownout_active && (now_ms % 2000U) == 0U) {
        g_state.brownout_active = false;
        if (g_policy_cache.poe_budget_mw != 0U) {
            g_state.allocated_mw = g_policy_cache.poe_budget_mw;
        }
    }

    status->poe_budget_mw = g_state.allocated_mw;
    if (g_state.brownout_active) {
        status->alerts++;
    }
}

const nm_poe_state_t *poe_state(void)
{
    return &g_state;
}
