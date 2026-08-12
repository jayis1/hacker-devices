/**
 * @file flash_driver.h
 * @brief QSPI NOR Flash driver for bitstream, model, and config storage
 *
 * Manages the W25Q128JVSIQ 16MB QSPI NOR Flash.
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef FLASH_DRIVER_H
#define FLASH_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize QSPI NOR Flash interface
 * @return true if flash is present and responsive
 */
bool flash_init(void);

/**
 * @brief Read data from flash
 * @param addr Flash address (offset from base)
 * @param buffer Output buffer
 * @param length Number of bytes
 * @return true if read succeeded
 */
bool flash_read(uint32_t addr, uint8_t *buffer, uint32_t length);

/**
 * @brief Write data to flash (page program)
 * @param addr Flash address
 * @param data Data to write
 * @param length Number of bytes (auto-handled page boundaries)
 * @return true if write succeeded
 */
bool flash_write(uint32_t addr, const uint8_t *data, uint32_t length);

/**
 * @brief Erase a 4KB sector
 * @param addr Address within the sector to erase
 */
void flash_erase_sector(uint32_t addr);

/**
 * @brief Erase a 32KB block
 */
void flash_erase_block_32k(uint32_t addr);

/**
 * @brief Erase a 64KB block
 */
void flash_erase_block_64k(uint32_t addr);

/**
 * @brief Erase entire chip
 */
void flash_erase_chip(void);

/**
 * @brief Erase a range of flash
 * @param addr Start address
 * @param length Number of bytes to erase
 */
void flash_erase_range(uint32_t addr, uint32_t length);

/**
 * @brief Read flash status register
 */
uint8_t flash_read_status(void);

/**
 * @brief Wait for flash write to complete (busy flag)
 */
void flash_wait_ready(void);

/**
 * @brief Calculate CRC32 over data
 */
uint32_t flash_calculate_crc32(const void *data, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_DRIVER_H */