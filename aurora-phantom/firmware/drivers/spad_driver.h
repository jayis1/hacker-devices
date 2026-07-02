/*
 * spad_driver.h — SPAD focal-array control interface
 * Author:  jayis1   License: GPL-2.0
 *
 * The 16x16 SPAD array + on-die TDC are controlled through the FPGA
 * register file (see registers.h). This driver wraps those accesses and
 * manages bias trim, gating, and per-pixel rate readback.
 */
#ifndef AURORA_SPAD_DRIVER_H
#define AURORA_SPAD_DRIVER_H
#include <stdint.h>
#include <stdbool.h>

int      spad_init(void);
void     spad_set_bias(uint16_t trim);
uint16_t spad_get_bias(void);
void     spad_set_tdc_resolution(uint8_t res);   /* 0=1ns 1=0.5ns 2=0.25ns */
void     spad_enable_all_pixels(bool on);
void     spad_enable_pixel(uint8_t idx, bool on);  /* idx 0..255 */
uint32_t spad_get_rate_pixel(uint8_t idx);          /* Hz */
uint32_t spad_get_rate_aggregate(void);             /* Hz, all pixels summed */
uint32_t spad_get_saturated_mask(uint32_t mask[2]); /* returns count */

#endif