/*
 * OneWire Cartographer register and protocol map
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef OWC_REGISTERS_H
#define OWC_REGISTERS_H

#include <stdint.h>

#define OWC_USB_MAGIC              0x4F574331u /* OWC1 */
#define OWC_PROTOCOL_VERSION       1u

/* Host commands. Mutating commands require both session unlock and ARM. */
typedef enum {
    OWC_CMD_GET_INFO       = 0x01,
    OWC_CMD_GET_STATUS     = 0x02,
    OWC_CMD_CAPTURE_START  = 0x10,
    OWC_CMD_CAPTURE_STOP   = 0x11,
    OWC_CMD_CAPTURE_READ   = 0x12,
    OWC_CMD_SCAN_ROMS      = 0x20,
    OWC_CMD_SET_THRESHOLDS = 0x21,
    OWC_CMD_ARM_SESSION    = 0x30,
    OWC_CMD_EMIT_RESET     = 0x31,
    OWC_CMD_EMIT_BYTE      = 0x32,
    OWC_CMD_BRIDGE_MODE    = 0x33,
    OWC_CMD_CLEAR_FAULT    = 0x40
} owc_command_t;

typedef enum {
    OWC_STATUS_OK          = 0x00,
    OWC_STATUS_BAD_FRAME   = 0x01,
    OWC_STATUS_DENIED      = 0x02,
    OWC_STATUS_BUSY        = 0x03,
    OWC_STATUS_RANGE       = 0x04,
    OWC_STATUS_HW_FAULT    = 0x05,
    OWC_STATUS_NO_DEVICE   = 0x06
} owc_status_t;

typedef enum {
    OWC_EVT_RESET,
    OWC_EVT_PRESENCE,
    OWC_EVT_BIT,
    OWC_EVT_BYTE,
    OWC_EVT_ROM,
    OWC_EVT_TIMING_ANOMALY,
    OWC_EVT_VOLTAGE_ANOMALY,
    OWC_EVT_CONTENTION,
    OWC_EVT_POLICY_DENIAL
} owc_event_type_t;

typedef enum {
    OWC_MODE_FAIL_OPEN,
    OWC_MODE_MONITOR,
    OWC_MODE_ISOLATED,
    OWC_MODE_LAB_DRIVE
} owc_bridge_mode_t;

/* Memory-mapped FPGA/CPLD edge-capture aperture. */
#define REG_CAPTURE_CONTROL        0x40020000u
#define REG_CAPTURE_STATUS         0x40020004u
#define REG_EDGE_FIFO              0x40020008u
#define REG_THRESHOLD_LOW          0x4002000Cu
#define REG_THRESHOLD_HIGH         0x40020010u
#define REG_DRIVE_CONTROL          0x40020014u
#define REG_FAULT_STATUS           0x40020018u
#define REG_TRIGGER_CONTROL        0x4002001Cu

#define CAPTURE_ENABLE             (1u << 0)
#define CAPTURE_CLEAR              (1u << 1)
#define CAPTURE_OVERFLOW           (1u << 8)
#define DRIVE_UPSTREAM_LOW         (1u << 0)
#define DRIVE_DOWNSTREAM_LOW       (1u << 1)
#define DRIVE_STRONG_PULLUP        (1u << 2)
#define FAULT_SHORT                (1u << 0)
#define FAULT_OVERVOLT             (1u << 1)
#define FAULT_CONTENTION           (1u << 2)
#define FAULT_WATCHDOG             (1u << 3)

#endif /* OWC_REGISTERS_H */
