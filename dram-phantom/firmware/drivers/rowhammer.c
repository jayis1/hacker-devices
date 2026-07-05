/*
 * rowhammer.c — Rowhammer injection orchestration
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The MCU tells the FPGA which aggressor row to hammer, how many times, and
 * which pattern (single-sided, double-sided, many-sided). After hammering, the
 * FPGA reads back the adjacent victim rows and reports any flipped bits.
 *
 * SAFETY: rowhammer_arm() is a no-op unless the device is in MODE_ROWHAMMER
 * AND armed (two-key sequence completed). The FPGA enforces this too.
 */

#include "board.h"

int rowhammer_arm(uint32_t aggressor, uint32_t count, uint8_t pat) {
    uint32_t st = fpga_status();
    if (!(st & FSTAT_DDR_PRESENT)) return -1;
    if (!(st & FSTAT_TARGET_ARMED)) return -2;
    if (count == 0 || count > 10000000UL) return -3;
    /* Pattern IDs:
     *   0 = single-sided (hammer row R, check R+1)
     *   1 = double-sided (hammer R-1 and R+1, check R)
     *   2 = many-sided (hammer R-2,R-1,R+1,R+2, check R)
     *   3 = TRR-defeating (interleave with REF to confuse single-row TRR)
     */
    if (pat > 3) return -4;
    return fpga_hammer_pattern(aggressor, count, pat);
}

int rowhammer_run(uint32_t *victim_row, uint32_t *flip_mask) {
    uint32_t st = fpga_status();
    if (!(st & FSTAT_TARGET_ARMED)) return -1;
    /* The FPGA starts hammering as soon as the pattern is loaded; we poll
     * for the FLIP_DETECTED bit or a timeout. */
    uint32_t t0 = millis();
    while (1) {
        st = fpga_status();
        if (st & FSTAT_FLIP_DETECTED) break;
        if ((st & FSTAT_TARGET_ARMED) == 0) {
            /* FPGA disarmed itself (e.g., pattern complete with no flips) */
            if (victim_row) *victim_row = 0xFFFFFFFF;
            if (flip_mask)  *flip_mask  = 0;
            return 0;
        }
        if ((millis() - t0) > 30000) return -2; /* 30 s timeout */
    }
    return fpga_drain_flip(victim_row, flip_mask);
}