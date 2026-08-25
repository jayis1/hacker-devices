/*
 * MoCA Phantom Power Interface
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef MPH_POWER_H
#define MPH_POWER_H

#include "../board.h"

void mph_power_init(void);
void mph_power_tick(mph_power_state_t *state, mph_metrics_t *metrics, bool radio_enabled, bool injection_active, bool spectrum_active);
void mph_power_print(const mph_power_state_t *state);

#endif
