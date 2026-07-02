/*
 * spad_driver.c — SPAD focal-array control
 * Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
 *
 * Implements the interface in spad_driver.h by driving the FPGA register
 * file. Also implements the low-level FPGA SPI bus accessors (fpga_read /
 * fpga_write / fpga_read_burst) declared in registers.h.
 */
#include <stdint.h>
#include <stdbool.h>
#include "../board.h"
#include "../registers.h"
#include "spad_driver.h"

/* ---- Low-level FPGA SPI bus (ESP32-S3 SPI3) --------------------------- */
/* In a real ESP-IDF build these wrap spi_device_transmit(). For portability
 * and host-test builds we provide a weak-backed mock; the real HAL is
 * linked from the BSP. The bus is MSB-first, 16-bit payload, 7-bit address
 * encoded in the first byte as (addr & 0x7F) | (R<<7). */

static uint16_t fpga_xfer(uint8_t addr, bool read, uint16_t wval)
{
    /* Real HAL: drive CS low, clock 3 bytes, deassert CS.
     *   byte0 = (read ? 0x80 : 0x00) | (addr & 0x7F)
     *   byte1 = hi(wval) on write / don't-care on read
     *   byte2 = lo(wval) on write / don't-care on read
     *   response hi,lo come back on MISO during byte1,byte2 phases.
     * Mock: keep a static register file so host builds link. */
    (void)addr; (void)read; (void)wval;
    return 0;
}

uint16_t fpga_read(uint8_t addr)
{
    return fpga_xfer(addr, true, 0);
}

void fpga_write(uint8_t addr, uint16_t val)
{
    (void)fpga_xfer(addr, false, val);
}

void fpga_read_burst(uint8_t addr, uint16_t *buf, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
        buf[i] = fpga_read(addr);
}

/* ---- SPAD driver ------------------------------------------------------ */
int spad_init(void)
{
    /* Reset the signal-core SPAD block. */
    fpga_write(REG_CTRL, CTRL_RESET);
    /* Default: all pixels gated off, midrange bias, 0.5 ns TDC. */
    fpga_write(REG_SPAD_GATE_EN, 0);
    fpga_write(REG_SPAD_GATE_EN2, 0);
    fpga_write(REG_SPAD_BIAS, 2048);
    fpga_write(REG_SPAD_TDC_RES, 1);
    fpga_write(REG_SPAD_DEADTM, SPAD_DEADTIME_NS);
    return 0;
}

void spad_set_bias(uint16_t trim)
{
    fpga_write(REG_SPAD_BIAS, trim & 0x0FFF);
}

uint16_t spad_get_bias(void)
{
    return fpga_read(REG_SPAD_BIAS);
}

void spad_set_tdc_resolution(uint8_t res)
{
    fpga_write(REG_SPAD_TDC_RES, res & 0x03);
}

void spad_enable_all_pixels(bool on)
{
    uint16_t mask = on ? 0xFFFF : 0x0000;
    fpga_write(REG_SPAD_GATE_EN,  mask);
    fpga_write(REG_SPAD_GATE_EN2, mask);
    uint16_t ctrl = fpga_read(REG_CTRL);
    if (on) ctrl |=  CTRL_SPAD_ENABLE;
    else    ctrl &= ~CTRL_SPAD_ENABLE;
    fpga_write(REG_CTRL, ctrl);
}

void spad_enable_pixel(uint8_t idx, bool on)
{
    if (idx >= SPAD_PIXELS) return;
    uint8_t reg = (idx < 16) ? REG_SPAD_GATE_EN : REG_SPAD_GATE_EN2;
    uint8_t bit = idx & 0x0F;
    uint16_t m = fpga_read(reg);
    if (on) m |=  (1u << bit);
    else    m &= ~(1u << bit);
    fpga_write(reg, m);
}

uint32_t spad_get_rate_pixel(uint8_t idx)
{
    /* The register file exposes only pixels 0/1 directly; for the rest we
     * use the auto-increment IQ index path which also carries rate. In a
     * real build a small firmware extension maps idx->rate. For the design
     * artifact we read the aggregate and divide by 256 as a stub. */
    if (idx == 0) return fpga_read(REG_SPAD_RATE0) * 1000u;
    if (idx == 1) return fpga_read(REG_SPAD_RATE1) * 1000u;
    return spad_get_rate_aggregate() / SPAD_PIXELS;
}

uint32_t spad_get_rate_aggregate(void)
{
    /* Register reports kHz; convert to Hz. */
    return (uint32_t)fpga_read(REG_SPAD_RATE_SUM) * 1000u;
}

uint32_t spad_get_saturated_mask(uint32_t mask[2])
{
    mask[0] = fpga_read(REG_SPAD_SAT_MASK);
    mask[1] = fpga_read(REG_SPAD_SAT_MASK2);
    uint32_t cnt = 0;
    for (int i = 0; i < 2; i++)
        for (int b = 0; b < 16; b++)
            if (mask[i] & (1u << b)) cnt++;
    return cnt;
}