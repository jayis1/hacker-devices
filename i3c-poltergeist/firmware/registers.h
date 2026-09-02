/*
 * registers.h - I3C Poltergeist constants and command IDs
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef I3C_POLTERGEIST_REGISTERS_H
#define I3C_POLTERGEIST_REGISTERS_H

#define IP_CCC_ENTDAA            0x0007u
#define IP_CCC_RSTDAA            0x0006u
#define IP_CCC_ENEC              0x0000u
#define IP_CCC_DISEC             0x0001u
#define IP_CCC_SETMWL            0x0009u
#define IP_CCC_GETMWL            0x0089u
#define IP_CCC_SETMRL            0x000Au
#define IP_CCC_GETMRL            0x008Au
#define IP_CCC_GETPID            0x008Du
#define IP_CCC_GETBCR            0x008Eu
#define IP_CCC_GETDCR            0x008Fu
#define IP_CCC_SETDASA           0x0087u
#define IP_CCC_SETAASA           0x0029u
#define IP_CCC_DEFSLVS           0x008Bu

#define IP_REG_DEVICE_ID         0x0000u
#define IP_REG_STATUS            0x0001u
#define IP_REG_POWER             0x0002u
#define IP_REG_INT_MASK          0x0003u
#define IP_REG_INT_STATUS        0x0004u
#define IP_REG_SENSOR_VECTOR     0x0010u
#define IP_REG_FW_REV            0x0011u
#define IP_REG_MAILBOX           0x0020u
#define IP_REG_DEBUG_WINDOW      0x00F0u

#define IP_TRIGGER_ANY_CCC       0xFFFFu
#define IP_TARGET_ANY            0xFFu

#define IP_CMD_STAGE_PROFILE     0x10u
#define IP_CMD_ARM               0x11u
#define IP_CMD_INJECT_HOTJOIN    0x12u
#define IP_CMD_REPLAY_IBI        0x13u
#define IP_CMD_SUPPRESS_CCC      0x14u
#define IP_CMD_EXPORT            0x15u
#define IP_CMD_BYPASS            0x16u

#endif
