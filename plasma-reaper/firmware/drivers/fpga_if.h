/*
 * fpga_if.h — FPGA interface driver header
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef FPGA_IF_H
#define FPGA_IF_H

#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

/* FPGA register addresses (accessed via SPI) */
#define FPGA_REG_TRIGGER_CTRL   0x00
#define FPGA_REG_TRIGGER_SRC    0x01
#define FPGA_REG_TRIGGER_OFFSET 0x02  /* 32-bit, 4 bytes */
#define FPGA_REG_GLITCH_CTRL    0x06
#define FPGA_REG_GLITCH_SHAPE   0x07
#define FPGA_REG_GLITCH_WIDTH   0x08  /* 32-bit */
#define FPGA_REG_STATUS         0x0C
#define FPGA_REG_VERSION        0x0D
#define FPGA_REG_RESET          0x0E

/* Trigger source codes (must match FPGA bitstream) */
#define FPGA_TRIG_SRC_GPIO      0x00
#define FPGA_TRIG_SRC_UART      0x01
#define FPGA_TRIG_SRC_POWER_ENV 0x02
#define FPGA_TRIG_SRC_MANUAL    0x03
#define FPGA_TRIG_SRC_TIMER     0x04
#define FPGA_TRIG_SRC_EXT_SYNC  0x05

/* Status bits */
#define FPGA_STATUS_FIRED       0x01
#define FPGA_STATUS_TIMEOUT     0x02
#define FPGA_STATUS_ARMED       0x04
#define FPGA_STATUS_CDONE       0x08

void fpga_init(void);
bool fpga_load_bitstream(void);
void fpga_write_reg(uint8_t reg, const uint8_t *data, uint8_t len);
void fpga_read_reg(uint8_t reg, uint8_t *data, uint8_t len);
void fpga_arm_trigger(trigger_source_t source, uint32_t offset_ns);
bool fpga_glitch_fired(void);
bool fpga_trigger_timed_out(void);
void fpga_disarm(void);
uint8_t fpga_get_version(void);

#endif /* FPGA_IF_H */