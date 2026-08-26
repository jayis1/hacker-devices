/*
 * DP AUX Phantom firmware register map
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef DP_AUX_PHANTOM_REGISTERS_H
#define DP_AUX_PHANTOM_REGISTERS_H

#define REG_DPA_AUX_CONTROL        0x40001000U
#define REG_DPA_AUX_STATUS         0x40001004U
#define REG_DPA_AUX_FIFO           0x40001008U
#define REG_DPA_PD_STATUS          0x40002000U
#define REG_DPA_PD_CONTROL         0x40002004U
#define REG_DPA_HPD_CONTROL        0x40002008U
#define REG_DPA_RADIO_STATUS       0x40003000U
#define REG_DPA_CAPTURE_STATUS     0x40004000U
#define REG_DPA_FPGA_STATUS        0x40005000U
#define REG_DPA_STORAGE_STATUS     0x40006000U

#define DPA_AUX_STATUS_RX_READY    (1U << 0)
#define DPA_AUX_STATUS_TX_READY    (1U << 1)
#define DPA_AUX_STATUS_ERROR       (1U << 2)
#define DPA_PD_STATUS_ATTACHED     (1U << 0)
#define DPA_PD_STATUS_ALT_MODE     (1U << 1)
#define DPA_HPD_CONTROL_ASSERT     (1U << 0)
#define DPA_HPD_CONTROL_PULSE      (1U << 1)

#endif
