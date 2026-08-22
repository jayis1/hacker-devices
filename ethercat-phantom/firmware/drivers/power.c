/*
 * Power management simulation for EtherCAT Phantom
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "power.h"

#include <stdio.h>

static uint16_t g_tick_count;

void eph_power_init(void) {
    g_tick_count = 0u;
}

void eph_power_tick(eph_power_state_t *state, eph_metrics_t *metrics, bool radio_enabled, bool manipulation_active) {
    uint16_t drain_mv = 3u;

    if (state == NULL || metrics == NULL) {
        return;
    }

    g_tick_count += 1u;
    if (radio_enabled) {
        drain_mv += 1u;
    }
    if (manipulation_active) {
        drain_mv += 2u;
    }
    if ((g_tick_count % 5u) == 0u && state->battery_mv > drain_mv) {
        state->battery_mv = (uint16_t)(state->battery_mv - drain_mv);
    }

    state->low_power = state->battery_mv <= EPH_POWER_LOW_MV;
    state->critical = state->battery_mv <= EPH_POWER_CRITICAL_MV;
    if (state->critical) {
        metrics->brownouts += 1u;
    }

    if (state->battery_mv >= 4200u) {
        state->soc_percent = 100u;
    } else if (state->battery_mv <= 3300u) {
        state->soc_percent = 5u;
    } else {
        state->soc_percent = (uint8_t)(((state->battery_mv - 3300u) * 95u) / 900u + 5u);
    }
}

void eph_power_print(const eph_power_state_t *state) {
    if (state == NULL) {
        return;
    }
    printf("[power] battery=%umV soc=%u%% usb=%s charging=%s low=%s critical=%s\n",
           state->battery_mv,
           state->soc_percent,
           state->usb_present ? "yes" : "no",
           state->charging ? "yes" : "no",
           state->low_power ? "yes" : "no",
           state->critical ? "yes" : "no");
}
