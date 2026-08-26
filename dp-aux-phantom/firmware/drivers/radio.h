/*
 * DP AUX Phantom radio backhaul
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DP_AUX_PHANTOM_RADIO_H
#define DP_AUX_PHANTOM_RADIO_H

#include "../board.h"

void radio_init(void);
void radio_pair(const char *peer_name);
void radio_tick(uint32_t now_ms, dpa_runtime_status_t *status);
void radio_publish_status(const dpa_runtime_status_t *status,
                          const dpa_policy_t *policy,
                          const dpa_sink_identity_t *sink);

#endif
