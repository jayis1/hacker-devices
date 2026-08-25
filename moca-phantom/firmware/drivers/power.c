/*
 * MoCA Phantom Power Simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>

#include "power.h"

void mph_power_init(void) {
    /* Intentionally empty for simulation. */
}

void mph_power_tick(mph_power_state_t *state,
                    mph_metrics_t *metrics,
                    bool radio_enabled,
                    bool injection_active,
                    bool spectrum_active) {
    uint16_t drain_mv = 2u;

    if (state == NULL || metrics == NULL) {
        return;
    }

    if (radio_enabled) {
        drain_mv = (uint16_t)(drain_mv + 1u);
    }
    if (injection_active) {
        drain_mv = (uint16_t)(drain_mv + 2u);
    }
    if (spectrum_active) {
        drain_mv = (uint16_t)(drain_mv + 1u);
    }

    if (state->battery_mv > drain_mv) {
        state->battery_mv = (uint16_t)(state->battery_mv - drain_mv);
    }

    if (state->soc_percent > 0u && (state->battery_mv % 13u) == 0u) {
        state->soc_percent -= 1u;
    }

    state->critical = state->battery_mv < 3600u || state->soc_percent < 15u;
    state->rail_3v3_mv = 3290u;
    state->rail_1v2_mv = 1195u;

    if (state->critical) {
        metrics->battery_warnings += 1u;
        metrics->bypass_events += 1u;
    }
}

void mph_power_print(const mph_power_state_t *state) {
    if (state == NULL) {
        return;
    }

    printf("[power] battery=%umV soc=%u%% usb=%s charging=%s critical=%s rails=%umV/%umV\n",
           state->battery_mv,
           state->soc_percent,
           state->usb_present ? "yes" : "no",
           state->charging ? "yes" : "no",
           state->critical ? "yes" : "no",
           state->rail_3v3_mv,
           state->rail_1v2_mv);
}
