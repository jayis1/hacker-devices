/*
 * inline bridge and packet policy engine
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NAC_MIRAGE_BRIDGE_H
#define NAC_MIRAGE_BRIDGE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../board.h"
#include "lldp.h"

void bridge_init(void);
void bridge_apply_policy(const nm_policy_t *policy);
void bridge_tick(uint32_t now_ms, nm_runtime_status_t *status, nm_policy_t *policy, nm_neighbor_t *neighbor);
size_t bridge_pending_frames(void);

#endif
