/*
 * spi_flash.c — W25Q128JVSIQ SPI NOR Flash Driver
 * PhantomBridge PoE Network Implant
 */

#include "spi_flash.h"
#include "../board.h"
#include "../registers.h"

/* CS# control macros */
#define FLASH_CS_LOW()   GPIO_CLR(GPIOC_BASE, FLASH_CS_PIN)
#define FLASH_CS_HIGH()  GPIO_SET(GPIOC_BASE, FLASH_CS_PIN)

/* SPI2 transfer: write one byte, read one byte simultaneously */
static uint8_t spi2_xfer(uint8_t tx_byte) {
    /* Wait for TX empty */
    while (!(SPI2_SR & SPI_SR_TXE));
    SPI2_DR = tx_byte;
    /* Wait for RX not empty */
    while (!(SPI2_SR & SPI_SR_RXNE));
    return (uint8_t)SPI2_DR;
}

/* SPI2 write-only (no read data needed) */
static void spi2_write(uint8_t tx_byte) {
    while (!(SPI2_SR & SPI_SR_TXE));
    SPI2_DR = tx_byte;
    while (!(SPI2_SR & SPI_SR_TXE));
    /* Dummy read to clear RXNE and avoid overrun */
    (void)SPI2_DR;
}

/* ===== SPI2 Peripheral Init ===== */
static void spi2_hw_init(void) {
    /* Enable SPI2 clock */
    RCC_APB1LENR |= (1 << 14); /* SPI2EN */

    /* Disable SPI for configuration */
    SPI2_CR1 &= ~SPI_CR1_SPE;

    /* Configure SPI2: Master, CPOL=0, CPHA=0, 8-bit, MSB first */
    /* BR[2:0] = 010 = APB1CLK/16 = 120MHz/16 = 7.5MHz (safe for init) */
    SPI2_CR1 = SPI_CR1_MSTR | (2 << 3) | (0 << 0); /* Master, BR=/16, CPOL=0 */
    SPI2_CR2 = (1 << 0) | (1 << 2); /* DS=8bit, FRXTH=1/8 */

    /* Enable SPI */
    SPI2_CR1 |= SPI_CR1_SPE;
}

/* ===== Public API ===== */

int spi_flash_init(void) {
    spi2_hw_init();

    /* Release from power-down (just in case) */
    spi_flash_release_power_down();

    /* Small delay after power-down release */
    for (volatile int i = 0; i < 50000; i++);

    /* Read JEDEC ID to verify */
    w25q_jedec_id_t id;
    spi_flash_read_jedec_id(&id);

    if (id.manufacturer != W25Q_JEDEC_MANUFACTURER ||
        id.capacity != W25Q_JEDEC_CAPACITY) {
        return -1; /* Flash not detected or wrong part */
    }

    /* Switch SPI2 to faster clock now that we know flash is present */
    /* BR[2:0] = 000 = APB1CLK/2 = 120MHz/2 = 60MHz */
    SPI2_CR1 &= ~SPI_CR1_SPE;
    SPI2_CR1 = SPI2_CR1_MSTR | (0 << 3); /* Master, BR=/2 = 60MHz */
    SPI2_CR1 |= SPI_CR1_SPE;

    return 0;
}

int spi_flash_read_jedec_id(w25q_jedec_id_t *id) {
    FLASH_CS_LOW();
    spi2_xfer(W25Q_CMD_JEDEC_ID);
    id->manufacturer = spi2_xfer(0xFF);
    id->type = spi2_xfer(0xFF);
    id->capacity = spi2_xfer(0xFF);
    FLASH_CS_HIGH();
    return 0;
}

int spi_flash_read(uint32_t addr, uint8_t *buf, uint32_t len) {
    if (addr + len > W25Q_TOTAL_SIZE) return -1;

    FLASH_CS_LOW();
    spi2_xfer(W25Q_CMD_READ);
    spi2_xfer((addr >> 16) & 0xFF);
    spi2_xfer((addr >> 8) & 0xFF);
    spi2_xfer(addr & 0xFF);
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = spi2_xfer(0xFF);
    }
    FLASH_CS_HIGH();
    return 0;
}

int spi_flash_fast_read(uint32_t addr, uint8_t *buf, uint32_t len) {
    if (addr + len > W25Q_TOTAL_SIZE) return -1;

    FLASH_CS_LOW();
    spi2_xfer(W25Q_CMD_FAST_READ);
    spi2_xfer((addr >> 16) & 0xFF);
    spi2_xfer((addr >> 8) & 0xFF);
    spi2_xfer(addr & 0xFF);
    spi2_xfer(0xFF); /* 8 dummy clocks */
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = spi2_xfer(0xFF);
    }
    FLASH_CS_HIGH();
    return 0;
}

int spi_flash_page_program(uint32_t addr, const uint8_t *data, uint16_t len) {
    if (len == 0 || len > W25Q_PAGE_SIZE) return -1;
    if ((addr & 0xFF) + len > W25Q_PAGE_SIZE) return -1; /* Cross-page write */

    spi_flash_write_enable();

    FLASH_CS_LOW();
    spi2_xfer(W25Q_CMD_PAGE_PROGRAM);
    spi2_xfer((addr >> 16) & 0xFF);
    spi2_xfer((addr >> 8) & 0xFF);
    spi2_xfer(addr & 0xFF);
    for (uint16_t i = 0; i < len; i++) {
        spi2_xfer(data[i]);
    }
    FLASH_CS_HIGH();

    /* Wait for page program to complete (typical 0.7ms, max 3ms) */
    return spi_flash_wait_busy();
}

int spi_flash_write(uint32_t addr, const uint8_t *data, uint32_t len) {
    if (addr + len > W25Q_TOTAL_SIZE) return -1;

    uint32_t offset = 0;
    while (offset < len) {
        /* Calculate bytes remaining in current page */
        uint16_t page_offset = addr & (W25Q_PAGE_SIZE - 1);
        uint16_t chunk = W25Q_PAGE_SIZE - page_offset;
        if (chunk > (len - offset))
            chunk = (uint16_t)(len - offset);

        int ret = spi_flash_page_program(addr + offset, data + offset, chunk);
        if (ret != 0) return ret;

        offset += chunk;
    }
    return 0;
}

int spi_flash_sector_erase(uint32_t addr) {
    /* Mask to sector boundary */
    addr &= ~(W25Q_SECTOR_SIZE - 1);

    spi_flash_write_enable();

    FLASH_CS_LOW();
    spi2_xfer(W25Q_CMD_SECTOR_ERASE);
    spi2_xfer((addr >> 16) & 0xFF);
    spi2_xfer((addr >> 8) & 0xFF);
    spi2_xfer(addr & 0xFF);
    FLASH_CS_HIGH();

    /* Wait for sector erase to complete (typical 45ms, max 400ms) */
    return spi_flash_wait_busy();
}

int spi_flash_block_erase_64k(uint32_t addr) {
    addr &= ~(W25Q_BLOCK_64K - 1);

    spi_flash_write_enable();

    FLASH_CS_LOW();
    spi2_xfer(W25Q_CMD_BLOCK_ERASE_64);
    spi2_xfer((addr >> 16) & 0xFF);
    spi2_xfer((addr >> 8) & 0xFF);
    spi2_xfer(addr & 0xFF);
    FLASH_CS_HIGH();

    /* Wait for block erase (typical 150ms, max 2s) */
    return spi_flash_wait_busy();
}

int spi_flash_chip_erase(void) {
    spi_flash_write_enable();

    FLASH_CS_LOW();
    spi2_xfer(W25Q_CMD_CHIP_ERASE);
    FLASH_CS_HIGH();

    /* Wait for chip erase (typical 25s, max 100s) */
    return spi_flash_wait_busy();
}

int spi_flash_wait_busy(void) {
    uint32_t timeout = 10000000; /* ~10 second timeout at 480MHz */
    while (timeout--) {
        if (!(spi_flash_read_status() & W25Q_SR1_BUSY))
            return 0;
    }
    return -1; /* Timeout */
}

uint8_t spi_flash_read_status(void) {
    uint8_t status;
    FLASH_CS_LOW();
    spi2_xfer(W25Q_CMD_READ_SR1);
    status = spi2_xfer(0xFF);
    FLASH_CS_HIGH();
    return status;
}

int spi_flash_write_enable(void) {
    FLASH_CS_LOW();
    spi2_xfer(W25Q_CMD_WRITE_ENABLE);
    FLASH_CS_HIGH();

    /* Verify WEL is set */
    if (!(spi_flash_read_status() & W25Q_SR1_WEL))
        return -1;

    return 0;
}

int spi_flash_power_down(void) {
    FLASH_CS_LOW();
    spi2_xfer(W25Q_CMD_POWER_DOWN);
    FLASH_CS_HIGH();
    return 0;
}

int spi_flash_release_power_down(void) {
    FLASH_CS_LOW();
    spi2_xfer(W25Q_CMD_RELEASE_PD);
    FLASH_CS_HIGH();
    /* tRES1 = 3µs typical */
    for (volatile int i = 0; i < 5000; i++);
    return 0;
}