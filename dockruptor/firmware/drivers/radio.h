/*
 * Dockruptor Radio Control Plane
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DOCKRUPTOR_RADIO_H
#define DOCKRUPTOR_RADIO_H

#include "../board.h"

void dr_radio_init(dr_context_t *ctx);
void dr_radio_process_tick(dr_context_t *ctx);
void dr_radio_export_status(const dr_context_t *ctx, char *buffer, size_t buffer_len);

#endif
