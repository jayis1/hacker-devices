/**
 * @file    capture_ctrl.h
 * @brief   Frame capture controller — header
 * @author  jayis1
 */

#ifndef CAPTURE_CTRL_H
#define CAPTURE_CTRL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialize capture controller
 */
void capture_ctrl_init(void);

/**
 * @brief Trigger a single frame capture
 * @return 0 on success, negative error code on failure
 */
int capture_ctrl_trigger(void);

/**
 * @brief Transfer captured frame from FPGA SDRAM to local buffer
 * @return 0 on success, negative on failure
 */
int capture_ctrl_transfer_frame(void);

/**
 * @brief Get pointer to most recently captured frame
 * @param size Output: frame size in bytes
 * @return Pointer to frame buffer, or NULL if no frame available
 */
const uint8_t *capture_ctrl_get_frame(size_t *size);

/**
 * @brief Release the frame buffer (mark as available for next capture)
 */
void capture_ctrl_release_frame(void);

/**
 * @brief Check if capture hardware is ready
 * @return true if HDMI locked and FPGA ready
 */
bool capture_ctrl_is_ready(void);

/**
 * @brief Check if a capture is in progress
 * @return true if busy
 */
bool capture_ctrl_is_busy(void);

/**
 * @brief Get total frames captured this session
 * @return Frame count
 */
uint32_t capture_ctrl_get_frame_count(void);

/**
 * @brief FreeRTOS task for capture management
 */
void capture_task(void *pvParameters);

#endif /* CAPTURE_CTRL_H */
