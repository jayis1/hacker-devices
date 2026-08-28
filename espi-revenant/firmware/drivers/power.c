/*
 * power.c - rail and safety model for eSPI Revenant
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "power.h"

void er_power_init(er_power_t *pwr)
{
    memset(pwr, 0, sizeof(*pwr));
    pwr->board_temp_c = 34.0f;
    pwr->ec_current_ma = 96.0f;
    pwr->target_voltage_v = 1.80f;
}

void er_power_step(er_power_t *pwr, uint32_t tick_ms, uint8_t active_profile)
{
    float phase = (float)(tick_ms % 100u) / 100.0f;
    pwr->board_temp_c = 34.0f + (phase * 8.0f) + (active_profile ? 4.0f : 0.5f);
    pwr->ec_current_ma = 92.0f + (active_profile ? 48.0f : 22.0f) + (float)((tick_ms / 5u) % 7u);
    pwr->target_voltage_v = 1.80f - (active_profile ? 0.03f : 0.0f) + (0.01f * sinf(phase * 6.28318f));
    pwr->thermal_fault = (uint8_t)(pwr->board_temp_c >= ER_TEMP_LIMIT_C);
    pwr->current_fault = (uint8_t)(pwr->ec_current_ma >= ER_CURRENT_LIMIT_MA);
}

uint8_t er_power_apply_status(const er_power_t *pwr, er_status_t *status, er_event_t *event_out)
{
    status->board_temp_c = pwr->board_temp_c;
    status->ec_current_ma = pwr->ec_current_ma;
    status->target_voltage_v = pwr->target_voltage_v;
    if (!pwr->thermal_fault && !pwr->current_fault) {
        return 0u;
    }

    event_out->code = ER_EVENT_SAFETY;
    event_out->channel = ER_CH_GPIO;
    event_out->risk = ER_RISK_LOW;
    if (pwr->thermal_fault) {
        snprintf(event_out->message, sizeof(event_out->message), "Thermal rollback %.1fC", pwr->board_temp_c);
    } else {
        snprintf(event_out->message, sizeof(event_out->message), "Current rollback %.1fmA", pwr->ec_current_ma);
    }
    return 1u;
}
