/*
 * w25q128.c — W25Q128JVSIQ 16 MB SPI NOR Flash driver
 * SPI1 @ 50 MHz, manual CS (PA4)
 */

#include "drivers/w25q128.h"
#include "registers.h"

/* SPI1 transfer timeout */
#define SPI_TIMEOUT  10000U

/* ========================================
 * SPI1 helper functions
 * ======================================== */

static uint8_t spi1_transfer(uint8_t tx_byte) {
    /* Wait for TXE (transmit buffer empty) */
    uint32_t timeout = SPI_TIMEOUT;
    while (!(SPI1->SR & SPI_SR_TXE)) {
        if (--timeout == 0) return 0xFF;
    }

    /* Send byte */
    SPI1->DR = tx_byte;

    /* Wait for RXNE (receive buffer not empty) */
    timeout = SPI_TIMEOUT;
    while (!(SPI1->SR & SPI_SR_RXNE)) {
        if (--timeout == 0) return 0xFF;
    }

    return (uint8_t)(SPI1->DR & 0xFF);
}

static void spi1_write(uint8_t tx_byte) {
    uint32_t timeout = SPI_TIMEOUT;
    while (!(SPI1->SR & SPI_SR_TXE)) {
        if (--timeout == 0) return;
    }
    SPI1->DR = tx_byte;
    /* Wait for TX complete */
    timeout = SPI_TIMEOUT;
    while (SPI1->SR & SPI_SR_BSY) {
        if (--timeout == 0) return;
    }
}

/* ========================================
 * W25Q128 initialization
 * ======================================== */

void w25q128_init(void) {
    /* SPI1 already configured in board_peripheral_init() */
    /* Verify JEDEC ID */
    uint32_t jedec = w25q128_read_jedec_id();
    (void)jedec;  /* Caller checks in main() */
}

/* ========================================
 * Read JEDEC ID (0xEF4018 for W25Q128JVSIQ)
 * ======================================== */

uint32_t w25q128_read_jedec_id(void) {
    uint32_t id = 0;

    W25Q128_CS_LOW();
    spi1_transfer(W25Q_JEDEC_ID);
    id = spi1_transfer(0xFF) << 16;  /* Manufacturer */
    id |= spi1_transfer(0xFF) << 8;   /* Type */
    id |= spi1_transfer(0xFF);          /* Capacity */
    W25Q128_CS_HIGH();

    return id;
}

/* ========================================
 * Read data from flash
 * ======================================== */

void w25q128_read(uint32_t addr, uint8_t *buf, uint16_t len) {
    W25Q128_CS_LOW();
    spi1_transfer(W25Q_READ);
    spi1_transfer((addr >> 16) & 0xFF);  /* Address [23:16] */
    spi1_transfer((addr >> 8) & 0xFF);   /* Address [15:8] */
    spi1_transfer(addr & 0xFF);           /* Address [7:0] */
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = spi1_transfer(0xFF);
    }
    W25Q128_CS_HIGH();
}

/* ========================================
 * Write enable
 * ======================================== */

void w25q128_write_enable(void) {
    W25Q128_CS_LOW();
    spi1_transfer(W25Q_WRITE_ENABLE);
    W25Q128_CS_HIGH();
}

/* ========================================
 * Write disable
 * ======================================== */

void w25q128_write_disable(void) {
    W25Q128_CS_LOW();
    spi1_transfer(W25Q_WRITE_DISABLE);
    W25Q128_CS_HIGH();
}

/* ========================================
 * Wait for write in progress to clear
 * ======================================== */

void w25q128_wait_busy(void) {
    uint8_t status;
    do {
        W25Q128_CS_LOW();
        spi1_transfer(W25Q_READ_SR1);
        status = spi1_transfer(0xFF);
        W25Q128_CS_HIGH();
    } while (status & 0x01);  /* WIP bit */
}

/* ========================================
 * Read status register
 * ======================================== */

uint8_t w25q128_read_status(uint8_t reg) {
    uint8_t cmd;
    switch (reg) {
    case 1: cmd = W25Q_READ_SR1; break;
    case 2: cmd = W25Q_READ_SR2; break;
    case 3: cmd = W25Q_READ_SR3; break;
    default: cmd = W25Q_READ_SR1; break;
    }

    W25Q128_CS_LOW();
    spi1_transfer(cmd);
    uint8_t status = spi1_transfer(0xFF);
    W25Q128_CS_HIGH();

    return status;
}

/* ========================================
 * Page program (write up to 256 bytes)
 * ======================================== */

void w25q128_write_page(uint32_t addr, const uint8_t *buf, uint16_t len) {
    if (len > W25Q128_PAGE_SIZE) len = W25Q128_PAGE_SIZE;

    w25q128_write_enable();

    W25Q128_CS_LOW();
    spi1_transfer(W25Q_PAGE_PROGRAM);
    spi1_transfer((addr >> 16) & 0xFF);
    spi1_transfer((addr >> 8) & 0xFF);
    spi1_transfer(addr & 0xFF);
    for (uint16_t i = 0; i < len; i++) {
        spi1_transfer(buf[i]);
    }
    W25Q128_CS_HIGH();

    w25q128_wait_busy();
}

/* ========================================
 * Sector erase (4 KB)
 * ======================================== */

void w25q128_erase_sector(uint32_t addr) {
    w25q128_write_enable();

    W25Q128_CS_LOW();
    spi1_transfer(W25Q_SECTOR_ERASE);
    spi1_transfer((addr >> 16) & 0xFF);
    spi1_transfer((addr >> 8) & 0xFF);
    spi1_transfer(addr & 0xFF);
    W25Q128_CS_HIGH();

    w25q128_wait_busy();
}

/* ========================================
 * Block erase (64 KB)
 * ======================================== */

void w25q128_erase_block_64k(uint32_t addr) {
    w25q128_write_enable();

    W25Q128_CS_LOW();
    spi1_transfer(W25Q_BLOCK_ERASE_64K);
    spi1_transfer((addr >> 16) & 0xFF);
    spi1_transfer((addr >> 8) & 0xFF);
    spi1_transfer(addr & 0xFF);
    W25Q128_CS_HIGH();

    w25q128_wait_busy();
}

/* ========================================
 * Chip erase (full 16 MB)
 * ======================================== */

void w25q128_erase_chip(void) {
    w25q128_write_enable();

    W25Q128_CS_LOW();
    spi1_transfer(W25Q_CHIP_ERASE);
    W25Q128_CS_HIGH();

    w25q128_wait_busy();  /* Can take up to 200 seconds! */
}