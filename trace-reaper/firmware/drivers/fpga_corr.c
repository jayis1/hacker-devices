/*
 * TRACE-REAPER — FPGA correlation engine driver
 *
 * The FPGA (iCE40UP5K reference) runs the inner CPA loop: for each incoming
 * trace it extracts the trigger-aligned window, and for each (key byte,
 * hypothesis) accumulates the five running sums (n, sum_x, sum_xx, sum_y,
 * sum_yy, sum_xy). The MCU reads these out as a corr_byte_t table snapshot
 * over SPI.
 *
 * This driver implements the SPI register protocol to the FPGA. The protocol
 * is documented below; the FPGA bitstream implements it.
 *
 * FPGA register map (16-bit addresses, 32-bit data unless noted):
 *   0x00  CTRL          { go, reset, irq_en, mode[1:0] }
 *   0x04  STATUS        { done, armed, buf_full, fault }
 *   0x08  WINDOW_BEG    sample index of window start
 *   0x0C  WINDOW_LEN    samples in window
 *   0x10  DECIMATE      0=none, N=keep 1 in N
 *   0x14  TRIG_MODE     0=ext 1=analog 2=template 3=none
 *   0x18  TRIG_LEVEL    analog threshold (12-bit signed + 2048 offset)
 *   0x1C  TARGET_TRACES number of traces to fold before auto-stop
 *   0x20  TRACE_COUNT   current count (RO)
 *   0x24  POI_BASE      base address of POI table (one entry per key byte)
 *   0x28  HYP_BASE      base address of hypothesis LUT (256 bytes per byte)
 *   0x2C  SNAP_CTRL     write 1 to snapshot accumulators into readout RAM
 *   0x30  ACC_BASE      base address of accumulator readout (5 x int64 per cell)
 *   0x34  IRQ_STATUS    write 1 to clear
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "fpga_corr.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

/* SPI1 is used for FPGA control. Software-managed NSS for simplicity. */
#define FPGA_NSS_PORT  GPIOA
#define FPGA_NSS_PIN    4

static volatile int g_fpga_ready = 0;
static volatile uint32_t g_fpga_trace_count = 0;

/* ---- Low-level SPI access ---- */
static void fpga_nss_low(void)
{
    FPGA_NSS_PORT->BSRR = (1U << (FPGA_NSS_PIN + 16));
}

static void fpga_nss_high(void)
{
    FPGA_NSS_PORT->BSRR = (1U << FPGA_NSS_PIN);
}

static void spi_wait_tx(void)
{
    while ((SPI1->SR & (1U << 1)) == 0) ; /* TXP */
}
static void spi_wait_rx(void)
{
    while ((SPI1->SR & (1U << 0)) == 0) ; /* RXP */
}

static uint16_t fpga_xfer16(uint16_t tx)
{
    spi_wait_tx();
    SPI1->TXDR = (uint32_t)tx;
    spi_wait_rx();
    return (uint16_t)SPI1->RXDR;
}

/* Write a 32-bit register: send {addr16 | (1<<15)}, then {hi16, lo16} */
static void fpga_write32(uint16_t addr, uint32_t val)
{
    fpga_nss_low();
    fpga_xfer16((uint16_t)(addr | 0x8000));
    fpga_xfer16((uint16_t)(val >> 16));
    fpga_xfer16((uint16_t)(val & 0xFFFF));
    fpga_nss_high();
}

/* Read a 32-bit register: send {addr16}, receive {hi16, lo16} */
static uint32_t fpga_read32(uint16_t addr)
{
    fpga_nss_low();
    fpga_xfer16(addr);
    uint16_t hi = fpga_xfer16(0);
    uint16_t lo = fpga_xfer16(0);
    fpga_nss_high();
    return ((uint32_t)hi << 16) | (uint32_t)lo;
}

/* Read a block of 32-bit words */
static void fpga_read_block(uint16_t addr, uint32_t *buf, uint32_t n)
{
    fpga_nss_low();
    fpga_xfer16(addr);
    for (uint32_t i = 0; i < n; i++) {
        uint16_t hi = fpga_xfer16(0);
        uint16_t lo = fpga_xfer16(0);
        buf[i] = ((uint32_t)hi << 16) | (uint32_t)lo;
    }
    fpga_nss_high();
}

/* ---- Public API ---- */

void fpga_corr_init(void)
{
    /* Configure FPGA SPI pins (already clocked by board_init via RCC).
     * NSS as output, SCK/MOSI AF5, MISO AF5 input.
     */
    gpio_mode(FPGA_NSS_PORT, FPGA_NSS_PIN, 1); /* output, software NSS */
    gpio_out(FPGA_NSS_PORT, FPGA_NSS_PIN, 1); /* idle high */
    /* Program FPGA for mode 0, 16-bit, MSB first, master, baud ~ 24 MHz */
    SPI1->CFG1 = (1U << 5) | (15U << 0); /* MBR=1, DSIZE=16-1 */
    SPI1->CFG2 = 0;
    SPI1->CR1  = (1U << 0) | (1U << 1) | (1U << 10); /* MAS, COMM, MASTER */
    SPI1->CR1 |= (1U << 0);

    /* Pulse PROG_B to reset the FPGA, wait for DONE */
    gpio_mode(GPIOB, 12, 1);
    gpio_out(GPIOB, 12, 0);
    for (volatile int i = 0; i < 1000; i++) ;
    gpio_out(GPIOB, 12, 1);
    for (volatile int i = 0; i < 100000; i++) ;
    /* DONE pin is input */
    gpio_mode(GPIOB, 11, 0);
    g_fpga_ready = gpio_in(GPIOB, 11) ? 1 : 0;

    if (g_fpga_ready) {
        fpga_corr_reset();
    }
}

int fpga_corr_is_ready(void) { return g_fpga_ready; }

void fpga_corr_reset(void)
{
    fpga_write32(0x00, (1U << 1)); /* reset pulse */
    fpga_write32(0x00, 0);
    g_fpga_trace_count = 0;
}

void fpga_corr_configure(const session_cfg_t *cfg,
                          const uint16_t poi[KEY_BYTES_AES256],
                          const uint8_t hyp[KEY_BYTES_AES256][256])
{
    uint32_t nbytes = (cfg->cipher == CIPHER_AES256) ? 32 : 16;

    fpga_corr_reset();
    fpga_write32(0x08, 0);                       /* window begin (post-trigger) */
    fpga_write32(0x0C, cfg->window_samples);
    fpga_write32(0x10, cfg->decimate ? cfg->decimate : 1);
    fpga_write32(0x14, (uint32_t)cfg->trig_src);
    fpga_write32(0x18, (uint32_t)((int32_t)cfg->trig_threshold + 2048));
    fpga_write32(0x1C, cfg->target_traces);

    /* Upload POI table: 16/32 entries of 16-bit sample indices */
    fpga_nss_low();
    fpga_xfer16(0x24 | 0x8000);   /* POI_BASE write addr */
    for (uint32_t i = 0; i < nbytes; i++) {
        fpga_xfer16(poi[i]);
    }
    fpga_nss_high();

    /* Upload hypothesis LUT: nbytes * 256 bytes. We pack two per 16-bit word. */
    fpga_nss_low();
    fpga_xfer16(0x28 | 0x8000);
    for (uint32_t i = 0; i < nbytes; i++) {
        for (uint32_t j = 0; j < 256; j += 2) {
            uint16_t w = (uint16_t)((hyp[i][j] << 8) | hyp[i][j+1]);
            fpga_xfer16(w);
        }
    }
    fpga_nss_high();
}

void fpga_corr_arm(void)
{
    /* CTRL: go=1, irq_en=1 */
    fpga_write32(0x00, (1U << 0) | (1U << 2));
}

void fpga_corr_stop(void)
{
    fpga_write32(0x00, 0);
}

uint32_t fpga_corr_trace_count(void)
{
    g_fpga_trace_count = fpga_read32(0x20);
    return g_fpga_trace_count;
}

int fpga_corr_done(void)
{
    uint32_t st = fpga_read32(0x04);
    return (st & 0x1) ? 1 : 0;
}

/* ---- Snapshot accumulator readout ----
 * Asks the FPGA to copy its running accumulators into readout RAM, then
 * reads nbytes * 256 cells, each 5 x int64 (n, sum_x, sum_xx, sum_y, sum_yy,
 * sum_xy). We pack as 5 x 2 x 32-bit words per cell (low/high of int64).
 *
 * This is heavy: 16 * 256 * 10 = 40960 32-bit words for AES-128. At 24 MHz
 * SPI/16-bit that's ~3.4 ms. Done at ~5 Hz snapshot cadence -> fine.
 */
void fpga_corr_snapshot(corr_byte_t *out, uint8_t nbytes)
{
    fpga_write32(0x2C, 1);   /* request snapshot */
    for (volatile int i = 0; i < 100; i++) ; /* settle */

    /* Read accumulators as a block: nbytes*256 cells * 5 int64 = 10 words */
    uint32_t buf[10];
    for (uint8_t i = 0; i < nbytes; i++) {
        corr_byte_t *bt = &out[i];
        for (uint16_t k = 0; k < 256; k++) {
            /* address increments automatically in the FPGA readout RAM */
            fpga_read_block(0x30, buf, 10);
            corr_accum_t *c = &bt->cells[k];
            c->n      = buf[0];
            /* int64 from two 32-bit words (little-endian) */
            c->sum_x  = (int64_t)(((uint64_t)buf[2] << 32) | buf[1]);
            c->sum_xx = (int64_t)(((uint64_t)buf[4] << 32) | buf[3]);
            c->sum_y  = (int64_t)(((uint64_t)buf[6] << 32) | buf[5]);
            c->sum_yy = (int64_t)(((uint64_t)buf[8] << 32) | buf[7]);
            c->sum_xy = (int64_t)(((uint64_t)buf[10-1] << 32) | buf[8]);
        }
    }
}

/* ---- Raw trace fetch (for live display / dump) ----
 * Pulls one captured trace window from the FPGA sample RAM at the given
 * trace index. Used for live display and SD dump.
 */
void fpga_corr_fetch_trace(uint32_t idx, int16_t *out, uint16_t nsamp)
{
    fpga_nss_low();
    fpga_xfer16(0x40 | 0x8000);   /* TRACE_RAM write addr */
    fpga_xfer16((uint16_t)(idx >> 16));
    fpga_xfer16((uint16_t)(idx & 0xFFFF));
    fpga_nss_high();

    fpga_nss_low();
    fpga_xfer16(0x42);            /* TRACE_RAM read addr */
    for (uint16_t i = 0; i < nsamp; i++) {
        uint16_t w = fpga_xfer16(0);
        out[i] = (int16_t)w;
    }
    fpga_nss_high();
}

/* ---- IRQ handler (EXTI on FPGA_IRQ pin) ----
 * The FPGA asserts IRQ when target_traces reached or buffer full.
 */
void EXTI15_10_IRQHandler(void)
{
    /* Check if the FPGA IRQ line caused it */
    if (EXTI->PR1 & (1U << 10)) {
        EXTI->PR1 = (1U << 10);
        uint32_t st = fpga_read32(0x34);
        fpga_write32(0x34, st); /* clear */
        g_fpga_trace_count = fpga_read32(0x20);
        /* The scheduler in main.c polls fpga_corr_done() so no async work here */
    }
}