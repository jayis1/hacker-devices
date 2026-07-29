/*
 * drivers/fpga_spi.h — ECP5 FPGA SPI interface header
 *
 * Author:  jayis1
 * License: GPL-2.0
 */

#ifndef NVME_PHANTOM_FPGA_SPI_H
#define NVME_PHANTOM_FPGA_SPI_H

#include <stdint.h>

int fpga_load_bitstream(const uint8_t *bitstream, uint32_t len);
int fpga_load_from_nor(uint32_t offset);

#endif /* NVME_PHANTOM_FPGA_SPI_H */