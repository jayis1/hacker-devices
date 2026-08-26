/*
 * DP AUX Phantom USB-C / PD manager
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DP_AUX_PHANTOM_PD_H
#define DP_AUX_PHANTOM_PD_H

#include "../board.h"

void pd_init(void);
void pd_attach_sink(dpa_sink_identity_t *sink);
void pd_apply_policy(const dpa_policy_t *policy, dpa_runtime_status_t *status);
void pd_tick(uint32_t now_ms,
             dpa_runtime_status_t *status,
             const dpa_policy_t *policy,
             dpa_sink_identity_t *sink);

#endif
