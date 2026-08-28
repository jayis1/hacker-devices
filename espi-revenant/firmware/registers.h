/*
 * registers.h - simulated register map for eSPI Revenant
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef ESPI_REVENANT_REGISTERS_H
#define ESPI_REVENANT_REGISTERS_H

#define ER_REG_SYS_STATUS          0x0000u
#define ER_REG_SYS_CONTROL         0x0004u
#define ER_REG_TRACE_CONTROL       0x0010u
#define ER_REG_TRACE_COUNT         0x0014u
#define ER_REG_VWIRE_STATUS        0x0020u
#define ER_REG_FLASH_DELAY_NS      0x0030u
#define ER_REG_ALERT_CONTROL       0x0034u
#define ER_REG_PERIPH_REPLAY       0x0040u
#define ER_REG_TEMP_MILLIC         0x0050u
#define ER_REG_CURRENT_MA          0x0054u
#define ER_REG_TARGET_MV           0x0058u
#define ER_REG_WATCHDOG_MS         0x0060u
#define ER_REG_PROFILE_HASH        0x0070u
#define ER_REG_LAST_ANOMALY        0x0074u

#define ER_SYS_ARMED               (1u << 0)
#define ER_SYS_BYPASS              (1u << 1)
#define ER_SYS_TARGET_PRESENT      (1u << 2)
#define ER_SYS_WATCHDOG_OK         (1u << 3)

#define ER_TRACE_ENABLE            (1u << 0)
#define ER_TRACE_DECODE            (1u << 1)
#define ER_TRACE_CORRELATE_POWER   (1u << 2)

#define ER_VWIRE_SLP_S3            (1u << 0)
#define ER_VWIRE_SLP_S4            (1u << 1)
#define ER_VWIRE_SLP_S5            (1u << 2)
#define ER_VWIRE_SUS_WARN          (1u << 3)
#define ER_VWIRE_HOST_RST_WARN     (1u << 4)
#define ER_VWIRE_OOB_RST_WARN      (1u << 5)

#endif
