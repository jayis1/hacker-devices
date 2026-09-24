/*
 * FlexRay Sentinel FPGA/MCU register contract
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef FLEXRAY_SENTINEL_REGISTERS_H
#define FLEXRAY_SENTINEL_REGISTERS_H

#include <stdint.h>

#define FS_FPGA_ID_VALUE                 0x46525331u
#define FS_REG_ID                        0x0000u
#define FS_REG_CONTROL                   0x0004u
#define FS_REG_STATUS                    0x0008u
#define FS_REG_IRQ_STATUS                0x000Cu
#define FS_REG_IRQ_ENABLE                0x0010u
#define FS_REG_TIME_LOW                  0x0014u
#define FS_REG_TIME_HIGH                 0x0018u
#define FS_REG_FIFO_COUNT                0x001Cu
#define FS_REG_FIFO_DATA                 0x0020u
#define FS_REG_A_EDGE_COUNT              0x0024u
#define FS_REG_B_EDGE_COUNT              0x0028u
#define FS_REG_A_ERROR_COUNT             0x002Cu
#define FS_REG_B_ERROR_COUNT             0x0030u
#define FS_REG_TRIGGER_SLOT              0x0034u
#define FS_REG_TRIGGER_CYCLE             0x0038u
#define FS_REG_TRIGGER_MASK              0x003Cu
#define FS_REG_FILTER_SLOT_MIN            0x0040u
#define FS_REG_FILTER_SLOT_MAX            0x0044u
#define FS_REG_BUILD_DATE                0x0048u
#define FS_REG_SCRATCH                   0x004Cu

#define FS_CTL_CAPTURE_ENABLE            (1u << 0)
#define FS_CTL_FIFO_RESET                (1u << 1)
#define FS_CTL_TIMESTAMP_RESET           (1u << 2)
#define FS_CTL_CHANNEL_A_ENABLE          (1u << 3)
#define FS_CTL_CHANNEL_B_ENABLE          (1u << 4)
#define FS_CTL_RAW_EDGE_ENABLE           (1u << 5)
#define FS_CTL_TRIGGER_ENABLE            (1u << 6)
#define FS_CTL_TEST_PATTERN              (1u << 7)

#define FS_STATUS_PLL_LOCKED             (1u << 0)
#define FS_STATUS_FIFO_EMPTY             (1u << 1)
#define FS_STATUS_FIFO_FULL              (1u << 2)
#define FS_STATUS_CHANNEL_A_ACTIVITY     (1u << 3)
#define FS_STATUS_CHANNEL_B_ACTIVITY     (1u << 4)
#define FS_STATUS_CHANNEL_A_FAULT        (1u << 5)
#define FS_STATUS_CHANNEL_B_FAULT        (1u << 6)
#define FS_STATUS_TRIGGERED              (1u << 7)

#define FS_IRQ_FIFO_WATERMARK            (1u << 0)
#define FS_IRQ_FIFO_OVERFLOW             (1u << 1)
#define FS_IRQ_TRIGGER                   (1u << 2)
#define FS_IRQ_CLOCK_LOSS                (1u << 3)
#define FS_IRQ_CHANNEL_FAULT             (1u << 4)

#define FS_CAPTURE_TYPE_FRAME            0x01u
#define FS_CAPTURE_TYPE_SYMBOL_ERROR     0x02u
#define FS_CAPTURE_TYPE_CYCLE_START      0x03u
#define FS_CAPTURE_TYPE_WAKEUP           0x04u
#define FS_CAPTURE_TYPE_IDLE_GAP         0x05u

#define FS_HEADER_CHANNEL_SHIFT          6u
#define FS_HEADER_TYPE_MASK              0x3Fu
#define FS_FRAME_FLAG_STARTUP            (1u << 0)
#define FS_FRAME_FLAG_SYNC               (1u << 1)
#define FS_FRAME_FLAG_NULL               (1u << 2)
#define FS_FRAME_FLAG_PAYLOAD_PREAMBLE   (1u << 3)
#define FS_FRAME_FLAG_CRC_VALID          (1u << 4)
#define FS_FRAME_FLAG_TRUNCATED          (1u << 5)

#endif
