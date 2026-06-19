/**
 * drivers/fpga.h — iCE40UP5K SPI command interface
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PHOTONSTRIKE_FPGA_H
#define PHOTONSTRIKE_FPGA_H

#include <stdint.h>
#include <stdbool.h>

void    fpga_init(void);
bool    fpga_ready(void);
void    fpga_cmd(uint8_t op, uint32_t a, uint32_t b, uint32_t c);
uint32_t fpga_read_status(void);
uint32_t fpga_read_clk(void);

#endif