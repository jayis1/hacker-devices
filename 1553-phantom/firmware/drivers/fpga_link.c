/*
 * fpga_link.c — SPI master link to the iCE40 codec + arm-latch gate
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The MCU is SPI master; the FPGA presents a 16-bit register interface:
 *
 *   TX path:   MCU writes [reg=TX_CTL][word_lo][word_hi]...[reg=TX_CTL|GO]
 *   RX path:   MCU reads [reg=RX_RD][word][reg=RX_TS_LO][ts_lo]
 *
 * The FPGA asserts a falling edge on PA11 (FPGA_IRQ) when a new RX word is
 * latched. We let the EXTI ISR schedule a pump from the main loop (the
 * pump itself is safe in thread context because the RX FIFO is
 * single-writer/single-reader from the MCU's point of view).
 */

#include "../board.h"
#include "../registers.h"
#include "fpga_link.h"
#include "capture.h"
#include <string.h>

/* ---- Channel state (per FPGA status reg) ---- */
static uint32_t chan_errs[N_CHAN];
static uint32_t chan_words[N_CHAN];

/* ---- Pending RX state ---- */
static volatile uint8_t rx_pending = 0;

/* ---- Soft arm gate (FPGA-side mirror of PB11) ---- */
static uint8_t fpga_armed = 0;

/* =========================================================================
 * Init
 * ========================================================================= */
void fpga_link_init(void) {
    /* Hold FPGA in reset for 10 ms, then release */
    GPIO_CLR(GPIOA, PA12_FPGA_RST);
    for (volatile int i = 0; i < 100000; i++) __asm volatile("nop");
    GPIO_SET(GPIOA, PA12_FPGA_RST);

    /* Wait for CDONE */
    for (int i = 0; i < 100000 && !GPIO_GET(GPIOB, PB5_FPGA_CDONE); i++) {
        __asm volatile("nop");
    }
    memset(chan_errs, 0, sizeof(chan_errs));
    memset(chan_words, 0, sizeof(chan_words));
    rx_pending = 0;
    fpga_armed = 0;
}

/* =========================================================================
 * Low-level SPI register access
 * ========================================================================= */
static uint16_t spi_xfer(uint16_t tx) {
    /* Write 16-bit frame, wait, read back */
    SPI1->DR = tx;
    while (!(SPI1->SR & SPI_SR_RXNE)) { }
    return (uint16_t)SPI1->DR;
}

static uint16_t fpga_read(uint8_t reg) {
    GPIO_CLR(GPIOA, PA4_FPGA_CS);     /* active low */
    spi_xfer((uint16_t)(0x8000 | reg));   /* R/W bit = 1 for read */
    uint16_t v = spi_xfer(0);
    GPIO_SET(GPIOA, PA4_FPGA_CS);
    return v;
}

static void fpga_write(uint8_t reg, uint16_t val) {
    GPIO_CLR(GPIOA, PA4_FPGA_CS);
    spi_xfer((uint16_t)reg);             /* R/W bit = 0 for write */
    spi_xfer(val);
    GPIO_SET(GPIOA, PA4_FPGA_CS);
}

/* =========================================================================
 * Status read
 * ========================================================================= */
uint16_t fpga_link_status(void) {
    return fpga_read(FPGA_REG_STATUS);
}

uint32_t fpga_link_timestamp(void) {
    uint32_t t = fpga_read(FPGA_REG_TIMESTAMP);
    t |= ((uint32_t)fpga_read(FPGA_REG_RX_TS_LO) << 16);
    return t;
}

uint32_t fpga_link_chan_errs(int ch) {
    if (ch < 0 || ch >= N_CHAN) return 0;
    return chan_errs[ch];
}

/* =========================================================================
 * TX path: send a 20-bit 1553 word on the selected channel
 * ========================================================================= */
int fpga_link_tx_word(int ch, uint32_t word20) {
    if (ch < 0 || ch >= N_CHAN) return -1;
    if (!fpga_armed) return -2;            /* refused unless armed */
    /* Write data word (low 16 bits), then high nibble, then GO */
    fpga_write(0x10, (uint16_t)(word20 & 0xFFFFu));
    fpga_write(0x11, (uint16_t)((word20 >> 16) & 0xFu));
    fpga_write(FPGA_REG_TX_CTL, (uint16_t)(ch | 0x80u));   /* GO bit */
    return 0;
}

/* =========================================================================
 * RX pump: read all pending RX words, push to capture ring
 * ========================================================================= */
void fpga_link_pump(void) {
    if (!rx_pending) return;
    rx_pending = 0;

    while (1) {
        uint16_t st = fpga_read(FPGA_REG_STATUS);
        int ch0_rdy = (st & STATUS_CH0_RDY) != 0;
        int ch1_rdy = (st & STATUS_CH1_RDY) != 0;
        if (!ch0_rdy && !ch1_rdy) break;

        int ch = ch0_rdy ? 0 : 1;
        uint16_t word = fpga_read(FPGA_REG_RX_RD);
        uint32_t ts   = fpga_link_timestamp();

        if (st & (STATUS_CH0_ERR << ch)) {
            chan_errs[ch]++;
        }
        chan_words[ch]++;

        /* Push to capture ring (decode word type in capture.c) */
        capture_push(ch, word, ts, st);
    }
}

void fpga_link_on_irq(void) {
    rx_pending = 1;   /* serviced from main loop */
}

/* =========================================================================
 * Arm gate (FPGA-side mirror of PB11 hardware gate)
 * ========================================================================= */
void fpga_link_arm(void) {
    fpga_write(FPGA_REG_ARM, 0xA5A5);   /* magic: enable tx path */
    fpga_armed = 1;
}

void fpga_link_disarm(void) {
    fpga_write(FPGA_REG_ARM, 0x0000);
    fpga_write(FPGA_REG_TX_CTL, 0);     /* clear any pending GO */
    fpga_armed = 0;
}

int fpga_link_armed(void) { return fpga_armed; }

/* =========================================================================
 * Fuzz / gap overrides (pass-through to codec)
 * ========================================================================= */
void fpga_link_set_fuzz(uint16_t mask) { fpga_write(FPGA_REG_FUZZ, mask); }
void fpga_link_set_gap(uint16_t us)    { fpga_write(FPGA_REG_GAP_US, us); }

/* =========================================================================
 * Helper: encode a 1553 command word
 *   bits 19..15: RT address
 *   bit  14:     TX (1) / RX (0)
 *   bit  13..9:  subaddress (5 bits)
 *   bits  8..4: (unused for mode codes; reserved)
 *   bits  4..0: word count (0 → 32)
 * ========================================================================= */
uint32_t fpga_link_make_cmd(uint8_t rt, int tx, uint8_t sa, uint8_t wc) {
    uint32_t w = ((uint32_t)(rt & 0x1F) << 15)
               | ((uint32_t)(tx ? 1 : 0) << 14)
               | ((uint32_t)(sa & 0x1F) << 10)
               | ((uint32_t)(wc & 0x1F));
    /* parity bit (odd parity over 16 data bits) */
    uint32_t p = w ^ (w >> 8);
    p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
    return (w << 1) | ((p & 1) ? 0u : 1u);
}

/* Helper: encode a 1553 status word (RT replies) */
uint32_t fpga_link_make_status(uint8_t rt, uint16_t fault_mask) {
    uint32_t w = ((uint32_t)(rt & 0x1F) << 11)
               | ((uint32_t)(fault_mask & 0x07FF));
    /* bit 15 = message error (set by fault_mask), bit 0 = terminal flag */
    uint32_t p = w ^ (w >> 8);
    p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
    return (w << 1) | ((p & 1) ? 0u : 1u);
}

/* Helper: encode a 1553 data word (16 data bits + parity) */
uint32_t fpga_link_make_data(uint16_t data) {
    uint32_t p = data ^ (data >> 8);
    p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
    return ((uint32_t)data << 1) | ((p & 1) ? 0u : 1u);
}

/* Helper: decode a captured 20-bit word back into components */
void fpga_link_decode(uint32_t w20, decoded_word_t *out) {
    out->raw = w20;
    out->parity_ok = (w20 & 1);
    uint32_t w = w20 >> 1;     /* 19 data bits */
    /* Sync head bits 18..17 distinguish command (10) vs data (01) vs status */
    uint32_t sync = (w >> 16) & 0x3u;
    if (sync == 0b10) {            /* command/status family */
        uint32_t rt = (w >> 11) & 0x1F;
        /* A status word has the same bit layout as a command word; we
         * disambiguate by context (TX direction) — capture.c tags it. */
        out->type = WORD_CMD;
        out->rt   = rt;
        out->tx   = (w >> 10) & 1;
        out->sa   = (w >> 5) & 0x1F;
        out->wc   = w & 0x1F;
    } else {                       /* data */
        out->type = WORD_DATA;
        out->data = (uint16_t)w;
    }
}