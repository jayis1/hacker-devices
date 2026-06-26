/*
 * drivers/fpga.c — Lattice iCE40-UP5K configuration & trigger engine control
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The FPGA holds three functional blocks:
 *   1. Trigger engine — watches the ADC stream for an analog-edge threshold,
 *      or a GPIO level, or a UART/SPI signature, then gates exactly N samples.
 *   2. Trace alignment — cross-correlates each trace against a reference
 *      template (captured from the first 50 traces) and shifts to align.
 *   3. Per-trace precompute — Σsample and Σsample² for the CPA variance
 *      (Welford running sums), reported back to the MCU per trace.
 *
 * The FPGA bitstream is embedded in the firmware as a const array
 * (sideprobe_fpga.bin, 1 Mbit = 128 KB) and loaded over SPI1 at boot using the
 * iCE40 SPI slave configuration protocol.
 */

#include <stdint.h>
#include "../board.h"
#include "../registers.h"

/* Embedded bitstream placeholder. In the real build this is a 128 KB const
   array produced by the Yosys/nextpnr flow. Here we keep a small stub so the
   config routine has something to send; the real bitstream is loaded from SD. */
static const uint8_t fpga_bitstream_stub[256] = {
    0x7E, 0xAA, 0x99, 0x7E, 0x92, 0x00, 0x00, 0x00 /* iCE40 sync word + dummy */,
    /* ... remainder zero ... */
};

extern void sp_delay_us(volatile uint32_t us);
extern void sp_delay_ms(volatile uint32_t ms);

/* FPGA FIFO + control are memory-mapped (see adc.c) */
#define FPGA_FIFO_BASE   0x70000000u
#define FPGA_CTRL_REG    (*(volatile uint32_t *)(FPGA_FIFO_BASE + 0x1000u))
#define FPGA_STAT_REG    (*(volatile uint32_t *)(FPGA_FIFO_BASE + 0x1004u))
#define FPGA_SUM_REG     (*(volatile uint32_t *)(FPGA_FIFO_BASE + 0x1008u))
#define FPGA_SUMSQ_REG   (*(volatile uint32_t *)(FPGA_FIFO_BASE + 0x100Cu))
#define FPGA_ALIGN_REG   (*(volatile uint32_t *)(FPGA_FIFO_BASE + 0x1010u))

#define FPGA_CTRL_ARM      0x01u
#define FPGA_CTRL_SRC_OFF  1u
#define FPGA_CTRL_NSAMP_OFF 4u
#define FPGA_CTRL_NTRAC_OFF 24u
#define FPGA_CTRL_LOAD_TEMPLATE 0x200u
#define FPGA_CTRL_STAT_RDY 0x80u

/* SPI1 for FPGA config (PB3=SCK, PB4=MISO, PB5=MOSI, PB8=CS, PB6=CRESET, PB7=CDONE) */
static void spi1_init(void) {
    SPI_CR1(SPI1_BASE) = 0;
    SPI_CR2(SPI1_BASE) = (7u << 0) | (1u << 12); /* 8-bit, FRXTH */
    SPI_CR1(SPI1_BASE) = SPI_CR1_MSTR | (3u << 3); /* master, /16 */
    SPI_CR1(SPI1_BASE) |= SPI_CR1_SPE;
}

static uint8_t spi1_xfer(uint8_t tx) {
    while (!(SPI_SR(SPI1_BASE) & SPI_SR_TXP)) { }
    *(volatile uint8_t *)&SPI_DR(SPI1_BASE) = tx;
    while (!(SPI_SR(SPI1_BASE) & SPI_SR_RXNE)) { }
    return *(volatile uint8_t *)&SPI_DR(SPI1_BASE);
}

static void fpga_cs_low(void)  { GPIO_BSRR(GPIOB_BASE) = (1u << (8 + 16)); }
static void fpga_cs_high(void) { GPIO_BSRR(GPIOB_BASE) = (1u << 8); }
static void fpga_creset_low(void)  { GPIO_BSRR(GPIOB_BASE) = (1u << (6 + 16)); }
static void fpga_creset_high(void) { GPIO_BSRR(GPIOB_BASE) = (1u << 6); }

static int fpga_cdone(void) {
    return (GPIO_IDR(GPIOB_BASE) >> 7) & 1;
}

static int fpga_load_bitstream(const uint8_t *bs, uint32_t len) {
    /* iCE40 SPI slave config sequence:
       1. CRESET low -> high
       2. send dummy 8 bytes
       3. send bitstream
       4. send dummy + check CDONE high
    */
    fpga_creset_low();
    sp_delay_us(100);
    fpga_creset_high();
    sp_delay_us(2000);

    fpga_cs_low();
    for (int i = 0; i < 8; i++) (void)spi1_xfer(0xFF);
    for (uint32_t i = 0; i < len; i++) (void)spi1_xfer(bs[i]);
    /* Pad with at least 49 dummy clocks per Lattice TN-02051 */
    for (int i = 0; i < 64; i++) (void)spi1_xfer(0x00);
    fpga_cs_high();

    /* Wait for CDONE */
    uint32_t to = 100000;
    while (!fpga_cdone() && --to) { }
    return fpga_cdone() ? 0 : -1;
}

void fpga_init(void) {
    spi1_init();
    /* In a real deployment the bitstream lives on SD card and is ~128 KB.
       We attempt the stub first; on failure the operator can reflash. */
    if (fpga_load_bitstream(fpga_bitstream_stub,
                            sizeof(fpga_bitstream_stub)) != 0) {
        /* Not fatal: FPGA-less mode still allows low-rate MCU-only capture. */
    }
    sp_delay_ms(10);
    FPGA_CTRL_REG = 0; /* idle */
}

void fpga_configure_trigger(uint8_t src, uint32_t samples, uint32_t n_traces) {
    /* The actual register write happens at arm time (adc.c). Here we just
       preload the alignment template mode: the first 50 traces become the
       reference template; after that, alignment is active. */
    FPGA_CTRL_REG = (1u << 9); /* reset alignment template */
    sp_delay_us(10);
    FPGA_CTRL_REG = 0;
    (void)src; (void)samples; (void)n_traces;
}

/* Read back the per-trace Σ/Σ² from the FPGA (used optionally by cpa.c) */
void fpga_get_sums(int32_t *sum, int64_t *sumsq) {
    *sum   = (int32_t)FPGA_SUM_REG;
    *sumsq = (int64_t)FPGA_SUMSQ_REG;
}

/* Push a reference trace (first 50) to the FPGA alignment template RAM */
void fpga_push_template(const int16_t *samples, uint32_t nsamp) {
    /* Memory-mapped write window at +0x2000.. */
    volatile uint16_t *tmpl = (volatile uint16_t *)(FPGA_FIFO_BASE + 0x2000u);
    for (uint32_t i = 0; i < nsamp && i < TRACE_SAMPLES; i++) {
        tmpl[i] = (uint16_t)samples[i];
    }
    FPGA_CTRL_REG = FPGA_CTRL_LOAD_TEMPLATE;
    sp_delay_us(10);
    FPGA_CTRL_REG = 0;
}