/*
 * EtherCAT Phantom Register Definitions (simulation-friendly)
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef ETHERCAT_PHANTOM_REGISTERS_H
#define ETHERCAT_PHANTOM_REGISTERS_H

#include <stdint.h>

#define REG32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))

#define EPH_REG_BASE           0x50000000u
#define EPH_REG_FPGA_CTRL      (EPH_REG_BASE + 0x0000u)
#define EPH_REG_FPGA_STATUS    (EPH_REG_BASE + 0x0004u)
#define EPH_REG_FPGA_MAILBOX   (EPH_REG_BASE + 0x0008u)
#define EPH_REG_PHY0_STATUS    (EPH_REG_BASE + 0x0010u)
#define EPH_REG_PHY1_STATUS    (EPH_REG_BASE + 0x0014u)
#define EPH_REG_POWER_STATUS   (EPH_REG_BASE + 0x0020u)
#define EPH_REG_RADIO_STATUS   (EPH_REG_BASE + 0x0030u)
#define EPH_REG_STORAGE_STATUS (EPH_REG_BASE + 0x0040u)

#define FPGA_CTRL_RESET        (1u << 0)
#define FPGA_CTRL_STREAM_EN    (1u << 1)
#define FPGA_CTRL_INJECT_EN    (1u << 2)
#define FPGA_CTRL_CAPTURE_EN   (1u << 3)

#define FPGA_STATUS_READY      (1u << 0)
#define FPGA_STATUS_LINK_UP0   (1u << 1)
#define FPGA_STATUS_LINK_UP1   (1u << 2)
#define FPGA_STATUS_FIFO_FULL  (1u << 3)
#define FPGA_STATUS_SAFE_BYPASS (1u << 4)

#define POWER_STATUS_USB       (1u << 0)
#define POWER_STATUS_BATTERY   (1u << 1)
#define POWER_STATUS_CHARGING  (1u << 2)
#define POWER_STATUS_LOW       (1u << 3)
#define POWER_STATUS_CRITICAL  (1u << 4)

#define RADIO_STATUS_CONNECTED (1u << 0)
#define RADIO_STATUS_STREAMING (1u << 1)
#define RADIO_STATUS_PAIRING   (1u << 2)

#define STORAGE_STATUS_MOUNTED (1u << 0)
#define STORAGE_STATUS_BUSY    (1u << 1)
#define STORAGE_STATUS_FAULT   (1u << 2)

#endif
