/*
 * registers.h - EDID Phantom simulated register map
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef EDID_PHANTOM_REGISTERS_H
#define EDID_PHANTOM_REGISTERS_H

#include <stdint.h>

#define REG_SYS_STATUS           0x0000u
#define REG_SYS_CONTROL          0x0004u
#define REG_SYS_TEMP_CX100       0x0008u
#define REG_SYS_CURRENT_MA       0x000Cu
#define REG_DDC_STATUS           0x0100u
#define REG_DDC_CONTROL          0x0104u
#define REG_DDC_LAST_ADDR        0x0108u
#define REG_DDC_LAST_LENGTH      0x010Cu
#define REG_HPD_STATUS           0x0200u
#define REG_HPD_CONTROL          0x0204u
#define REG_CEC_STATUS           0x0300u
#define REG_CEC_CONTROL          0x0304u
#define REG_CEC_LAST_OPCODE      0x0308u
#define REG_RADIO_STATUS         0x0400u
#define REG_RADIO_CONTROL        0x0404u
#define REG_LED_STATUS           0x0500u
#define REG_POLICY_STATUS        0x0600u

#define SYS_STATUS_TARGET_CONNECTED   (1u << 0)
#define SYS_STATUS_SINK_PRESENT       (1u << 1)
#define SYS_STATUS_THERMAL_DERATE     (1u << 2)
#define SYS_STATUS_MUTATION_ENABLED   (1u << 3)
#define SYS_STATUS_CEC_GUARD_ENABLED  (1u << 4)

#define DDC_STATUS_ACTIVITY           (1u << 0)
#define DDC_STATUS_CHECKSUM_VALID     (1u << 1)
#define DDC_STATUS_PROXY_ENABLED      (1u << 2)
#define DDC_STATUS_EMULATION_ENABLED  (1u << 3)

#define HPD_STATUS_ASSERTED           (1u << 0)
#define HPD_STATUS_PULSE_ARMED        (1u << 1)

#define CEC_STATUS_RX_READY           (1u << 0)
#define CEC_STATUS_TX_READY           (1u << 1)
#define CEC_STATUS_GUARD_BLOCK        (1u << 2)

#define RADIO_STATUS_CONNECTED        (1u << 0)
#define RADIO_STATUS_STREAMING        (1u << 1)

uint32_t reg_read(uint16_t reg);
void reg_write(uint16_t reg, uint32_t value);
void reg_set_bits(uint16_t reg, uint32_t mask);
void reg_clear_bits(uint16_t reg, uint32_t mask);
void reg_reset_all(void);

#endif
