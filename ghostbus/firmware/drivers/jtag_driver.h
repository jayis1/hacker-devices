/*
 * GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
 * JTAG Engine — IEEE 1149.1 bit-bang driver
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef GHOSTBUS_JTAG_DRIVER_H
#define GHOSTBUS_JTAG_DRIVER_H

#include "registers.h"
#include "board.h"

/* JTAG TAP states (per IEEE 1149.1) */
typedef enum {
    JTAG_TEST_LOGIC_RESET = 0,
    JTAG_RUN_TEST_IDLE = 1,
    JTAG_SELECT_DR_SCAN = 2,
    JTAG_CAPTURE_DR = 3,
    JTAG_SHIFT_DR = 4,
    JTAG_EXIT1_DR = 5,
    JTAG_PAUSE_DR = 6,
    JTAG_EXIT2_DR = 7,
    JTAG_UPDATE_DR = 8,
    JTAG_SELECT_IR_SCAN = 9,
    JTAG_CAPTURE_IR = 10,
    JTAG_SHIFT_IR = 11,
    JTAG_EXIT1_IR = 12,
    JTAG_PAUSE_IR = 13,
    JTAG_EXIT2_IR = 14,
    JTAG_UPDATE_IR = 15
} jtag_tap_state_t;

/* Standard JTAG instructions */
#define JTAG_INST_EXTEST         0x00
#define JTAG_INST_SAMPLE_PRELOAD 0x01
#define JTAG_INST_IDCODE         0x02
#define JTAG_INST_INTEST         0x03
#define JTAG_INST_BYPASS        0x0F
#define JTAG_INST_CLAMP         0x0C

typedef struct {
    uint8_t   tck_pin;
    uint8_t   tms_pin;
    uint8_t   tdi_pin;
    uint8_t   tdo_pin;
    uint32_t  idcode;
    uint8_t   ir_len;        /* instruction register length in bits */
    uint8_t   dr_len;        /* default DR length for IDCODE */
    uint8_t   chain_count;   /* number of TAPs in chain */
    uint32_t  chain_idcodes[8];
} jtag_target_info_t;

/* Public API */
void          jtag_init(probe_channel_t tck_ch, probe_channel_t tms_ch,
                       probe_channel_t tdi_ch, probe_channel_t tdo_ch);
void          jtag_set_clock(uint32_t hz);
uint32_t      jtag_get_clock(void);
void          jtag_tap_reset(void);
void          jtag_goto_state(jtag_tap_state_t target);
jtag_tap_state_t jtag_current_state(void);
gb_status_t   jtag_scan_chain(jtag_target_info_t *info);
gb_status_t   jtag_shift_ir(uint32_t instruction, uint8_t ir_len);
gb_status_t   jtag_shift_dr(uint32_t *tdi, uint32_t *tdo, uint32_t bits);
gb_status_t   jtag_read_idcode(uint32_t *idcode);
gb_status_t   jtag_read_dr(uint32_t *data, uint8_t bits);
gb_status_t   jtag_write_dr(uint32_t data, uint8_t bits);
gb_status_t   jtag_boundary_scan(uint32_t *sample_buf, uint8_t bits);

#endif /* GHOSTBUS_JTAG_DRIVER_H */