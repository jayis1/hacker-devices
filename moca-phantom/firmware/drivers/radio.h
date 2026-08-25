/*
 * MoCA Phantom Radio Interface
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef MPH_RADIO_H
#define MPH_RADIO_H

#include "../board.h"

void mph_radio_init(const mph_runtime_config_t *config);
void mph_radio_tick(uint32_t epoch_ms, mph_metrics_t *metrics);
void mph_radio_send_status(const mph_runtime_config_t *config, const mph_metrics_t *metrics, const mph_spectrum_snapshot_t *spectrum);
void mph_radio_send_captures(const mph_frame_t *frames, size_t count, mph_metrics_t *metrics);

#endif
