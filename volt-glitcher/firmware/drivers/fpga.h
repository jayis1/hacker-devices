#ifndef FPGA_H
#define FPGA_H

#include <stdint.h>

void fpga_init(void);
void fpga_load_bitstream(const uint8_t *data, uint32_t size);
void fpga_write_reg(uint32_t reg, uint32_t val);
uint32_t fpga_read_reg(uint32_t reg);

#endif // FPGA_H
