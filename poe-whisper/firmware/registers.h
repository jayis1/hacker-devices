/*
 * registers.h - simulation register shims for PoE Whisper
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef POE_WHISPER_REGISTERS_H
#define POE_WHISPER_REGISTERS_H

#include <stdint.h>

typedef struct {
    uint32_t ctrl;
    uint32_t status;
    uint32_t brownout_ticks;
    uint32_t relay_mask;
    uint32_t temp_raw;
    uint32_t current_raw;
    uint32_t voltage_raw;
} pw_fpga_regs_t;

typedef struct {
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t crc_errors;
    uint32_t last_opcode;
} pw_radio_regs_t;

extern pw_fpga_regs_t PW_FPGA;
extern pw_radio_regs_t PW_RADIO;

#define PW_CTRL_ENABLE_ACTIVE   (1u << 0)
#define PW_CTRL_FORCE_BYPASS    (1u << 1)
#define PW_CTRL_BROWNOUT_ARMED  (1u << 2)
#define PW_CTRL_LLDP_SPOOF      (1u << 3)
#define PW_CTRL_MPS_JITTER      (1u << 4)

#define PW_STATUS_PSE_PRESENT   (1u << 0)
#define PW_STATUS_PD_PRESENT    (1u << 1)
#define PW_STATUS_LINK_UP       (1u << 2)
#define PW_STATUS_THERMAL_FAULT (1u << 3)
#define PW_STATUS_OVERCURRENT   (1u << 4)

#endif
