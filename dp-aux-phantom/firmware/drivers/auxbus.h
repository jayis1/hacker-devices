/*
 * DP AUX Phantom AUX bus driver
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DP_AUX_PHANTOM_AUXBUS_H
#define DP_AUX_PHANTOM_AUXBUS_H

#include "../board.h"

void auxbus_init(void);
size_t auxbus_pending_transactions(void);
void auxbus_tick(uint32_t now_ms,
                 dpa_runtime_status_t *status,
                 const dpa_policy_t *policy,
                 dpa_sink_identity_t *sink);
const dpa_aux_transaction_t *auxbus_trace_at(size_t index);
size_t auxbus_trace_count(void);
void auxbus_print_summary(const dpa_sink_identity_t *sink);

#endif
