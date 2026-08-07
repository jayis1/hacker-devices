/*
 * drivers/fpga_if.h — FPGA Interface for Prism-Tap
 * SPI communication, bitstream loading, and register access for
 * the Lattice iCE40-UP5K FPGA.
 *
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef PRISM_TAP_FPGA_IF_H
#define PRISM_TAP_FPGA_IF_H

#include <stdint.h>
#include "board.h"

/* ---- FPGA SPI access ---- */
int  fpga_init(void);
int  fpga_load_bitstream(const uint8_t *bitstream, uint32_t length);
int  fpga_load_from_nor(void);
int  fpga_is_ready(void);

/* Register access (16-bit addressed, 16-bit data) */
uint16_t fpga_read_reg(uint16_t addr);
void     fpga_write_reg(uint16_t addr, uint16_t data);

/* DMA frame buffer read (reads one full frame buffer) */
int fpga_read_frame_dma(uint16_t buf_sel, uint8_t *dest, uint32_t max_bytes,
                        uint32_t *actual_bytes);

/* Status helpers */
uint16_t fpga_get_status(void);
uint8_t  fpga_csi_link_up(void);
uint8_t  fpga_dsi_link_up(void);
uint32_t fpga_get_frame_index(void);

/* Control helpers */
void fpga_enable_tap(uint8_t enable);
void fpga_enable_capture(uint8_t enable);
void fpga_enable_inject(uint8_t enable);
void fpga_reset(void);

/* Injection frame upload */
int fpga_upload_inject_frame(const uint8_t *frame_data, uint32_t length,
                              uint16_t width, uint16_t height, uint8_t format);

/* Timing control */
void fpga_set_timing_delay(int32_t delay_ns);
void fpga_set_timing_jitter(uint8_t jitter_pct);
void fpga_set_timing_drop_pattern(uint8_t pattern);
void fpga_enable_timing(uint8_t enable);

/* Error statistics */
typedef struct {
    uint32_t crc_errors;
    uint32_t overflows;
    uint32_t short_packets;
    uint32_t long_packets;
} fpga_error_stats_t;

void fpga_get_error_stats(fpga_error_stats_t *stats);

#endif /* PRISM_TAP_FPGA_IF_H */

/* ---- End of fpga_if.h ----
 * Author: jayis1
 */