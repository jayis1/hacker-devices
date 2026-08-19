/*
 * fpga_spi.c — Spartan-7 FPGA control & classifier interface
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * SPI command frame:
 *   [CMD:1][ADDR:1][LEN:2][DATA:LEN]
 *
 * Commands:
 *   0x01 SET_THRESHOLDS  (data = semi_db, metal_db)
 *   0x02 SET_INTEGRATION (data = n_spectra)
 *   0x03 READ_RESULT     (returns 5 bytes: p2_hi, p2_lo, p3_hi, p3_lo, ratio)
 *   0x04 READ_STATUS     (returns 4 bytes: status word)
 *   0x05 RESET           (soft reset of the classifier pipeline)
 */
#include "../registers.h"
#include "../board.h"
#include "fpga_spi.h"

#define FPGA_CMD_SET_THRESH   0x01u
#define FPGA_CMD_SET_INTEG    0x02u
#define FPGA_CMD_READ_RESULT  0x03u
#define FPGA_CMD_READ_STATUS  0x04u
#define FPGA_CMD_RESET        0x05u

static void fpga_cs_low(void)
{
    GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << FPGA_CS_PIN);
}
static void fpga_cs_high(void)
{
    GPIO_OUTSET(NRF_GPIO_BASE) = (1u << FPGA_CS_PIN);
}

static void fpga_xfer(uint8_t *tx, uint8_t *rx, uint16_t len)
{
    /* Use SPIM1 (shared with AD9361); AD9361 CS is high while we assert
     * FPGA CS, so no bus collision. */
    SPIM_TXD_PTR(AD9361_SPI_BASE)   = (uint32_t)tx;
    SPIM_TXD_MAXCNT(AD9361_SPI_BASE) = len;
    SPIM_RXD_PTR(AD9361_SPI_BASE)   = (rx) ? (uint32_t)rx : 0;
    SPIM_RXD_MAXCNT(AD9361_SPI_BASE) = (rx) ? len : 0;
    SPIM_START_TX(AD9361_SPI_BASE);
    while (!SPIM_END_EVENT(AD9361_SPI_BASE)) { /* spin */ }
    SPIM_END_CLEAR(AD9361_SPI_BASE);
}

void fpga_boot(void)
{
    /* Pulse PROGRAM_B low for 1 us, then high. Wait up to 100 ms for DONE. */
    GPIO_PIN_CNF(NRF_GPIO_BASE, FPGA_PROG_B_PIN) =
        GPIO_CNF_DIR_OUTPUT | (GPIO_CNF_DRIVE_H0H1 << 2);
    GPIO_OUTCLR(NRF_GPIO_BASE) = (1u << FPGA_PROG_B_PIN);
    nrf_delay_us(1);
    GPIO_OUTSET(NRF_GPIO_BASE) = (1u << FPGA_PROG_B_PIN);

    uint32_t timeout = 100000u;  /* 100 ms in 1 us steps */
    while (!(GPIO_OUT(NRF_GPIO_BASE) & (1u << FPGA_DONE_PIN)) && timeout--) {
        nrf_delay_us(1);
    }
    /* If DONE never asserted, the FPGA bitstream didn't load — the
     * device can still operate in a degraded mode (no FFT, raw RSSI
     * only). We log the fault but don't halt. */
}

void fpga_set_thresholds(int8_t semi_db, int8_t metal_db)
{
    uint8_t tx[6] = { FPGA_CMD_SET_THRESH, 0x00, 2,
                      (uint8_t)semi_db, (uint8_t)metal_db, 0 };
    fpga_cs_low();
    fpga_xfer(tx, 0, 6);
    fpga_cs_high();
}

void fpga_set_integration(uint8_t n_spectra)
{
    uint8_t tx[5] = { FPGA_CMD_SET_INTEG, 0x00, 1, n_spectra, 0 };
    fpga_cs_low();
    fpga_xfer(tx, 0, 5);
    fpga_cs_high();
}

uint8_t fpga_read_result(fpga_result_t *out)
{
    /* Check the FPGA IRQ pin — high when a fresh result is latched. */
    if (!(GPIO_OUT(NRF_GPIO_BASE) & (1u << FPGA_IRQ_PIN))) return 0;

    uint8_t tx[8] = { FPGA_CMD_READ_RESULT, 0x00, 5, 0, 0, 0, 0, 0 };
    uint8_t rx[8] = { 0 };
    fpga_cs_low();
    fpga_xfer(tx, rx, 8);
    fpga_cs_high();

    /* rx[0..2] = cmd echo; rx[3..7] = result payload */
    out->p2_dbfs = (int16_t)((rx[3] << 8) | rx[4]);
    out->p3_dbfs = (int16_t)((rx[5] << 8) | rx[6]);
    out->ratio_db = (int8_t)rx[7];
    return 1;
}

uint32_t fpga_get_status(void)
{
    uint8_t tx[8] = { FPGA_CMD_READ_STATUS, 0x00, 4, 0, 0, 0, 0, 0 };
    uint8_t rx[8] = { 0 };
    fpga_cs_low();
    fpga_xfer(tx, rx, 8);
    fpga_cs_high();
    return ((uint32_t)rx[3] << 24) | ((uint32_t)rx[4] << 16) |
           ((uint32_t)rx[5] << 8)  | (uint32_t)rx[6];
}

/* EOF — fpga_spi.c — jayis1 */