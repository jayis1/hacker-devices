/*
 * codec_reg.h — FPGA codec register map (mirror of the Verilog)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The iCE40 codec presents an 8-bit register interface over SPI slave.
 * The MCU is master. This file documents the map; the actual Verilog
 * is shipped separately under codec/.
 *
 * Register | R/W | Function
 * ---------|-----|----------------------------------------------------
 * 0x00 STATUS   | R  | ch0/1 rdy, ch0/1 err, ch0/1 active, tx_busy, parity_fault
 * 0x01 TX_CTL   | W  | bits[0] = channel, bit[7] = GO (start tx of queued word)
 * 0x02 RX_RD    | R  | next RX word (16 data bits)
 * 0x03 RX_TS_LO | R  | timestamp low 16 bits
 * 0x04 FUZZ     | W  | fuzz mask (parity/sync/gap/...)
 * 0x05 GAP_US   | W  | inter-message gap override in µs
 * 0x06 ARM      | W  | 0xA5A5 = arm tx path, 0x0000 = disarm
 * 0x07 TIMESTAMP| R  | free-running 96 MHz counter[31:0]
 * 0x10 TX_LO    | W  | low 16 bits of 20-bit tx word
 * 0x11 TX_HI    | W  | high 4 bits of 20-bit tx word
 *
 * Read transaction: MOSI = 0x80 | reg, then 0x0000 (dummy), read MISO.
 * Write transaction: MOSI = reg, then 16-bit data.
 */

#ifndef CODEC_REG_H
#define CODEC_REG_H

/* Re-export the constants from board.h for the codec author's convenience */
#include "../board.h"

/* Fuzz mask bits (FPGA-side implementation) */
#define CODEC_FUZZ_PARITY    0x0001   /* flip parity bit on next tx word */
#define CODEC_FUZZ_SYNC      0x0002   /* corrupt sync head (10->11)      */
#define CODEC_FUZZ_GAP_SHORT 0x0004   /* force gap < 4 µs next message    */
#define CODEC_FUZZ_BAD_LEN   0x0008   /* force word count = 31 (=32 wrap) */
#define CODEC_FUZZ_RT_FAULT  0x0010   /* inject status-word fault bits    */
#define CODEC_FUZZ_RESERVED  0x0020   /* reserved-mode-code               */

/* ARM magic */
#define CODEC_ARM_MAGIC 0xA5A5u
#define CODEC_DISARM    0x0000u

#endif /* CODEC_REG_H */