/*
 * TRACE-REAPER — FPGA correlation engine driver header
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FPGA_CORR_H
#define FPGA_CORR_H

#include <stdint.h>
#include "../board.h"

void fpga_corr_init(void);
int  fpga_corr_is_ready(void);
void fpga_corr_reset(void);
void fpga_corr_configure(const session_cfg_t *cfg,
                          const uint16_t poi[KEY_BYTES_AES256],
                          const uint8_t hyp[KEY_BYTES_AES256][256]);
void fpga_corr_arm(void);
void fpga_corr_stop(void);
uint32_t fpga_corr_trace_count(void);
int  fpga_corr_done(void);
void fpga_corr_snapshot(corr_byte_t *out, uint8_t nbytes);
void fpga_corr_fetch_trace(uint32_t idx, int16_t *out, uint16_t nsamp);

#endif /* FPGA_CORR_H */