/*
 * spi_flash.c — SPI Flash (MX25LW1636) Driver for Substation
 * Implements read, write, erase operations via SSI1 (SPI)
 */

#include "spi_flash.h"
#include "board.h"
#include "registers.h"

/* SPI Flash chip select (active low) */
#define FLASH_CS_LOW()   (GPIO_DOUT0 &= ~(1 << PIN_FLASH_CS))
#define FLASH_CS_HIGH()  (GPIO_DOUT0 |=  (1 << PIN_FLASH_CS))

/* Wait for SSI TX FIFO to be not full */
static void spi_wait_tnf(void) {
    while (!(SSI_SR(SSI1_BASE) & SSI_SR_TNF))
        ;
}

/* Wait for SSI RX FIFO to have data */
static void spi_wait_rne(void) {
    while (!(SSI_SR(SSI1_BASE) & SSI_SR_RNE))
        ;
}

/* Transfer one byte over SPI1 */
static uint8_t spi_xfer(uint8_t tx) {
    spi_wait_tnf();
    SSI_DR(SSI1_BASE) = tx;
    spi_wait_rne();
    return (uint8_t)SSI_DR(SSI1_BASE);
}

/* Initialize SPI1 for Flash access */
int spi_flash_init(void) {
    /* Configure SSI1 as SPI master, 8-bit, mode 0 */
    SSI_CR0(SSI1_BASE) = SSI_CR0_DSS_8BIT | SSI_CR0_FRF_SPI;
    SSI_CPSR(SSI1_BASE) = 2;  /* Clock prescale: 48 MHz / 2 = 24 MHz */
    SSI_CR1(SSI1_BASE) = SSI_CR1_SSE;  /* Enable SSI */

    /* Set CS high (deselected) */
    FLASH_CS_HIGH();

    /* Release from reset */
    FLASH_CS_LOW();
    spi_xfer(FLASH_CMD_RESET_ENABLE);
    FLASH_CS_HIGH();

    FLASH_CS_LOW();
    spi_xfer(FLASH_CMD_RESET);
    FLASH_CS_HIGH();

    /* Wait for flash ready */
    spi_flash_wait_ready(100);

    return 0;
}

/* Read JEDEC ID */
int spi_flash_read_id(uint8_t *manufacturer, uint8_t *device_id) {
    FLASH_CS_LOW();
    spi_xfer(FLASH_CMD_READ_ID);
    *manufacturer = spi_xfer(0xFF);
    *device_id = spi_xfer(0xFF);
    /* Read remaining byte (capacity ID) */
    spi_xfer(0xFF);
    FLASH_CS_HIGH();
    return 0;
}

/* Read data from flash */
int spi_flash_read(uint32_t addr, uint8_t *buf, uint32_t len) {
    FLASH_CS_LOW();
    spi_xfer(FLASH_CMD_READ_DATA);
    spi_xfer((addr >> 16) & 0xFF);
    spi_xfer((addr >> 8) & 0xFF);
    spi_xfer(addr & 0xFF);
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = spi_xfer(0xFF);
    }
    FLASH_CS_HIGH();
    return 0;
}

/* Write enable */
int spi_flash_write_enable(void) {
    FLASH_CS_LOW();
    spi_xfer(FLASH_CMD_WRITE_ENABLE);
    FLASH_CS_HIGH();
    return 0;
}

/* Wait until flash is ready (not busy) */
int spi_flash_wait_ready(uint32_t timeout_ms) {
    uint32_t start = g_uptime_ms;
    while ((g_uptime_ms - start) < timeout_ms) {
        FLASH_CS_LOW();
        spi_xfer(FLASH_CMD_READ_STATUS);
        uint8_t status = spi_xfer(0xFF);
        FLASH_CS_HIGH();
        if (!(status & FLASH_STATUS_BUSY)) {
            return 0;
        }
    }
    return -1;  /* Timeout */
}

/* Program a page (256 bytes max) */
int spi_flash_write(uint32_t addr, const uint8_t *buf, uint32_t len) {
    if (len > FLASH_PAGE_SIZE) return -1;
    if ((addr & 0xFF) + len > FLASH_PAGE_SIZE) return -1;  /* Cross-page */

    spi_flash_write_enable();

    FLASH_CS_LOW();
    spi_xfer(FLASH_CMD_PAGE_PROGRAM);
    spi_xfer((addr >> 16) & 0xFF);
    spi_xfer((addr >> 8) & 0xFF);
    spi_xfer(addr & 0xFF);
    for (uint32_t i = 0; i < len; i++) {
        spi_xfer(buf[i]);
    }
    FLASH_CS_HIGH();

    return spi_flash_wait_ready(100);
}

/* Erase a 4 KB sector */
int spi_flash_erase_sector(uint32_t addr) {
    spi_flash_write_enable();

    FLASH_CS_LOW();
    spi_xfer(FLASH_CMD_SECTOR_ERASE);
    spi_xfer((addr >> 16) & 0xFF);
    spi_xfer((addr >> 8) & 0xFF);
    spi_xfer(addr & 0xFF);
    FLASH_CS_HIGH();

    return spi_flash_wait_ready(1000);  /* Sector erase: up to 1s */
}

/* Erase a 64 KB block */
int spi_flash_erase_block(uint32_t addr) {
    spi_flash_write_enable();

    FLASH_CS_LOW();
    spi_xfer(FLASH_CMD_BLOCK_ERASE);
    spi_xfer((addr >> 16) & 0xFF);
    spi_xfer((addr >> 8) & 0xFF);
    spi_xfer(addr & 0xFF);
    FLASH_CS_HIGH();

    return spi_flash_wait_ready(2000);  /* Block erase: up to 2s */
}

/* Erase entire chip */
int spi_flash_erase_chip(void) {
    spi_flash_write_enable();

    FLASH_CS_LOW();
    spi_xfer(FLASH_CMD_CHIP_ERASE);
    FLASH_CS_HIGH();

    return spi_flash_wait_ready(10000);  /* Chip erase: up to 10s */
}

/* Check if flash is busy */
bool spi_flash_busy(void) {
    FLASH_CS_LOW();
    spi_xfer(FLASH_CMD_READ_STATUS);
    uint8_t status = spi_xfer(0xFF);
    FLASH_CS_HIGH();
    return (status & FLASH_STATUS_BUSY) != 0;
}