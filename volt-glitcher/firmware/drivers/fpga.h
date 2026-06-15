/*
 * drivers/fpga.h — Lattice iCE40HX1K FPGA driver interface
 * SPI-based configuration and register access
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

#ifndef DRIVERS_FPGA_H
#define DRIVERS_FPGA_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * FPGA Configuration Constants
 * ======================================================================== */

/* iCE40 configuration via SPI: bitstream format requires
 * - 8 dummy clocks before first data
 * - Bitstream data MSB first
 * - 49+ dummy clocks after last data byte
 * - CDONE goes high when configuration succeeds */

#define FPGA_CFG_DUMMY_PREFIX    8U      /* Dummy bytes before bitstream */
#define FPGA_CFG_DUMMY_SUFFIX    50U    /* Dummy bytes after bitstream */
#define FPGA_CFG_TIMEOUT_MS      500U   /* Config timeout */
#define FPGA_CDONE_POLL_MS       1U     /* Poll interval for CDONE */

/* SPI command format: first byte = [R/W# | ADDR5:0 | DONTCARE]
 * R/W# = 0: write, 1: read
 * Following bytes = data (16-bit for writes, response for reads) */
#define FPGA_SPI_CMD_WRITE       0x00
#define FPGA_SPI_CMD_READ        0x80
#define FPGA_SPI_CMD_ADDR_SHIFT  1U

/* Bitstream storage in external flash (W25Q128 at SPI2) */
#define FPGA_BIT_FLASH_ADDR      0x000000UL   /* Start of bitstream in flash */
#define FPGA_BIT_MAX_SIZE        (128U * 1024UL)  /* 128 KB max for HX1K */

/* ========================================================================
 * FPGA register access (address definitions in registers.h)
 * ======================================================================== */

/* Read a 16-bit register from FPGA via SPI */
uint16_t fpga_read_reg(uint8_t addr);

/* Write a 16-bit register to FPGA via SPI */
void fpga_write_reg(uint8_t addr, uint16_t value);

/* Read multiple consecutive registers */
void fpga_read_reg_burst(uint8_t start_addr, uint16_t *buf, uint8_t count);

/* Write multiple consecutive registers */
void fpga_write_reg_burst(uint8_t start_addr, const uint16_t *buf, uint8_t count);

/* ========================================================================
 * FPGA Configuration / Lifecycle
 * ======================================================================== */

/* Configure FPGA from external flash bitstream.
 * Returns 0 on success, negative on error. */
int fpga_configure(void);

/* Configure FPGA from a memory buffer instead of flash.
 * Used for USB-driven bitstream updates. */
int fpga_configure_from_buf(const uint8_t *bitstream, uint32_t length);

/* Reset FPGA (assert CRESET_B, wait, release) */
void fpga_reset(void);

/* Check if FPGA is configured (CDONE high) */
int fpga_is_configured(void);

/* Put FPGA to sleep (reduce power) */
void fpga_sleep(void);

/* Wake FPGA from sleep */
void fpga_wake(void);

/* Suspend FPGA (freeze PLL, gate clocks) */
void fpga_suspend(void);

/* Resume from suspend */
void fpga_resume(void);

/* ========================================================================
 * FPGA Trigger Pattern Interface
 * ======================================================================== */

/* Feed a UART byte to the FPGA pattern matcher.
 * The FPGA processes the byte through its shift-register comparator
 * and fires a trigger event when the configured pattern matches. */
void fpga_feed_uart_trigger(uint8_t byte);

/* Set the UART trigger pattern in FPGA registers.
 * pattern: 4 bytes to match, mask: which bytes to compare */
void fpga_set_uart_pattern(const uint8_t pattern[4], uint8_t mask);

/* Set the JTAG TAP state to trigger on */
void fpga_set_jtag_trigger(uint8_t tap_state);

/* Set GPIO trigger edge configuration */
void fpga_set_gpio_trigger(uint8_t edge_config);

/* ========================================================================
 * FPGA Waveform RAM Interface
 * ======================================================================== */

/* Load waveform samples into FPGA BRAM.
 * samples: array of 16-bit amplitude values
 * count: number of samples (max 1024)
 * Returns 0 on success. */
int fpga_load_waveform(const uint16_t *samples, uint16_t count);

/* Start waveform playback.
 * loop: if nonzero, repeat waveform continuously
 * trigger: if nonzero, wait for trigger before starting */
void fpga_start_waveform(int loop, int trigger);

/* Stop waveform playback */
void fpga_stop_waveform(void);

/* ========================================================================
 * FPGA PLL Reconfiguration
 * ======================================================================== */

/* Reconfigure FPGA PLL for different glitch timing resolution.
 * div_int: integer divisor (1-128)
 * div_frac: fractional divisor (0-9999, in 1/10000 units)
 * Returns 0 on success. */
int fpga_pll_reconfig(uint16_t div_int, uint16_t div_frac);

/* Get current PLL lock status */
int fpga_pll_locked(void);

/* ========================================================================
 * FPGA Status & Diagnostics
 * ======================================================================== */

/* Read FPGA temperature (if supported by bitstream) */
int8_t fpga_read_temperature(void);

/* Get FPGA version register */
uint16_t fpga_get_version(void);

/* Get full status register */
uint16_t fpga_get_status(void);

/* Get glitch fire counter from FPGA */
uint32_t fpga_get_glitch_count(void);

/* Get last trigger timestamp from FPGA */
uint32_t fpga_get_last_timestamp(void);

/* ========================================================================
 * FPGA Fan Control
 * ======================================================================== */

/* Set fan PWM speed (0=off, 255=max) */
void fpga_set_fan_speed(uint8_t speed);

/* Auto-regulate fan based on temperature */
void fpga_fan_auto(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_FPGA_H */