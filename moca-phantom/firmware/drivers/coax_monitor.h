/*
 * MoCA Phantom Coax Spectrum Monitor Interface
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef MPH_COAX_MONITOR_H
#define MPH_COAX_MONITOR_H

#include "../board.h"

void mph_coax_monitor_init(void);
void mph_coax_monitor_tick(uint32_t epoch_ms, mph_spectrum_snapshot_t *snapshot, mph_metrics_t *metrics);
void mph_coax_monitor_print(const mph_spectrum_snapshot_t *snapshot);

#endif
