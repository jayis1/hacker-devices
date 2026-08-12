/**
 * @file fpga_driver.h
 * @brief iCE40UP5K FPGA driver — display bus protocol capture interface
 *
 * Manages the Lattice iCE40UP5K FPGA that handles high-speed display bus
 * capture for MIPI DSI, RGB parallel, SPI LCD, and LVDS protocols.
 *
 * Author: jayis1
 * @copyright Copyright (c) 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef FPGA_DRIVER_H
#define FPGA_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FPGA capture status flags */
#define FPGA_STATUS_CAPTURE_ACTIVE  0x01
#define FPGA_STATUS_FRAME_READY     0x02
#define FPGA_STATUS_LINE_READY      0x04
#define FPGA_STATUS_SYNC_LOCKED     0x08
#define FPGA_STATUS_OVERFLOW        0x10
#define FPGA_STATUS_PROTOCOL_ERROR  0x20
#define FPGA_STATUS_POWER_DOWN      0x40

/* FPGA control flags */
#define FPGA_CTRL_RESET_CAPTURE     0x01
#define FPGA_CTRL_ENABLE_CAPTURE    0x02
#define FPGA_CTRL_DISABLE_CAPTURE   0x04
#define FPGA_CTRL_CLEAR_OVERFLOW    0x08
#define FPGA_CTRL_PASS_THROUGH      0x10

/**
 * @brief Initialize the FPGA driver and SPI interface
 */
void fpga_driver_init(void);

/**
 * @brief Configure FPGA with the specified display protocol bitstream
 * @param protocol Display protocol to configure
 * @return true if configuration succeeded
 */
bool fpga_configure_bitstream(display_protocol_t protocol);

/**
 * @brief Set expected display resolution
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @param color_depth Bits per pixel (16 or 24)
 */
void fpga_set_resolution(uint16_t width, uint16_t height, uint8_t color_depth);

/**
 * @brief Enable or disable pixel capture
 * @param enable true to enable, false to disable
 */
void fpga_enable_capture(bool enable);

/**
 * @brief Check if a complete frame is ready
 * @return true if frame is ready to read
 */
bool fpga_is_frame_ready(void);

/**
 * @brief Read captured frame data from FPGA
 * @param buffer Output buffer for pixel data
 * @param width Frame width
 * @param height Frame height
 * @param color_depth Bits per pixel
 * @return Number of bytes read, 0 on error
 */
uint32_t fpga_read_frame(uint8_t *buffer, uint16_t width, uint16_t height, 
                         uint8_t color_depth);

/**
 * @brief Get FPGA status register
 * @return Status flags
 */
uint8_t fpga_get_status(void);

/**
 * @brief Clear overflow flag
 */
void fpga_clear_overflow(void);

/**
 * @brief Reset FPGA capture state machine
 */
void fpga_reset_capture(void);

/**
 * @brief Put FPGA in power-down mode
 */
void fpga_power_down(void);

/**
 * @brief Wake FPGA from power-down
 */
void fpga_wake_up(void);

/**
 * @brief Reconfigure FPGA (used for error recovery)
 * @param protocol Protocol to configure
 * @return true if reconfiguration succeeded
 */
bool fpga_reconfigure(display_protocol_t protocol);

#ifdef __cplusplus
}
#endif

#endif /* FPGA_DRIVER_H */