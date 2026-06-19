/**
 * drivers/fpga.c — iCE40UP5K SPI configuration & command interface
 * Author: jayis1
 * License: GPL-2.0
 *
 * At boot the STM32 configures the iCE40 from an embedded bitstream
 * (firmware/fpga/photonstrike.bit) over SPI1. After configuration, the
 * SPI bus is reused as a low-rate command channel: each access is a
 * single 17-byte transaction (1 opcode + 4×4 byte arguments). The FPGA
 * firmware implements a small command decoder that latches the delay,
 * pulse width, energy, trigger source, pattern, and MEMS goto values
 * into the appropriate hardware blocks.
 *
 * The bitstream is generated from the open-source yosys + nextpnr-ice40
 * flow (see firmware/fpga/). This driver only loads it and issues
 * commands; the Verilog source is out of scope for this C file.
 */

#include "fpga.h"
#include "gpio.h"
#include "../registers.h"

/* Embedded bitstream — declared in fpga_bitstream.c (generated). */
extern const uint8_t  ps_bitstream[];
extern const uint32_t ps_bitstream_len;

#define SPI1   ((volatile uint32_t *)SPI1_BASE)

static bool s_ready = false;

/* ─── SPI1 low-level ──────────────────────────────────────────────────── */
static void spi1_wait_tx(void)  { while (!(SPI1[SPI_SR / 4u] & SPI_SR_TXE)); }
static void spi1_wait_rx(void)  { while (!(SPI1[SPI_SR / 4u] & SPI_SR_RXNE)); }
static void spi1_wait_busy(void){ while (SPI1[SPI_SR / 4u] & SPI_SR_BSY); }

static void spi1_xfer(const uint8_t *tx, uint8_t *rx, uint32_t n)
{
    gpio_clr_pin((volatile uint32_t *)GPIOA_BASE, 4);  /* CS low */
    for (uint32_t i = 0; i < n; i++) {
        spi1_wait_tx();
        *(volatile uint8_t *)&SPI1[SPI_DR / 4u] = tx[i];
        spi1_wait_rx();
        uint8_t r = *(volatile uint8_t *)&SPI1[SPI_DR / 4u];
        if (rx) rx[i] = r;
    }
    spi1_wait_busy();
    gpio_set_pin((volatile uint32_t *)GPIOA_BASE, 4);  /* CS high */
}

/* ─── SPI1 init ───────────────────────────────────────────────────────── */
static void spi1_init(void)
{
    /* Master, CPOL=0 CPHA=0, fAPB=120 MHz / 16 = 7.5 MHz for config,
     * 8-bit frames, software CS. */
    SPI1[SPI_CR1 / 4u] = 0;
    SPI1[SPI_CR1 / 4u] = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (7u << 3);
    SPI1[SPI_CR1 / 4u] |= SPI_CR1_SPE;
}

/* ─── iCE40 configuration sequence ────────────────────────────────────── */
static bool fpga_configure(const uint8_t *bit, uint32_t len)
{
    volatile uint32_t *gpiob = (volatile uint32_t *)GPIOB_BASE;

    /* 1. Assert CRESET low for > 200 ns. */
    gpio_clr_pin(gpiob, 4);
    for (volatile int i = 0; i < 50; i++) ;   /* > 1 µs */
    gpio_set_pin(gpiob, 4);

    /* 2. Wait for CDONE to go high (FPGA in slave-SPI config mode). */
    uint32_t to = 0;
    while (gpio_read_pin(gpiob, 3)) {
        if (++to > 100000u) return false;
    }

    /* 3. Send the bitstream in 256-byte chunks. */
    for (uint32_t off = 0; off < len; off += 256) {
        uint32_t n = (len - off < 256) ? (len - off) : 256;
        spi1_xfer(&bit[off], NULL, n);
    }

    /* 4. Send at least 49 dummy clocks for the iCE40 to finish. */
    uint8_t dummy[8] = {0};
    spi1_xfer(dummy, NULL, 8);

    /* 5. Wait for CDONE high. */
    to = 0;
    while (!gpio_read_pin(gpiob, 3)) {
        if (++to > 100000u) return false;
    }

    return true;
}

/* ─── Public API ──────────────────────────────────────────────────────── */
void fpga_init(void)
{
    spi1_init();

    /* Drop SPI clock to config speed already set; after config we raise
     * it for command traffic. */
    if (fpga_configure(ps_bitstream, ps_bitstream_len)) {
        /* Raise SPI clock to 30 MHz for command traffic. */
        SPI1[SPI_CR1 / 4u] &= ~SPI_CR1_SPE;
        SPI1[SPI_CR1 / 4u]  = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (1u << 3);
        SPI1[SPI_CR1 / 4u] |= SPI_CR1_SPE;
        s_ready = true;
    } else {
        s_ready = false;
    }
}

bool fpga_ready(void) { return s_ready; }

void fpga_cmd(uint8_t op, uint32_t a, uint32_t b, uint32_t c)
{
    /* 17-byte packet: [op][a3 a2 a1 a0][b3 b2 b1 b0][c3 c2 c1 c0][crc][crc] */
    uint8_t tx[17];
    tx[0] = op;
    tx[1] = (a >> 24) & 0xFF; tx[2] = (a >> 16) & 0xFF;
    tx[3] = (a >> 8)  & 0xFF; tx[4] =  a        & 0xFF;
    tx[5] = (b >> 24) & 0xFF; tx[6] = (b >> 16) & 0xFF;
    tx[7] = (b >> 8)  & 0xFF; tx[8] =  b        & 0xFF;
    tx[9] = (c >> 24) & 0xFF; tx[10]= (c >> 16) & 0xFF;
    tx[11]= (c >> 8)  & 0xFF; tx[12]=  c        & 0xFF;
    /* Simple XOR-8 checksum over bytes 0..12, split into two nibbles. */
    uint8_t crc = 0;
    for (int i = 0; i < 13; i++) crc ^= tx[i];
    tx[13] = (crc >> 4) & 0xFu;
    tx[14] =  crc       & 0xFu;
    tx[15] = 0x00;  /* status reply placeholder */
    tx[16] = 0x00;
    spi1_xfer(tx, NULL, 17);
}

uint32_t fpga_read_status(void)
{
    /* Send a READ_STATUS opcode and read back 4 bytes. */
    uint8_t tx[5] = { FPGA_CMD_READ_STATUS, 0, 0, 0, 0 };
    uint8_t rx[5] = {0};
    spi1_xfer(tx, rx, 5);
    return ((uint32_t)rx[1] << 24) | ((uint32_t)rx[2] << 16) |
           ((uint32_t)rx[3] << 8)  |  (uint32_t)rx[4];
}

uint32_t fpga_read_clk(void)
{
    /* After MEASURE_CLK, the result is in the status word. */
    return fpga_read_status();
}