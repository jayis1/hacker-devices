/*
 * secure operator radio link simulator
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NAC_MIRAGE_RADIO_H
#define NAC_MIRAGE_RADIO_H

#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

void radio_init(void);
void radio_pair(const char *operator_id);
void radio_publish_status(const nm_runtime_status_t *status, const nm_policy_t *policy);
void radio_publish_neighbor(const nm_neighbor_t *neighbor);
void radio_tick(uint32_t now_ms, nm_runtime_status_t *status);
bool radio_is_connected(void);

#endif
