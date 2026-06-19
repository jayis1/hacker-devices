/**
 * drivers/trigger.c — trigger arbiter (M7-side configuration shim)
 * Author: jayis1
 * License: GPL-2.0
 *
 * The actual real-time trigger arbitration runs on the M4 (m4_core.c).
 * This driver is the M7-side configuration shim that programs the FPGA's
 * trigger source, pattern match, and power threshold, then asks the M4
 * to arm/disarm through the D3 SRAM mailbox.
 */

#include "trigger.h"
#include "gpio.h"
#include "fpga.h"
#include "../registers.h"

#define MAILBOX ((volatile ps_m4_mailbox_t *)M4_RELEASE_REG)

static uint8_t s_current_src = 0;

void trigger_init(void)
{
    s_current_src = 0;
    /* FPGA defaults to GPIO1 trigger source. */
    fpga_cmd(FPGA_CMD_SET_TRIG_SRC, 0, 0, 0);
}

void trigger_set_source(uint8_t src)
{
    if (src > 4) src = 0;
    s_current_src = src;
    fpga_cmd(FPGA_CMD_SET_TRIG_SRC, src, 0, 0);
}

void trigger_configure(const ps_scan_desc_t *desc)
{
    s_current_src = desc->trig_src;
    fpga_cmd(FPGA_CMD_SET_TRIG_SRC, desc->trig_src, 0, 0);

    /* If pattern-match source, load the pattern + mask. */
    if (desc->trig_src == 2) {
        uint32_t pat = ((uint32_t)desc->trig_pattern_hi << 16) | desc->trig_pattern_lo;
        uint32_t msk = ((uint32_t)desc->trig_mask_hi    << 16) | desc->trig_mask_lo;
        fpga_cmd(FPGA_CMD_SET_PATTERN, pat, msk, 0);
    }

    /* If power-trigger source, program the ADC threshold into the FPGA. */
    if (desc->trig_src == 3) {
        fpga_cmd(0x0D, desc->power_threshold, 0, 0);  /* SET_POWER_THRESH */
    }
}

void trigger_arm(void)
{
    /* Encode the source in the high byte of arm_req so the M4 knows
     * which trigger GPIO to wait on. */
    MAILBOX->arm_req = (1u | ((uint32_t)s_current_src << 8));
}

void trigger_disarm(void)
{
    MAILBOX->disarm_req = 1;
}

bool trigger_fired(uint32_t timeout_us)
{
    /* The M4 increments shot_count when a shot actually fires. */
    uint32_t prev = MAILBOX->shot_count;
    for (uint32_t i = 0; i < timeout_us; i++) {
        if (MAILBOX->shot_count != prev) return true;
        /* ~1 µs delay at 480 MHz M7 */
        for (volatile int k = 0; k < 160; k++) ;
    }
    return false;
}