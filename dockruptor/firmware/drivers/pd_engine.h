/*
 * Dockruptor PD Engine
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DOCKRUPTOR_PD_ENGINE_H
#define DOCKRUPTOR_PD_ENGINE_H

#include "../board.h"

void dr_pd_init(dr_context_t *ctx);
void dr_pd_attach_default_topology(dr_context_t *ctx);
void dr_pd_simulate_tick(dr_context_t *ctx);
void dr_pd_emit_contract_summary(const dr_context_t *ctx, char *buffer, size_t buffer_len);
void dr_pd_copy_capabilities(const dr_port_state_t *port, char *buffer, size_t buffer_len);
void dr_pd_force_mode(dr_context_t *ctx, dr_mode_t mode);
void dr_pd_soft_reset(dr_context_t *ctx);

#endif
