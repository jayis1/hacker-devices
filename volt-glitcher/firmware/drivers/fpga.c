#include "fpga.h"
#include "../board.h"
#include "../registers.h"

void fpga_init(void) {
    // Initialize SPI/FMC for FPGA communication
    // Set CS High
}

void fpga_load_bitstream(const uint8_t *data, uint32_t size) {
    // Toggle FPGA_CRESET_B
    // Send bitstream via SPI
    // Wait for FPGA_CDONE
}

void fpga_write_reg(uint32_t reg, uint32_t val) {
    volatile uint32_t *addr = (uint32_t *)(FPGA_BASE_ADDR + reg);
    *addr = val;
}

uint32_t fpga_read_reg(uint32_t reg) {
    volatile uint32_t *addr = (uint32_t *)(FPGA_BASE_ADDR + reg);
    return *addr;
}
