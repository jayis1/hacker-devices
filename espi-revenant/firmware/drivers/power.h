/*
 * power.h - rail and safety model for eSPI Revenant
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef ESPI_REVENANT_POWER_H
#define ESPI_REVENANT_POWER_H

#include "../board.h"
#include "interposer.h"

typedef struct {
    float board_temp_c;
    float ec_current_ma;
    float target_voltage_v;
    uint8_t thermal_fault;
    uint8_t current_fault;
} er_power_t;

void er_power_init(er_power_t *pwr);
void er_power_step(er_power_t *pwr, uint32_t tick_ms, uint8_t active_profile);
uint8_t er_power_apply_status(const er_power_t *pwr, er_status_t *status, er_event_t *event_out);

#endif
