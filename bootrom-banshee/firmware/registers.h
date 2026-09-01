/*
 * BootROM Banshee simulated register map
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef BOOTROM_BANSHEE_REGISTERS_H
#define BOOTROM_BANSHEE_REGISTERS_H

#define BB_REG_STATUS          0x0000u
#define BB_REG_CONTROL         0x0004u
#define BB_REG_BUS_MODE        0x0008u
#define BB_REG_DELAY_US        0x000Cu
#define BB_REG_TRUNCATE_LEN    0x0010u
#define BB_REG_STRAP_MASK      0x0014u
#define BB_REG_CAPTURE_COUNT   0x0018u
#define BB_REG_OVERLAY_INDEX   0x001Cu
#define BB_REG_FAULT_STATUS    0x0020u

#define BB_STATUS_TARGET_PRESENT (1u << 0)
#define BB_STATUS_FLASH_PRESENT  (1u << 1)
#define BB_STATUS_MUTATION_ON    (1u << 2)
#define BB_STATUS_TRIGGERED      (1u << 3)
#define BB_STATUS_TRIP           (1u << 4)

#define BB_CONTROL_PROXY         (1u << 0)
#define BB_CONTROL_MUTATION      (1u << 1)
#define BB_CONTROL_TRIGGER       (1u << 2)
#define BB_CONTROL_ABORT         (1u << 3)

#define BB_FAULT_OVERCURRENT     (1u << 0)
#define BB_FAULT_OVERTEMP        (1u << 1)
#define BB_FAULT_VOLTAGE         (1u << 2)
#define BB_FAULT_WATCHDOG        (1u << 3)

#endif
