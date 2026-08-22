/*
 * BLE/Wi-Fi control plane simulation
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef ETHERCAT_PHANTOM_RADIO_H
#define ETHERCAT_PHANTOM_RADIO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../board.h"
#include "ethercat.h"

void eph_radio_init(const eph_runtime_config_t *config);
void eph_radio_set_enabled(bool enabled);
bool eph_radio_is_connected(void);
void eph_radio_send_status(const eph_runtime_config_t *config, const eph_metrics_t *metrics);
void eph_radio_send_captures(const eph_capture_t *captures, size_t count);
void eph_radio_tick(uint32_t epoch_ms, eph_metrics_t *metrics);

#endif
