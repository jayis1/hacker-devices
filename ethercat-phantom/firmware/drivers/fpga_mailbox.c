/*
 * FPGA mailbox abstraction for EtherCAT Phantom
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "fpga_mailbox.h"
#include "../registers.h"

#include <stdio.h>
#include <string.h>

static eph_runtime_config_t *g_config;
static eph_metrics_t *g_metrics;
static eph_fpga_status_t g_status;
static uint32_t g_rules_hash;

void eph_fpga_init(eph_runtime_config_t *config, eph_metrics_t *metrics) {
    g_config = config;
    g_metrics = metrics;
    memset(&g_status, 0, sizeof(g_status));
    g_status.ready = true;
    g_status.link_up_a = true;
    g_status.link_up_b = true;
    g_status.bypass_engaged = false;
    g_status.last_cycle_ns = 480u;
    g_status.timing_margin_ns = 220u;
    g_rules_hash = 0u;
}

void eph_fpga_apply_command(const eph_mailbox_command_t *cmd) {
    if (cmd == NULL || g_config == NULL || g_metrics == NULL) {
        return;
    }

    g_metrics->mailbox_commands += 1u;
    switch (cmd->opcode) {
        case EPH_MB_SET_ROLE:
            if (cmd->arg0 <= (uint32_t)EPH_ROLE_ACTIVE_TEST) {
                g_config->role = (eph_role_t)cmd->arg0;
            }
            break;
        case EPH_MB_SET_TIMING:
            g_config->cycle_budget_ns = cmd->arg0;
            g_status.timing_margin_ns = cmd->arg1;
            break;
        case EPH_MB_ARM_RULESET:
            g_rules_hash = cmd->arg0;
            break;
        case EPH_MB_PUSH_FRAME:
            g_status.last_cycle_ns = cmd->arg0;
            break;
        case EPH_MB_QUERY_STATUS:
            break;
        case EPH_MB_FORCE_BYPASS:
            g_status.bypass_engaged = true;
            g_config->safe_mode = true;
            break;
        case EPH_MB_NOP:
        default:
            break;
    }
}

void eph_fpga_set_rules_hash(uint32_t rules_hash) {
    eph_mailbox_command_t cmd = {EPH_MB_ARM_RULESET, rules_hash, 0u, 0u};
    eph_fpga_apply_command(&cmd);
}

void eph_fpga_get_status(eph_fpga_status_t *status) {
    if (status == NULL) {
        return;
    }
    *status = g_status;
}

uint32_t eph_fpga_estimate_latency_ns(const eph_frame_view_t *frame, bool modified) {
    uint32_t penalty = 0u;

    if (frame == NULL) {
        return 0u;
    }

    penalty += frame->has_mailbox ? 70u : 20u;
    penalty += frame->is_write ? 50u : 30u;
    penalty += modified ? 140u : 0u;
    penalty += (uint32_t)(frame->slave_address % 3u) * 10u;
    return 260u + penalty;
}

void eph_fpga_note_frame(const eph_frame_view_t *frame, bool modified) {
    uint32_t latency;

    if (frame == NULL || g_config == NULL || g_metrics == NULL) {
        return;
    }

    latency = eph_fpga_estimate_latency_ns(frame, modified);
    g_status.last_cycle_ns = latency;

    if (latency > g_config->cycle_budget_ns) {
        g_status.timing_margin_ns = 0u;
        g_status.bypass_engaged = true;
        g_config->safe_mode = true;
        g_metrics->working_counter_faults += 1u;
        printf("[fpga] timing budget exceeded (%u ns > %u ns); forcing safe bypass\n",
               latency, g_config->cycle_budget_ns);
    } else {
        g_status.timing_margin_ns = g_config->cycle_budget_ns - latency;
    }

    if (modified) {
        printf("[fpga] frame surgery scheduled for slave %u object 0x%04X:%u, ruleset=0x%08X\n",
               frame->slave_address,
               frame->object_index,
               frame->object_subindex,
               g_rules_hash);
    }
}
