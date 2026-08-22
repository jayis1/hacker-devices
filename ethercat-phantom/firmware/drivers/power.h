/*
 * Power management simulation for EtherCAT Phantom
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef ETHERCAT_PHANTOM_POWER_H
#define ETHERCAT_PHANTOM_POWER_H

#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

typedef struct {
    uint16_t battery_mv;
    uint8_t soc_percent;
    bool usb_present;
    bool charging;
    bool low_power;
    bool critical;
} eph_power_state_t;

void eph_power_init(void);
void eph_power_tick(eph_power_state_t *state, eph_metrics_t *metrics, bool radio_enabled, bool manipulation_active);
void eph_power_print(const eph_power_state_t *state);

#endif
