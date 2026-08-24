/*
 * nac-mirage simulated register map
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef NAC_MIRAGE_REGISTERS_H
#define NAC_MIRAGE_REGISTERS_H

#define REG_BRIDGE_CONTROL       0x40001000U
#define REG_BRIDGE_STATUS        0x40001004U
#define REG_POE_CONTROL          0x40002000U
#define REG_POE_STATUS           0x40002004U
#define REG_LLDP_RX              0x40003000U
#define REG_LLDP_TX              0x40003004U
#define REG_CAPTURE_BASE         0x40004000U
#define REG_RADIO_STATUS         0x40005000U
#define REG_RADIO_TX             0x40005004U
#define REG_WATCHDOG             0x40006000U
#define REG_POWER_GATE           0x40007000U

#define BRIDGE_CTRL_BYPASS       (1U << 0)
#define BRIDGE_CTRL_RELAY        (1U << 1)
#define BRIDGE_CTRL_MIRROR       (1U << 2)
#define BRIDGE_CTRL_MUTATE       (1U << 3)

#define POE_CTRL_ENABLE          (1U << 0)
#define POE_CTRL_GLITCH          (1U << 1)
#define POE_CTRL_LIMIT           (1U << 2)

#define RADIO_STATUS_LINK        (1U << 0)
#define RADIO_STATUS_PAIRING     (1U << 1)

#endif
