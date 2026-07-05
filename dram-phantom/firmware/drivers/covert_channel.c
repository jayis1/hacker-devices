/*
 * covert_channel.c — bank-conflict timing sensor configuration
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The FPGA measures the latency between consecutive ACTIVATE commands on a
 * chosen bank. If the second ACT hits a different row in the same bank, the
 * DRAM must first PRECHARGE the first row (tRP) then ACTIVATE the new one
 * (tRCD), adding ~50 ns over a row-hit. Two colluding workloads can modulate
 * this latency to transmit bits with no software-visible channel.
 */

#include "board.h"

int covert_channel_set(uint8_t bg, uint8_t bank, uint32_t *baseline_ns) {
    uint32_t st = fpga_status();
    if (!(st & FSTAT_DDR_PRESENT)) return -1;
    if (!(st & FSTAT_TARGET_ARMED)) return -2;
    if (fpga_covert_config(bg, bank) != 0) return -3;
    /* The FPGA reports a baseline timing (row-hit latency) back via status;
     * we expose it as an approximate nanosecond value for the app. */
    /* For now, derive from DDR4 constants: tRC=48 ns, tRP~14 ns, tRCD~16 ns */
    if (baseline_ns) {
        *baseline_ns = DDR4_TRC_NS; /* approximate baseline */
    }
    return 0;
}