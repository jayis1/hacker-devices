/*
 * spi_flash.h — SPI Flash (MX25LW1636) Driver for Substation
 */

#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include <stdint.h>
#include <stdbool.h>

/* Flash commands */
#define FLASH_CMD_WRITE_ENABLE      0x06
#define FLASH_CMD_WRITE_DISABLE     0x04
#define FLASH_CMD_READ_STATUS       0x05
#define FLASH_CMD_WRITE_STATUS      0x01
#define FLASH_CMD_READ_DATA         0x03
#define FLASH_CMD_FAST_READ         0x0B
#define FLASH_CMD_QUAD_READ         0x6B
#define FLASH_CMD_PAGE_PROGRAM      0x02
#define FLASH_CMD_SECTOR_ERASE      0x20
#define FLASH_CMD_BLOCK_ERASE       0xD8
#define FLASH_CMD_CHIP_ERASE        0xC7
#define FLASH_CMD_READ_ID           0x9F
#define FLASH_CMD_READ_SFDP         0x5A
#define FLASH_CMD_ENTER_QPI         0x38
#define FLASH_CMD_EXIT_QPI          0xFF
#define FLASH_CMD_RESET_ENABLE      0x66
#define FLASH_CMD_RESET             0x99

/* Status register bits */
#define FLASH_STATUS_BUSY           (1 << 0)
#define FLASH_STATUS_WEL           (1 << 1)
#define FLASH_STATUS_BP0            (1 << 2)
#define FLASH_STATUS_BP1            (1 << 3)
#define FLASH_STATUS_BP2            (1 << 4)
#define FLASH_STATUS_QE             (1 << 5)

/* Flash geometry */
#define FLASH_PAGE_SIZE             256
#define FLASH_SECTOR_SIZE           4096
#define FLASH_BLOCK_SIZE            65536
#define FLASH_TOTAL_SIZE            (16 * 1024 * 1024)  /* 16 MB */
#define FLASH_SECTOR_COUNT          (FLASH_TOTAL_SIZE / FLASH_SECTOR_SIZE)

/* Partitions (offsets) */
#define FLASH_FW_PARTITION         0x00000000
#define FLASH_CFG_PARTITION         0x000F0000
#define FLASH_PCAP_PARTITION        0x000F8000

int  spi_flash_init(void);
int  spi_flash_read_id(uint8_t *manufacturer, uint8_t *device_id);
int  spi_flash_read(uint32_t addr, uint8_t *buf, uint32_t len);
int  spi_flash_write(uint32_t addr, const uint8_t *buf, uint32_t len);
int  spi_flash_erase_sector(uint32_t addr);
int  spi_flash_erase_block(uint32_t addr);
int  spi_flash_erase_chip(void);
bool spi_flash_busy(void);
int  spi_flash_write_enable(void);
int  spi_flash_wait_ready(uint32_t timeout_ms);

#endif /* SPI_FLASH_H */