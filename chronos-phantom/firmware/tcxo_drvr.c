/*
 * tcxo_drvr.c — VC-TCXO frequency control and 1-PPS disciplining
 *
 * Drives DAC1 channel 1 to tune the MV89 VC-TCXO. Supports controlled
 * drift injection (±500 ppb) for the SKEW_STEALTH profile, and a software
 * PLL for GNSS 1-PPS disciplining.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "tcxo_drvr.h"
#include "registers.h"
#include "board.h"

/* PI controller gains */
#define TCXO_PI_KP   2
#define TCXO_PI_KI   1
#define TCXO_PI_ILIMIT 1000  /* integrator limit in ppb */

void tcxo_init(tcxo_state_t *st)
{
    st->dac_value = TCXO_DAC_CENTER;
    st->drift_ppb = 0;
    st->target_ppb = 0;
    st->pps_count = 0;
    st->pps_offset_ns = 0;
    st->disciplined = 0;
    st->integ = 0;
    st->prop = 0;
    tcxo_apply(st);
}

void tcxo_set_drift(tcxo_state_t *st, int32_t target_ppb)
{
    if (target_ppb > TCXO_DRIFT_MAX_PPB)
        target_ppb = TCXO_DRIFT_MAX_PPB;
    if (target_ppb < -TCXO_DRIFT_MAX_PPB)
        target_ppb = -TCXO_DRIFT_MAX_PPB;
    st->target_ppb = target_ppb;
    /* Immediate transition — in real HW this would be gradual to avoid
     * phase discontinuity; for attack purposes, immediate is fine.
     */
    st->drift_ppb = target_ppb;
    /* Convert ppb to DAC offset: ±500 ppb ≈ ±200 DAC counts (approx) */
    int32_t dac_offset = (target_ppb * 200) / TCXO_DRIFT_MAX_PPB;
    st->dac_value = (uint16_t)(TCXO_DAC_CENTER + dac_offset);
    tcxo_apply(st);
}

void tcxo_apply(tcxo_state_t *st)
{
    /* Write DAC1 channel 1 (12-bit right-aligned) */
    DAC1->DHR12R1 = st->dac_value & 0xFFF;
    /* Trigger: software trigger if not using DMA */
    DAC1->SWTRIGR = 1;
}

void tcxo_pps_handler(tcxo_state_t *st, int64_t offset_ns)
{
    st->pps_count++;
    st->pps_offset_ns = offset_ns;
    if (st->disciplined) {
        /* PI controller: correct TCXO to align with PPS */
        int32_t err_ns = (int32_t)offset_ns;
        st->prop = TCXO_PI_KP * err_ns;
        st->integ += TCXO_PI_KI * err_ns;
        if (st->integ > TCXO_PI_ILIMIT) st->integ = TCXO_PI_ILIMIT;
        if (st->integ < -TCXO_PI_ILIMIT) st->integ = -TCXO_PI_ILIMIT;
        int32_t correction = (st->prop + st->integ) / 100;
        int32_t new_drift = st->target_ppb + correction;
        tcxo_set_drift(st, new_drift);
    }
}

void tcxo_discipline_tick(tcxo_state_t *st, uint32_t now_ms)
{
    /* Called periodically — in real HW this would check PPS watchdog
     * and flag loss-of-lock. Here it's a stub for completeness.
     */
    (void)st;
    (void)now_ms;
}

int32_t tcxo_get_drift(const tcxo_state_t *st)
{
    return st->drift_ppb;
}