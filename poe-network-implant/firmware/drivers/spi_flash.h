/*
 * spi_flash.h — W25Q128JVSIQ SPI NOR Flash Driver
 * PhantomBridge PoE Network Implant
 *
 * Provides read, write, erase, and JEDEC ID probe via SPI2.
 * Used for rule storage, configuration, capture logs, and OTA firmware.
 */

#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include <stdint.h>

/* W25Q128JVSIQ Commands */
#define W25Q_CMD_READ          0x03  /* Normal read (up to 50MHz) */
#define W25Q_CMD_FAST_READ     0x0B  /* Fast read (8 dummy clocks) */
#define W25Q_CMD_FAST_READ_DUAL  0x3B  /* Dual output fast read */
#define W25Q_CMD_FAST_READ_QUAD  0x6B  /* Quad output fast read */
#define W25Q_CMD_PAGE_PROGRAM  0x02  /* Page program (256 bytes max) */
#define W25Q_CMD_SECTOR_ERASE  0x20  /* 4KB sector erase */
#define W25Q_CMD_BLOCK_ERASE_32 0x52  /* 32KB block erase */
#define W25Q_CMD_BLOCK_ERASE_64 0xD8  /* 64KB block erase */
#define W25Q_CMD_CHIP_ERASE    0xC7  /* Full chip erase */
#define W25Q_CMD_WRITE_ENABLE  0x06  /* Write enable */
#define W25Q_CMD_WRITE_DISABLE 0x04  /* Write disable */
#define W25Q_CMD_READ_SR1      0x05  /* Read status register 1 */
#define W25Q_CMD_READ_SR2      0x35  /* Read status register 2 */
#define W25Q_CMD_READ_SR3      0x15  /* Read status register 3 */
#define W25Q_CMD_WRITE_SR1     0x01  /* Write status register 1 */
#define W25Q_CMD_WRITE_SR2     0x31  /* Write status register 2 */
#define W25Q_CMD_JEDEC_ID      0x9F  /* Read JEDEC ID */
#define W25Q_CMD_RELEASE_PD    0xAB  /* Release power-down */
#define W25Q_CMD_POWER_DOWN    0xB9  /* Enter power-down */
#define W25Q_CMD_RESET_EN      0x66  /* Reset enable */
#define W25Q_CMD_RESET         0x99  /* Reset device */

/* Status Register 1 bits */
#define W25Q_SR1_BUSY          (1 << 0)  /* Write in progress */
#define W25Q_SR1_WEL           (1 << 1)  /* Write enable latch */
#define W25Q_SR1_BP0           (1 << 2)  /* Block protect 0 */
#define W25Q_SR1_BP1           (1 << 3)  /* Block protect 1 */
#define W25Q_SR1_BP2           (1 << 4)  /* Block protect 2 */
#define W25Q_SR1_TB             (1 << 5)  /* Top/bottom protect */
#define W25Q_SR1_SEC           (1 << 6)  /* Sector protect */
#define W25Q_SR1_SRP0          (1 << 7)  /* Status reg protect 0 */

/* Status Register 2 bits */
#define W25Q_SR2_SRP1          (1 << 0)  /* Status reg protect 1 */
#define W25Q_SR2_QE            (1 << 1)  /* Quad enable */
#define W25Q_SR2_SUS           (1 << 7)  /* Suspend status */

/* Expected JEDEC ID for W25Q128JVSIQ */
#define W25Q_JEDEC_MANUFACTURER  0xEF  /* Winbond */
#define W25Q_JEDEC_TYPE         0x40   /* SPI NOR */
#define W25Q_JEDEC_CAPACITY     0x17   /* 128Mb = 16MB */

/* Flash geometry */
#define W25Q_SECTOR_SIZE       4096    /* 4 KB */
#define W25Q_PAGE_SIZE         256     /* 256 bytes */
#define W25Q_BLOCK_32K         32768   /* 32 KB */
#define W25Q_BLOCK_64K         65536   /* 64 KB */
#define W25Q_TOTAL_SIZE        16777216 /* 16 MB */
#define W25Q_NUM_SECTORS       4096
#define W25Q_NUM_PAGES         65536

/* JEDEC ID structure */
typedef struct {
    uint8_t manufacturer;
    uint8_t type;
    uint8_t capacity;
} w25q_jedec_id_t;

/* ===== Function Prototypes ===== */

/**
 * spi_flash_init — Initialize SPI2 peripheral and probe flash
 * Configures SPI2 in master mode, reads JEDEC ID
 * Returns 0 on success, -1 if flash not detected
 */
int spi_flash_init(void);

/**
 * spi_flash_read_jedec_id — Read flash JEDEC ID
 * @id: Pointer to store ID
 * Returns 0 on success
 */
int spi_flash_read_jedec_id(w25q_jedec_id_t *id);

/**
 * spi_flash_read — Read data from flash
 * @addr: 24-bit start address
 * @buf: Destination buffer
 * @len: Number of bytes to read
 * Returns 0 on success
 */
int spi_flash_read(uint32_t addr, uint8_t *buf, uint32_t len);

/**
 * spi_flash_fast_read — Fast read with dummy cycles
 * @addr: 24-bit start address
 * @buf: Destination buffer
 * @len: Number of bytes to read
 * Returns 0 on success
 */
int spi_flash_fast_read(uint32_t addr, uint8_t *buf, uint32_t len);

/**
 * spi_flash_page_program — Program a 256-byte page
 * @addr: 24-bit start address (must be page-aligned)
 * @data: Source data
 * @len: Number of bytes (1-256, must not cross page boundary)
 * Returns 0 on success
 */
int spi_flash_page_program(uint32_t addr, const uint8_t *data, uint16_t len);

/**
 * spi_flash_write — Write arbitrary data (handles page boundaries)
 * @addr: 24-bit start address
 * @data: Source data
 * @len: Number of bytes
 * Returns 0 on success
 */
int spi_flash_write(uint32_t addr, const uint8_t *data, uint32_t len);

/**
 * spi_flash_sector_erase — Erase a 4KB sector
 * @addr: Address within the sector to erase
 * Returns 0 on success
 */
int spi_flash_sector_erase(uint32_t addr);

/**
 * spi_flash_block_erase_64k — Erase a 64KB block
 * @addr: Address within the block
 * Returns 0 on success
 */
int spi_flash_block_erase_64k(uint32_t addr);

/**
 * spi_flash_chip_erase — Erase entire chip
 * Returns 0 on success
 */
int spi_flash_chip_erase(void);

/**
 * spi_flash_wait_busy — Wait until flash is not busy
 * Returns 0 on success, -1 on timeout
 */
int spi_flash_wait_busy(void);

/**
 * spi_flash_read_status — Read status register 1
 * Returns: Status register value
 */
uint8_t spi_flash_read_status(void);

/**
 * spi_flash_write_enable — Set write enable latch
 * Returns 0 on success
 */
int spi_flash_write_enable(void);

/**
 * spi_flash_power_down — Enter power-down mode
 * Returns 0 on success
 */
int spi_flash_power_down(void);

/**
 * spi_flash_release_power_down — Exit power-down mode
 * Returns 0 on success
 */
int spi_flash_release_power_down(void);

#endif /* SPI_FLASH_H */