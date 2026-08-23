/*
 * Dockruptor Capture Store
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DOCKRUPTOR_CAPTURE_H
#define DOCKRUPTOR_CAPTURE_H

#include "../board.h"

void dr_capture_init(dr_context_t *ctx);
void dr_capture_event(dr_context_t *ctx, dr_event_type_t type, dr_direction_t direction, const char *summary);
void dr_capture_render_recent(const dr_context_t *ctx, char *buffer, size_t buffer_len, size_t max_items);

#endif
