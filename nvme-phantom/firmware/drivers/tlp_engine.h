/*
 * drivers/tlp_engine.h — FPGA TLP inspector / injector interface header
 *
 * Author:  jayis1
 * License: GPL-2.0
 */

#ifndef NVME_PHANTOM_TLP_ENGINE_H
#define NVME_PHANTOM_TLP_ENGINE_H

#include "../board.h"
#include "nvme_parser.h"

int tlp_engine_init(void);
int tlp_engine_set_mode(board_mode_t mode);
int tlp_engine_load_rule(const uint8_t rule[32]);
int tlp_engine_commit_rules(void);
int tlp_engine_clear_rules(void);
int tlp_engine_read_decoded(nvme_cmd_t *cmd);
int tlp_engine_inject_sq(const uint8_t sq[64]);
int tlp_engine_inject_cpl(const uint8_t cq[16]);
int tlp_engine_dma_build(uint64_t host_phys, uint32_t len, uint8_t dir,
                         uint32_t capture_lba, uint16_t *out_cid);
int tlp_engine_spoof_ident(const uint8_t ident[4096]);
int tlp_engine_arm_modify(void);
int tlp_engine_disarm_modify(void);
int tlp_engine_get_stats(uint32_t *cap, uint32_t *inj);

#endif /* NVME_PHANTOM_TLP_ENGINE_H */