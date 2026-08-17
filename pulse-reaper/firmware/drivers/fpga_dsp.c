/*
 * fpga_dsp.c — FPGA command interface (SPI1 to iCE40UP5K)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The MCU is SPI master; the FPGA is a fixed-function DSP coprocessor.
 * All commands are 1 opcode byte followed by parameters / data.
 */

#include "fpga_dsp.h"
#include "board.h"
#include "registers.h"
#include "tdr_engine.h"
#include <string.h>

/* ----------------------------------------------------------------------- */
/*  SPI1 low-level                                                          */
/* ----------------------------------------------------------------------- */

static void spi1_select(void) {
    GPIOB->BRR = (1u << FPGA_CS_PIN);   /* CS low */
}

static void spi1_deselect(void) {
    GPIOB->BSRR = (1u << FPGA_CS_PIN);  /* CS high */
}

static uint8_t spi1_xfer(uint8_t tx) {
    SPI1->TXDR = (uint32_t)tx;
    while ((SPI1->SR & SPI_SR_EOT) == 0u) { /* spin */ }
    SPI1->IFCR = SPI_SR_EOT;
    return (uint8_t)SPI1->RXDR;
}

static void spi1_xfer_buf(const uint8_t *tx, uint8_t *rx, int len) {
    for (int i = 0; i < len; i++) {
        uint8_t t = tx ? tx[i] : 0xFFu;
        uint8_t r = spi1_xfer(t);
        if (rx) rx[i] = r;
    }
}

/* ----------------------------------------------------------------------- */
/*  Init                                                                    */
/* ----------------------------------------------------------------------- */

/* FTHLV value for threshold=1 byte: bits [4:2] -> 000 = 1 byte.
 * (Our minimal register map does not define this constant; define locally.) */
#define SPI_CFG1_FTHLV_1  0u

static void spi1_hw_init(void) {
    /* SPI1 is on APB2 (PCLK2 = 120 MHz). We want <= 50 MHz SPI clock.
     * MBR = log2(120/50) -> MBR=1 (f_CLK = 60 MHz) is the nearest config.
     * Use MBR=2 -> f_SPI = 30 MHz for safe margin. */
    SPI1->CFG1 = (2u << SPI_CFG1_MBR_SHIFT)   /* baud rate */
              | (7u)                          /* DSIZE: 8-bit (0..7 -> 8) */
              | SPI_CFG1_FTHLV_1;            /* FIFO threshold = 1 */
    SPI1->CFG2 = SPI_CFG2_MASTER
              | SPI_CFG2_SSM
              | SPI_CFG2_SSI
              | SPI_CFG2_CPHA
              | SPI_CFG2_CPOL;
    SPI1->CR1 = SPI_CR1_SPE;   /* enable */
}

void fpga_dsp_init(void) {
    spi1_hw_init();
    /* Release FPGA from reset */
    GPIOC->BRR  = (1u << FPGA_CRESET_PIN);   /* CRESET low */
    /* tiny delay */
    for (volatile int i = 0; i < 1000; i++) { /* ~4 us */ }
    GPIOC->BSRR = (1u << FPGA_CRESET_PIN);   /* CRESET high -> start */
    /* Wait for CDONE */
    int timeout = 100000;
    while (((GPIOC->IDR >> FPGA_CDONE_PIN) & 1u) == 0u && timeout-- > 0) {
        /* spin */
    }
    /* Configure default filter (flat passthrough) */
    static int16_t default_taps[64];
    memset(default_taps, 0, sizeof(default_taps));
    default_taps[0] = 4096;   /* passthrough gain */
    fpga_dsp_configure_filter(default_taps, 64);
}

int fpga_dsp_configure(void) {
    /* The FPGA bitstream is loaded from the W25Q128 on boot; this function
     * is a placeholder for a runtime reconfiguration path. */
    return 0;
}

/* ----------------------------------------------------------------------- */
/*  Status                                                                  */
/* ----------------------------------------------------------------------- */

uint8_t fpga_dsp_status(void) {
    spi1_select();
    uint8_t s = spi1_xfer(FPGA_CMD_VERSION);   /* opcode doubles as status read */
    s = spi1_xfer(0x00u);
    spi1_deselect();
    return s;
}

/* ----------------------------------------------------------------------- */
/*  TDR                                                                     */
/* ----------------------------------------------------------------------- */

int fpga_dsp_arm_tdr(void) {
    spi1_select();
    spi1_xfer(FPGA_CMD_ARM_TDR);
    spi1_deselect();
    /* Wait for TDR_READY */
    int timeout = 100000;
    while (timeout-- > 0) {
        if (fpga_dsp_status() & FPGA_STATUS_TDR_READY) return 0;
    }
    return -1;  /* timeout */
}

int fpga_dsp_read_reflectogram(int16_t *out, int max_samples) {
    if (max_samples <= 0) return 0;
    int n = (max_samples < (int)FPGA_REFLECTOGRAM_SAMPLES) ? max_samples
                                                           : (int)FPGA_REFLECTOGRAM_SAMPLES;
    spi1_select();
    spi1_xfer(FPGA_CMD_READ_REFLECTOGRAM);
    for (int i = 0; i < n; i++) {
        uint8_t lo = spi1_xfer(0x00u);
        uint8_t hi = spi1_xfer(0x00u);
        out[i] = (int16_t)((uint16_t)lo | ((uint16_t)hi << 8));
    }
    spi1_deselect();
    return n;
}

/* ----------------------------------------------------------------------- */
/*  Filter                                                                  */
/* ----------------------------------------------------------------------- */

int fpga_dsp_configure_filter(const int16_t *taps, int n_taps) {
    if (n_taps > 64) n_taps = 64;
    spi1_select();
    spi1_xfer(FPGA_CMD_CONFIGURE_FILTER);
    spi1_xfer((uint8_t)n_taps);
    for (int i = 0; i < n_taps; i++) {
        spi1_xfer((uint8_t)(taps[i] & 0xFFu));
        spi1_xfer((uint8_t)((taps[i] >> 8) & 0xFFu));
    }
    spi1_deselect();
    return 0;
}

/* ----------------------------------------------------------------------- */
/*  Frame read                                                              */
/* ----------------------------------------------------------------------- */

int fpga_dsp_read_frame(uint8_t *out, uint16_t *out_len, int max_len) {
    if (!out || !out_len || max_len <= 0) return 0;
    uint8_t st = fpga_dsp_status();
    if ((st & FPGA_STATUS_FRAME_AVAIL) == 0u) return 0;

    spi1_select();
    spi1_xfer(FPGA_CMD_READ_FRAME);
    uint8_t lo = spi1_xfer(0x00u);
    uint8_t hi = spi1_xfer(0x00u);
    uint16_t flen = (uint16_t)((uint16_t)lo | ((uint16_t)hi << 8));
    if (flen > (uint16_t)max_len) flen = (uint16_t)max_len;
    if (flen > FPGA_MAX_FRAME_LEN) flen = FPGA_MAX_FRAME_LEN;
    for (uint16_t i = 0; i < flen; i++) {
        out[i] = spi1_xfer(0x00u);
    }
    spi1_deselect();
    *out_len = flen;
    return (int)flen;
}

/* ----------------------------------------------------------------------- */
/*  Covert channel                                                          */
/* ----------------------------------------------------------------------- */

static uint8_t g_covert_tx_queue[16];
static uint8_t g_covert_tx_head = 0u;
static uint8_t g_covert_tx_tail = 0u;

void fpga_dsp_covert_pump(void) {
    /* If we have a queued byte, TX it */
    if (g_covert_tx_head != g_covert_tx_tail) {
        uint8_t b = g_covert_tx_queue[g_covert_tx_tail++];
        g_covert_tx_tail &= 0x0Fu;
        spi1_select();
        spi1_xfer(FPGA_CMD_COVERT_TX);
        spi1_xfer(b);
        spi1_deselect();
    }
    /* Try to RX a byte if available */
    uint8_t st = fpga_dsp_status();
    if (st & FPGA_STATUS_COVERT_RX_AVAIL) {
        /* byte is consumed by the caller via fpga_dsp_covert_rx_byte() */
    }
}

int fpga_dsp_covert_tx_byte(uint8_t b) {
    uint8_t next = (g_covert_tx_head + 1u) & 0x0Fu;
    if (next == g_covert_tx_tail) return -1;  /* queue full */
    g_covert_tx_queue[g_covert_tx_head++] = b;
    g_covert_tx_head = next;
    return 0;
}

int fpga_dsp_covert_rx_byte(void) {
    uint8_t st = fpga_dsp_status();
    if ((st & FPGA_STATUS_COVERT_RX_AVAIL) == 0u) return -1;
    spi1_select();
    spi1_xfer(FPGA_CMD_COVERT_RX);
    uint8_t b = spi1_xfer(0x00u);
    spi1_deselect();
    return (int)b;
}