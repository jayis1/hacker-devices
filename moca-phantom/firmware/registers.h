/*
 * MoCA Phantom Register Map Model
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef MPH_REGISTERS_H
#define MPH_REGISTERS_H

#include <stdint.h>

#define MPH_REG_PHY_A_STATUS        0x0000u
#define MPH_REG_PHY_B_STATUS        0x0004u
#define MPH_REG_BYPASS_CONTROL      0x0008u
#define MPH_REG_FPGA_STATUS         0x000Cu
#define MPH_REG_FPGA_TIMING_BUDGET  0x0010u
#define MPH_REG_RADIO_STATUS        0x0014u
#define MPH_REG_BATTERY_STATUS      0x0018u
#define MPH_REG_SPECTRUM_STATUS     0x001Cu
#define MPH_REG_CAPTURE_HEAD        0x0020u
#define MPH_REG_CAPTURE_COUNT       0x0024u

#define MPH_PHY_LINK_UP             (1u << 0)
#define MPH_PHY_ACTIVITY            (1u << 1)
#define MPH_PHY_PRIVACY_LOCK        (1u << 2)

#define MPH_BYPASS_FORCE_ENABLE     (1u << 0)
#define MPH_BYPASS_SAFE_ASSERT      (1u << 1)

#define MPH_FPGA_READY              (1u << 0)
#define MPH_FPGA_TIMING_OK          (1u << 1)
#define MPH_FPGA_REWRITE_ACTIVE     (1u << 2)
#define MPH_FPGA_GUARD_TRIPPED      (1u << 3)

#define MPH_RADIO_BLE_CONNECTED     (1u << 0)
#define MPH_RADIO_WIFI_CONNECTED    (1u << 1)

static inline uint32_t mph_reg_mask(uint32_t base, uint32_t mask) {
    return base | mask;
}

#endif
