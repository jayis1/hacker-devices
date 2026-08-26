/*
 * DP AUX Phantom capture subsystem
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DP_AUX_PHANTOM_CAPTURE_H
#define DP_AUX_PHANTOM_CAPTURE_H

#include "../board.h"

void capture_init(void);
void capture_log(uint8_t type, const char *message, const dpa_aux_transaction_t *tx);
void capture_dump(void);
size_t capture_count(void);
size_t capture_type_count(uint8_t type);

#endif
