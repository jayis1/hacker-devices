/*
 * Type-C Specter simulated register map
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef TYPEC_SPECTER_REGISTERS_H
#define TYPEC_SPECTER_REGISTERS_H

#include <stdint.h>

#define TS_REG_BASE_SYSTEM 0x40000000u
#define TS_REG_BASE_PD_A   0x40001000u
#define TS_REG_BASE_PD_B   0x40002000u
#define TS_REG_BASE_POWER  0x40003000u
#define TS_REG_BASE_MUX    0x40004000u
#define TS_REG_BASE_RADIO  0x40005000u

#define TS_SYS_CTRL        (TS_REG_BASE_SYSTEM + 0x00u)
#define TS_SYS_STATUS      (TS_REG_BASE_SYSTEM + 0x04u)
#define TS_PD_CTRL(side)   ((side) == 0 ? TS_REG_BASE_PD_A + 0x00u : TS_REG_BASE_PD_B + 0x00u)
#define TS_PD_STATUS(side) ((side) == 0 ? TS_REG_BASE_PD_A + 0x04u : TS_REG_BASE_PD_B + 0x04u)
#define TS_POWER_CTRL      (TS_REG_BASE_POWER + 0x00u)
#define TS_POWER_TEMP      (TS_REG_BASE_POWER + 0x14u)
#define TS_MUX_CTRL        (TS_REG_BASE_MUX + 0x00u)
#define TS_RADIO_CTRL      (TS_REG_BASE_RADIO + 0x00u)

#define TS_SYS_CTRL_ENABLE    (1u << 0)
#define TS_SYS_CTRL_SAFE_MODE (1u << 1)
#define TS_SYS_CTRL_ARMED     (1u << 2)

#define TS_PD_CTRL_ENABLE     (1u << 0)
#define TS_PD_CTRL_PROXY      (1u << 1)
#define TS_PD_CTRL_MUTATE     (1u << 2)
#define TS_PD_CTRL_HARD_RESET (1u << 3)

#define TS_POWER_CTRL_ENABLE    (1u << 0)
#define TS_POWER_CTRL_DISCHARGE (1u << 1)
#define TS_POWER_CTRL_LIMITED   (1u << 2)

#define TS_MUX_CTRL_USB2_PASS (1u << 0)
#define TS_MUX_CTRL_USB2_ISO  (1u << 1)
#define TS_MUX_CTRL_SBU_PASS  (1u << 2)
#define TS_MUX_CTRL_SBU_ISO   (1u << 3)

#define TS_RADIO_CTRL_ENABLE (1u << 0)
#define TS_RADIO_CTRL_ADVERT (1u << 1)

#endif
