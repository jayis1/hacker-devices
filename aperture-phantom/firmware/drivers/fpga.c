/*
 * aperture-phantom / firmware / drivers / fpga.c
 *
 * SPI interface to the Lattice CrossLink-NX FPGA bridge. The FPGA presents
 * a 64-entry 32-bit register file and two FIFOs (capture, inject) accessed
 * via SPI command packets:
 *   read reg : [0x10 | addr]              → 4 bytes back
 *   write reg: [0x20 | addr] [4 bytes BE] → ack byte
 *   read cap : [0x40] [len BE u16]        → len bytes from capture FIFO
 *   write inj: [0x60] [len BE u16] [data] → ack byte
 * All transfers are full-duplex over SPI1 with PB1 as manual CS.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include "board.h"

#define SPI_CMD_RD_REG  0x10u
#define SPI_CMD_WR_REG  0x20u
#define SPI_CMD_RD_CAP  0x40u
#define SPI_CMD_WR_INJ  0x60u
#define SPI_CMD_IRQ_ACK 0x80u

static void spi_cs_low(void)  { GPIO_CLR(GPIOB_BASE, PIN_FPGA_CS); }
static void spi_cs_high(void) { GPIO_SET(GPIOB_BASE, PIN_FPGA_CS); }

static uint8_t spi_xfer(uint8_t tx) {
    /* Wait for TXP, write byte, wait for RXP, read byte. */
    while ((SPI1_SR & SPI1_SR_TXP) == 0) { }
    SPI1_DR = tx;
    while ((SPI1_SR & SPI1_SR_RXP) == 0) { }
    return (uint8_t)SPI1_DR;
}

static void spi_init(void) {
    /* Disable SPI, configure as main, 8-bit, CPOL=0 CPHA=0, baud = APB2/16. */
    SPI1_CR1 = 0;
    SPI1_CFG1 = (8u - 1u) << 0          /* MBR_SIZE = 8 bits */
              | (7u << 28);             /* MBRDIV = /16 (120/16=7.5 MHz) */
    SPI1_CFG2 = (1u << 22)              /* MASTER */
              | (0u << 0)               /* CPHA=0 */
              | (0u << 1);              /* CPOL=0 */
    SPI1_CR2 = 0;
    SPI1_CR1 = (1u << 0);               /* SPE */
}

void fpga_init(void) {
    spi_init();
    /* Release FPGA reset (already high after board_init), wait for version. */
    uint32_t v = fpga_read_reg(FPGA_REG_VERSION);
    (void)v; /* logf("fpga ver 0x%08x\r\n", v); */
}

void fpga_reset(void) {
    GPIO_CLR(GPIOB_BASE, PIN_FPGA_RST);
    for (volatile int i = 0; i < 100000; i++) { }
    GPIO_SET(GPIOB_BASE, PIN_FPGA_RST);
    for (volatile int i = 0; i < 100000; i++) { }
    fpga_write_reg(FPGA_REG_CONTROL, FPGA_CTRL_RESET);
    fpga_write_reg(FPGA_REG_CONTROL, 0);
}

void fpga_write_reg(uint8_t addr, uint32_t val) {
    spi_cs_low();
    spi_xfer((uint8_t)(SPI_CMD_WR_REG | (addr & 0x3F)));
    spi_xfer((uint8_t)(val >> 24));
    spi_xfer((uint8_t)(val >> 16));
    spi_xfer((uint8_t)(val >> 8));
    spi_xfer((uint8_t)(val & 0xFF));
    spi_xfer(0x00); /* dummy to read ack */
    spi_cs_high();
}

uint32_t fpga_read_reg(uint8_t addr) {
    spi_cs_low();
    spi_xfer((uint8_t)(SPI_CMD_RD_REG | (addr & 0x3F)));
    uint8_t b3 = spi_xfer(0x00);
    uint8_t b2 = spi_xfer(0x00);
    uint8_t b1 = spi_xfer(0x00);
    uint8_t b0 = spi_xfer(0x00);
    spi_cs_high();
    return ((uint32_t)b3 << 24) | ((uint32_t)b2 << 16) |
           ((uint32_t)b1 << 8)  | (uint32_t)b0;
}

int fpga_wait_rx_lock(uint32_t timeout_ms) {
    extern volatile uint32_t g_ticks_ms;
    uint32_t t0 = g_ticks_ms;
    while ((g_ticks_ms - t0) < timeout_ms) {
        if (fpga_read_reg(FPGA_REG_STATUS) & FPGA_STATUS_RX_LOCK) return 0;
    }
    return -1;
}

void fpga_set_mode(op_mode_t m) {
    uint32_t ctrl = 0;
    switch (m) {
    case MODE_PASSTHROUGH: ctrl = FPGA_CTRL_PASSTHROUGH; break;
    case MODE_CAPTURE:     ctrl = FPGA_CTRL_PASSTHROUGH | FPGA_CTRL_CAP_ENABLE
                                   | FPGA_CTRL_CRC_CHECK | FPGA_CTRL_ECC_CHECK; break;
    case MODE_INJECT:      ctrl = FPGA_CTRL_INJECT_ENABLE | FPGA_CTRL_MUTE_SENSOR; break;
    case MODE_REPLAY:      ctrl = FPGA_CTRL_INJECT_ENABLE; break;
    case MODE_FUZZ:        ctrl = FPGA_CTRL_INJECT_ENABLE; break;
    }
    fpga_write_reg(FPGA_REG_CONTROL, ctrl);
}

void fpga_start_capture(void) {
    uint32_t c = fpga_read_reg(FPGA_REG_CONTROL);
    c |= FPGA_CTRL_CAP_ENABLE;
    fpga_write_reg(FPGA_REG_CONTROL, c);
}
void fpga_stop_capture(void) {
    uint32_t c = fpga_read_reg(FPGA_REG_CONTROL);
    c &= ~FPGA_CTRL_CAP_ENABLE;
    fpga_write_reg(FPGA_REG_CONTROL, c);
}

uint32_t fpga_capture_level(void) {
    return fpga_read_reg(FPGA_REG_CAP_FIFO_LVL);
}

uint32_t fpga_capture_drain(uint8_t *dst, uint32_t max) {
    uint32_t avail = fpga_capture_level();
    if (avail == 0) return 0;
    uint32_t n = (avail < max) ? avail : max;
    spi_cs_low();
    spi_xfer(SPI_CMD_RD_CAP);
    spi_xfer((uint8_t)(n >> 8));
    spi_xfer((uint8_t)(n & 0xFF));
    for (uint32_t i = 0; i < n; i++) {
        dst[i] = spi_xfer(0x00);
    }
    spi_cs_high();
    return n;
}

int fpga_inject_load(const uint8_t *src, uint32_t n) {
    uint32_t free_lvl = fpga_read_reg(FPGA_REG_INJ_FIFO_LVL);
    if (n > free_lvl) return -1;
    spi_cs_low();
    spi_xfer(SPI_CMD_WR_INJ);
    spi_xfer((uint8_t)(n >> 8));
    spi_xfer((uint8_t)(n & 0xFF));
    for (uint32_t i = 0; i < n; i++) spi_xfer(src[i]);
    spi_xfer(0x00); /* ack dummy */
    spi_cs_high();
    return 0;
}

void fpga_inject_trigger(void) {
    fpga_write_reg(FPGA_REG_TX_TRIGGER, 1);
}

void fpga_mute_sensor(int on) {
    uint32_t c = fpga_read_reg(FPGA_REG_CONTROL);
    if (on) c |= FPGA_CTRL_MUTE_SENSOR; else c &= ~FPGA_CTRL_MUTE_SENSOR;
    fpga_write_reg(FPGA_REG_CONTROL, c);
}

uint8_t fpga_set_fuzz_bits(uint8_t mask) {
    uint32_t c = fpga_read_reg(FPGA_REG_CONTROL);
    c &= ~(FPGA_CTRL_TX_CRC_BAD | FPGA_CTRL_TX_SHORT_LINE |
           FPGA_CTRL_TX_BAD_DT  | FPGA_CTRL_TX_OVERSIZE);
    if (mask & 0x01) c |= FPGA_CTRL_TX_SHORT_LINE;
    if (mask & 0x02) c |= FPGA_CTRL_TX_BAD_DT;
    if (mask & 0x08) c |= FPGA_CTRL_TX_OVERSIZE;
    if (mask & 0x10) c |= FPGA_CTRL_TX_CRC_BAD;
    fpga_write_reg(FPGA_REG_CONTROL, c);
    return (uint8_t)c;
}

uint32_t fpga_status(void)      { return fpga_read_reg(FPGA_REG_STATUS); }
uint32_t fpga_frame_count(void) { return fpga_read_reg(FPGA_REG_FRAME_COUNT); }
uint32_t fpga_frame_len(void)   { return fpga_read_reg(FPGA_REG_FRAME_LEN); }
uint32_t fpga_crc_err_count(void){ return fpga_read_reg(FPGA_REG_CRC_ERR_COUNT); }