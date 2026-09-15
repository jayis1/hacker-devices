/*
 * SDIO Sentinel FPGA register contract
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef SDIO_SENTINEL_REGISTERS_H
#define SDIO_SENTINEL_REGISTERS_H

#include <stdint.h>

#define SS_FPGA_ID_EXPECTED 0x5344494Fu
#define SS_FPGA_ABI_VERSION 0x00010000u

/* 32-bit little-endian registers exposed over the MCU-to-FPGA SPI link. */
enum ss_fpga_register {
    FPGA_REG_ID             = 0x00u,
    FPGA_REG_ABI            = 0x04u,
    FPGA_REG_CONTROL        = 0x08u,
    FPGA_REG_STATUS         = 0x0Cu,
    FPGA_REG_HOST_CLOCK_HZ  = 0x10u,
    FPGA_REG_FRAME_COUNT    = 0x14u,
    FPGA_REG_CRC_ERROR_COUNT= 0x18u,
    FPGA_REG_TIMEOUT_COUNT  = 0x1Cu,
    FPGA_REG_FIFO_LEVEL     = 0x20u,
    FPGA_REG_FRAME_META     = 0x24u,
    FPGA_REG_FRAME_DATA     = 0x28u,
    FPGA_REG_RULE_INDEX     = 0x40u,
    FPGA_REG_RULE_MATCH     = 0x44u,
    FPGA_REG_RULE_MASK      = 0x48u,
    FPGA_REG_RULE_ACTION    = 0x4Cu,
    FPGA_REG_RULE_COMMIT    = 0x50u,
    FPGA_REG_BYPASS_REASON  = 0x54u,
    FPGA_REG_HEARTBEAT      = 0x58u,
    FPGA_REG_SCRATCH        = 0x5Cu
};

#define FPGA_CTRL_CAPTURE_ENABLE  (1u << 0)
#define FPGA_CTRL_POLICY_ENABLE   (1u << 1)
#define FPGA_CTRL_CLEAR_COUNTERS  (1u << 2)
#define FPGA_CTRL_FIFO_RESET      (1u << 3)
#define FPGA_CTRL_FAIL_OPEN       (1u << 4)
#define FPGA_CTRL_CARD_POWER      (1u << 5)
#define FPGA_CTRL_TEST_PATTERN    (1u << 6)

#define FPGA_STATUS_PLL_LOCKED    (1u << 0)
#define FPGA_STATUS_HOST_PRESENT  (1u << 1)
#define FPGA_STATUS_CARD_PRESENT  (1u << 2)
#define FPGA_STATUS_FIFO_NOT_EMPTY (1u << 3)
#define FPGA_STATUS_FIFO_OVERFLOW (1u << 4)
#define FPGA_STATUS_CMD_CONTENTION (1u << 5)
#define FPGA_STATUS_DATA_CONTENTION (1u << 6)
#define FPGA_STATUS_THERMAL_ALERT (1u << 7)
#define FPGA_STATUS_BYPASS_ACTIVE (1u << 8)

#define FPGA_META_LENGTH_MASK     0x0000007Fu
#define FPGA_META_DIRECTION       (1u << 8)
#define FPGA_META_CRC_VALID       (1u << 9)
#define FPGA_META_TIMEOUT         (1u << 10)
#define FPGA_META_CLOCK_EDGE      (1u << 11)
#define FPGA_META_TIMESTAMP_SHIFT 16u

#define FPGA_RULE_ALLOW           0u
#define FPGA_RULE_LOG             1u
#define FPGA_RULE_BLOCK           2u
#define FPGA_RULE_REQUIRE_ARM     3u

#endif
