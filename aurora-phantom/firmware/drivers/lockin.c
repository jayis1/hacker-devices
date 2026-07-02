/*
 * lockin.c — digital lock-in demodulator control
 * Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
 *
 * The lock-in demodulator itself runs in the FPGA. This driver programs its
 * registers: LO frequency (0.01 Hz resolution), phase, integration window,
 * and frame period, and enables/disables the block.
 *
 * LO frequency encoding: REG_LO_FREQ_HI/LO form a 32-bit value in units of
 * 0.01 Hz, giving 0..429 kHz range with 0.01 Hz resolution — more than
 * enough for refresh-rate (60-240 Hz), backlight PWM (200 Hz-25 kHz), and
 * LED data channels (up to a few kHz).
 */
#include <stdint.h>
#include "../board.h"
#include "../registers.h"
#include "lockin.h"

static uint32_t g_lo_hz = 60;

static void pack_freq(uint32_t hz)
{
    /* value = hz * 100 (units of 0.01 Hz) */
    uint32_t v = hz * 100u;
    fpga_write(REG_LO_FREQ_HI, (uint16_t)(v >> 16));
    fpga_write(REG_LO_FREQ_LO, (uint16_t)(v & 0xFFFF));
}

static uint32_t unpack_freq(void)
{
    uint32_t v = ((uint32_t)fpga_read(REG_LO_FREQ_HI) << 16)
               |  fpga_read(REG_LO_FREQ_LO);
    return v / 100u;
}

int lockin_init(void)
{
    g_lo_hz = 60;
    pack_freq(g_lo_hz);
    fpga_write(REG_LO_PHASE, 0);
    fpga_write(REG_INT_WIN_HI, 0);
    fpga_write(REG_INT_WIN_LO, 0);
    /* Disable until configured. */
    uint16_t ctrl = fpga_read(REG_CTRL);
    ctrl &= ~CTRL_LOCKIN_ENABLE;
    fpga_write(REG_CTRL, ctrl);
    return 0;
}

void lockin_set_lo_freq(uint32_t hz)
{
    if (hz == 0) hz = 60;
    g_lo_hz = hz;
    pack_freq(hz);
    /* Reset integrator when LO changes to avoid stale I/Q. */
    fpga_write(REG_INT_RESET, 1);
}

uint32_t lockin_get_lo_freq(void)
{
    g_lo_hz = unpack_freq();
    return g_lo_hz;
}

void lockin_set_integration_window(uint32_t us)
{
    /* 1 tick = 10 ns -> ticks = us * 100 */
    uint32_t t = us * 100u;
    fpga_write(REG_INT_WIN_HI, (uint16_t)(t >> 16));
    fpga_write(REG_INT_WIN_LO, (uint16_t)(t & 0xFFFF));
}

void lockin_set_frame_period(uint32_t us)
{
    uint32_t t = us * 100u;
    fpga_write(REG_FRAME_PERIOD, (uint16_t)t);
}

void lockin_enable(bool on)
{
    uint16_t ctrl = fpga_read(REG_CTRL);
    if (on) ctrl |=  CTRL_LOCKIN_ENABLE | CTRL_STREAM_IQ;
    else    ctrl &= ~(CTRL_LOCKIN_ENABLE | CTRL_STREAM_IQ);
    fpga_write(REG_CTRL, ctrl);
}

bool lockin_is_running(void)
{
    return (fpga_read(REG_CTRL) & CTRL_LOCKIN_ENABLE) != 0;
}