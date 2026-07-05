/*
 * fpga.c — SPI command interface to the Artix-7 FPGA
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The FPGA implements a simple SPI peripheral: the MCU asserts CS_N (PA4),
 * writes a command byte + args, and reads back responses. All DDR4 bus
 * operations are issued by the FPGA under the MCU's explicit command.
 */

#include "board.h"
#include "registers.h"

#define FPGA_CS_HIGH()  (GPIOA->ODR |= (1u<<PA4))
#define FPGA_CS_LOW()   (GPIOA->ODR &= ~(1u<<PA4))

static uint8_t spi_xfer(uint8_t tx) {
    /* Wait for TXE, write DR, wait for RXNE, read DR */
    while (!(SPI1_SR & SPI_SR_TXE)) {}
    *(volatile uint8_t *)&SPI1_DR = tx;
    while (!(SPI1_SR & SPI_SR_RXNE)) {}
    return (uint8_t)SPI1_DR;
}

static void spi_write_cmd(uint8_t cmd, const uint8_t *args, uint8_t nargs) {
    FPGA_CS_LOW();
    spi_xfer(cmd);
    for (uint8_t i = 0; i < nargs; i++) spi_xfer(args[i]);
    FPGA_CS_HIGH();
}

static uint8_t spi_read_status(void) {
    FPGA_CS_LOW();
    spi_xfer(FPGA_CMD_STATUS);
    uint8_t s0 = spi_xfer(0x00);
    uint8_t s1 = spi_xfer(0x00);
    uint8_t s2 = spi_xfer(0x00);
    uint8_t s3 = spi_xfer(0x00);
    FPGA_CS_HIGH();
    return (uint32_t)s0 | ((uint32_t)s1<<8) | ((uint32_t)s2<<16) | ((uint32_t)s3<<24);
}

/* public API */

int fpga_init(void) {
    /* Pulse PROG_N low to re-load the bitstream from on-board QSPI flash. */
    GPIOC->ODR &= ~(1u<<PC5);     /* PROG_N low */
    for (volatile int i = 0; i < 10000; i++) {}
    GPIOC->ODR |= (1u<<PC5);     /* PROG_N high */
    /* Wait for CFG_DONE_N (PC4) to go low */
    uint32_t t0 = millis();
    while ((GPIOC->IDR & (1u<<PC4)) && (millis() - t0) < 2000) {}
    if (GPIOC->IDR & (1u<<PC4)) return -1;
    return 0;
}

uint32_t fpga_status(void) {
    return spi_read_status();
}

int fpga_set_mode(device_mode_t m) {
    uint8_t arg = (uint8_t)m;
    spi_write_cmd(FPGA_CMD_SETMODE, &arg, 1);
    return 0;
}

int fpga_arm(const char token[ARM_TOKEN_LEN]) {
    uint8_t buf[ARM_TOKEN_LEN];
    memcpy(buf, token, ARM_TOKEN_LEN);
    spi_write_cmd(FPGA_CMD_ARM, buf, ARM_TOKEN_LEN);
    return 0;
}

void fpga_disarm(void) {
    spi_write_cmd(FPGA_CMD_DISARM, NULL, 0);
}

int fpga_hammer_pattern(uint32_t aggressor_row, uint32_t count, uint8_t pattern_id) {
    uint8_t buf[1 + 4 + 4 + 1];
    buf[0] = pattern_id;
    buf[1] = (uint8_t)(aggressor_row & 0xFF);
    buf[2] = (uint8_t)((aggressor_row >> 8) & 0xFF);
    buf[3] = (uint8_t)((aggressor_row >> 16) & 0xFF);
    buf[4] = (uint8_t)((aggressor_row >> 24) & 0xFF);
    buf[5] = (uint8_t)(count & 0xFF);
    buf[6] = (uint8_t)((count >> 8) & 0xFF);
    buf[7] = (uint8_t)((count >> 16) & 0xFF);
    buf[8] = (uint8_t)((count >> 24) & 0xFF);
    buf[9] = pattern_id;  /* second copy for sanity */
    spi_write_cmd(FPGA_CMD_HAMMER_PAT, buf, 10);
    return 0;
}

int fpga_drain_snoop(void *buf, uint16_t max_records, uint16_t *out_records) {
    uint8_t *p = (uint8_t *)buf;
    FPGA_CS_LOW();
    spi_xfer(FPGA_CMD_DRAIN_SNOOP);
    /* read 2-byte count */
    uint16_t cnt = (uint16_t)(spi_xfer(0) | (spi_xfer(0) << 8));
    if (cnt > max_records) cnt = max_records;
    /* each record is 64 bytes */
    for (uint16_t i = 0; i < cnt; i++) {
        for (int b = 0; b < 64; b++) {
            p[i*64 + b] = spi_xfer(0);
        }
    }
    FPGA_CS_HIGH();
    if (out_records) *out_records = cnt;
    return 0;
}

int fpga_drain_flip(uint32_t *victim_row, uint32_t *flip_mask) {
    FPGA_CS_LOW();
    spi_xfer(FPGA_CMD_DRAIN_FLIP);
    uint32_t vrow = (uint32_t)spi_xfer(0) | ((uint32_t)spi_xfer(0)<<8)
                  | ((uint32_t)spi_xfer(0)<<16) | ((uint32_t)spi_xfer(0)<<24);
    uint32_t mask = (uint32_t)spi_xfer(0) | ((uint32_t)spi_xfer(0)<<8)
                  | ((uint32_t)spi_xfer(0)<<16) | ((uint32_t)spi_xfer(0)<<24);
    FPGA_CS_HIGH();
    if (victim_row) *victim_row = vrow;
    if (flip_mask)  *flip_mask  = mask;
    return 0;
}

int fpga_drain_image(void *buf, uint32_t len, uint32_t *out_len) {
    uint8_t *p = (uint8_t *)buf;
    FPGA_CS_LOW();
    spi_xfer(FPGA_CMD_DRAIN_IMG);
    /* read 4-byte actual length */
    uint32_t n = (uint32_t)spi_xfer(0) | ((uint32_t)spi_xfer(0)<<8)
               | ((uint32_t)spi_xfer(0)<<16) | ((uint32_t)spi_xfer(0)<<24);
    if (n > len) n = len;
    for (uint32_t i = 0; i < n; i++) p[i] = spi_xfer(0);
    FPGA_CS_HIGH();
    if (out_len) *out_len = n;
    return 0;
}

int fpga_covert_config(uint8_t bank_group, uint8_t bank) {
    uint8_t buf[2] = { bank_group, bank };
    spi_write_cmd(FPGA_CMD_COVERT_CFG, buf, 2);
    return 0;
}