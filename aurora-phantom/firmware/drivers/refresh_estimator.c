/*
 * refresh_estimator.c — refresh-rate / backlight-PWM estimator
 * Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
 *
 * The FPGA computes an FFT of the aggregate photon-rate time series and
 * reports the peak bin. This driver programs the FFT size, kicks off a
 * collection window, polls for completion, and converts the peak bin index
 * to a frequency in Hz, then seeds the PLL tracker.
 *
 * FFT bin -> Hz:  f_peak = peak_bin * (sample_rate / N)
 *   sample_rate = the aggregate-rate stream decimated to 10 kHz
 *   N = 2^log2
 */
#include <stdint.h>
#include <stdbool.h>
#include "../board.h"
#include "../registers.h"
#include "refresh_estimator.h"

#define AGG_RATE_SR_HZ   10000u   /* 10 kHz decimated aggregate-rate stream */

static uint32_t g_peak_hz = 0;
static uint16_t g_peak_mag = 0;
static uint8_t  g_log2 = REFRESH_EST_DEF_LOG;

int refresh_estimator_init(void)
{
    g_log2 = REFRESH_EST_DEF_LOG;
    fpga_write(REG_FFT_SIZE_LOG, g_log2);
    fpga_write(REG_PLL_K_P, 0x0100);  /* Q8.8 = 1.0 */
    fpga_write(REG_PLL_K_I, 0x0040);  /* Q8.8 = 0.25 */
    fpga_write(REG_PLL_LOCK_THRESH, 180); /* 18.0 dB */
    uint16_t ctrl = fpga_read(REG_CTRL);
    ctrl |= CTRL_REFRESH_ENABLE;
    fpga_write(REG_CTRL, ctrl);
    return 0;
}

void refresh_estimator_set_fft_log(uint8_t log2)
{
    if (log2 < 8)  log2 = 8;
    if (log2 > 12) log2 = 12;
    g_log2 = log2;
    fpga_write(REG_FFT_SIZE_LOG, g_log2);
}

static uint32_t bin_to_hz(uint32_t bin)
{
    uint32_t n = 1u << g_log2;
    return bin * AGG_RATE_SR_HZ / n;
}

uint32_t refresh_estimator_run(uint32_t duration_ms)
{
    /* Arm a fresh collection. The FFT block latches the peak when the
     * window fills; we poll the status bit. */
    uint16_t ctrl = fpga_read(REG_CTRL);
    ctrl &= ~CTRL_REFRESH_ENABLE;
    fpga_write(REG_CTRL, ctrl);
    ctrl |= CTRL_REFRESH_ENABLE;
    fpga_write(REG_CTRL, ctrl);

    /* Wait for STAT_LOCKED or timeout. */
    uint32_t deadline = duration_ms;
    uint32_t waited = 0;
    while (waited < deadline) {
        uint16_t st = fpga_read(REG_STATUS);
        if (st & STAT_LOCKED) break;
        /* platform sleep 5 ms */
        for (volatile int i = 0; i < 20000; i++) {}
        waited += 5;
    }

    uint32_t bin = ((uint32_t)fpga_read(REG_FFT_BIN_HI) << 16)
                 |  fpga_read(REG_FFT_BIN_LO);
    g_peak_mag = fpga_read(REG_FFT_PEAK_MAG);
    g_peak_hz = bin_to_hz(bin);

    /* Seed the PLL with the peak so the lock-in LO can track it. */
    uint32_t v = g_peak_hz * 100u;
    fpga_write(REG_PLL_FREQ_HI, (uint16_t)(v >> 16));
    fpga_write(REG_PLL_FREQ_LO, (uint16_t)(v & 0xFFFF));
    return g_peak_hz;
}

uint32_t refresh_estimator_get_freq(void)
{
    /* Prefer the PLL-tracked value if locked. */
    if (refresh_estimator_is_locked()) {
        uint32_t v = ((uint32_t)fpga_read(REG_PLL_FREQ_HI) << 16)
                   |  fpga_read(REG_PLL_FREQ_LO);
        return v / 100u;
    }
    return g_peak_hz;
}

uint16_t refresh_estimator_get_peak_mag(void) { return g_peak_mag; }

bool refresh_estimator_is_locked(void)
{
    return (fpga_read(REG_STATUS) & STAT_LOCKED) != 0;
}