/**
 * @file psram_driver.h
 * @brief QSPI PSRAM driver for frame buffer storage
 *
 * Manages the APS6404L-3SQR-SN 8MB QSPI PSRAM used for frame buffers,
 * delta computation, OCR text storage, and frame queue.
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef PSRAM_DRIVER_H
#define PSRAM_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize QSPI PSRAM interface
 * @return true if PSRAM responded correctly
 */
bool psram_init(void);

/**
 * @brief Read data from PSRAM
 * @param addr PSRAM address (0 to PSRAM_SIZE-1)
 * @param buffer Output buffer
 * @param length Number of bytes to read
 * @return true if read succeeded
 */
bool psram_read(uint32_t addr, uint8_t *buffer, uint32_t length);

/**
 * @brief Write data to PSRAM
 * @param addr PSRAM address (0 to PSRAM_SIZE-1)
 * @param data Data to write
 * @param length Number of bytes
 * @return true if write succeeded
 */
bool psram_write(uint32_t addr, const uint8_t *data, uint32_t length);

/**
 * @brief Erase a region (sets to 0x00 for PSRAM)
 * @param addr Start address
 * @param length Number of bytes
 */
void psram_erase(uint32_t addr, uint32_t length);

/**
 * @brief Test PSRAM integrity
 * @return true if test passed
 */
bool psram_test(void);

#ifdef __cplusplus
}
#endif

#endif /* PSRAM_DRIVER_H */