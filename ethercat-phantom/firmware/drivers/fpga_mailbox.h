/*
 * FPGA mailbox abstraction for EtherCAT Phantom
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef ETHERCAT_PHANTOM_FPGA_MAILBOX_H
#define ETHERCAT_PHANTOM_FPGA_MAILBOX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../board.h"
#include "ethercat.h"

typedef enum {
    EPH_MB_NOP = 0,
    EPH_MB_SET_ROLE = 1,
    EPH_MB_SET_TIMING = 2,
    EPH_MB_ARM_RULESET = 3,
    EPH_MB_PUSH_FRAME = 4,
    EPH_MB_QUERY_STATUS = 5,
    EPH_MB_FORCE_BYPASS = 6
} eph_mailbox_opcode_t;

typedef struct {
    eph_mailbox_opcode_t opcode;
    uint32_t arg0;
    uint32_t arg1;
    uint32_t arg2;
} eph_mailbox_command_t;

typedef struct {
    bool ready;
    bool bypass_engaged;
    bool link_up_a;
    bool link_up_b;
    uint32_t last_cycle_ns;
    uint32_t timing_margin_ns;
} eph_fpga_status_t;

void eph_fpga_init(eph_runtime_config_t *config, eph_metrics_t *metrics);
void eph_fpga_apply_command(const eph_mailbox_command_t *cmd);
void eph_fpga_set_rules_hash(uint32_t rules_hash);
void eph_fpga_get_status(eph_fpga_status_t *status);
void eph_fpga_note_frame(const eph_frame_view_t *frame, bool modified);
uint32_t eph_fpga_estimate_latency_ns(const eph_frame_view_t *frame, bool modified);

#endif
