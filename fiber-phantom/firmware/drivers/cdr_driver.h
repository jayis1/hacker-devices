/*
 * cdr_driver.h — FPGA SPI interface for Clock/Data Recovery and frame access
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Provides the SPI interface between the RP2040 and the iCE40 UP5K FPGA.
 * The FPGA performs CDR, Ethernet/Fibre Channel framing, MITM rule evaluation,
 * and PCAP frame buffering. This driver handles:
 *  - FPGA bitstream configuration (cold boot)
 *  - FPGA register read/write over SPI
 *  - Frame data read from FPGA FIFO
 *  - MITM rule programming
 */

#ifndef FIBER_PHANTOM_CDR_DRIVER_H
#define FIBER_PHANTOM_CDR_DRIVER_H

#include <stdint.h>
#include "board.h"

/* Initialize SPI0 for FPGA communication */
void cdr_spi_init(void);

/* Configure FPGA with bitstream from external SPI flash (W25Q128).
 * The iCE40 configures from a dedicated flash on power-up; this function
 * verifies the CDONE pin and forces a re-configuration if needed.
 * Returns 0 on success, negative on error. */
int fpga_configure(void);

/* Check if FPGA is configured (CDONE asserted) */
uint8_t fpga_is_ready(void);

/* Read a 32-bit FPGA register */
uint32_t fpga_reg_read(uint8_t reg);

/* Write a 32-bit FPGA register */
void fpga_reg_write(uint8_t reg, uint32_t value);

/* Read frame data from FPGA FIFO into buffer.
 * Returns number of bytes read (0 if FIFO empty).
 * Also fills in timestamp and frame length via output params. */
uint32_t fpga_read_frame(uint8_t *buf, uint32_t max_len,
                         uint32_t *timestamp_lo, uint32_t *timestamp_hi,
                         uint16_t *frame_len);

/* Write a MITM rule to FPGA rule RAM.
 * index: rule slot (0 to MITM_RULE_MAX-1)
 * Returns 0 on success. */
int fpga_write_rule(uint8_t index, const mitm_rule_t *rule);

/* Clear (disable) a MITM rule slot */
int fpga_clear_rule(uint8_t index);

/* Clear all MITM rules */
void fpga_clear_all_rules(void);

/* Enable/disable MITM engine */
void fpga_mitm_enable(uint8_t enable);

/* Enable/disable VCSEL injection path */
void fpga_inject_enable(uint8_t enable);

/* Enable/disable frame capture */
void fpga_capture_enable(uint8_t enable);

/* Reset all FPGA statistics counters */
void fpga_reset_stats(void);

/* Get FPGA status register (see FPGA_STATUS_* bits) */
uint32_t fpga_get_status(void);

/* Get detected link rate from FPGA */
link_rate_t fpga_get_link_rate(void);

/* Read FPGA statistics counters */
void fpga_get_stats(uint32_t *frame_cnt_lo, uint32_t *frame_cnt_hi,
                    uint32_t *drop_cnt, uint32_t *mitm_modified,
                    uint32_t *mitm_dropped, uint32_t *crc_errors);

#endif /* FIBER_PHANTOM_CDR_DRIVER_H */