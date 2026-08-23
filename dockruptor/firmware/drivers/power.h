/*
 * Dockruptor Power and Safety
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DOCKRUPTOR_POWER_H
#define DOCKRUPTOR_POWER_H

#include "../board.h"

void dr_power_init(dr_context_t *ctx);
void dr_power_tick(dr_context_t *ctx);
void dr_power_assert_bypass(dr_context_t *ctx, const char *reason);

#endif
