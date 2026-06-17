/**
 * @file    spi_fpga.h
 * @brief   SPI master driver for FPGA register access — header
 * @author  jayis1
 */

#ifndef SPI_FPGA_H
#define SPI_FPGA_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Initialize SPI master for FPGA communication
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t spi_fpga_init(void);

/**
 * @brief Reset the SPI-FPGA interface
 * @return ESP_OK on success
 */
esp_err_t spi_fpga_reset(void);

#endif /* SPI_FPGA_H */
