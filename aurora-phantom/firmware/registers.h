/*
 * registers.h — FPGA signal-core register file definitions
 *
 * Device:  Aurora-Phantom — Optical Compromising-Emanations Reconstruction
 * Author:  jayis1
 * License: GPL-2.0
 *
 * The iCE40UP5K FPGA exposes a 16-bit-wide register file to the ESP32-S3 over
 * SPI. Each register has a 7-bit address (128 slots) and a 16-bit payload.
 * This header defines the address map, bit layouts, and accessors.
 *
 * The register file is grouped:
 *   0x00-0x0F  global control / status
 *   0x10-0x2F  per-pixel SPAD gating & TDC
 *   0x30-0x4F  lock-in demodulator (LO freq, phase, integration)
 *   0x50-0x5F  refresh-rate estimator (FFT bin / PLL)
 *   0x60-0x7F  frame buffer / IQ accumulator + event stream FIFO
 */
#ifndef AURORA_PHANTOM_REGISTERS_H
#define AURORA_PHANTOM_REGISTERS_H

#include <stdint.h>

#define FPGA_REG_ADDR_BITS  7
#define FPGA_REG_COUNT      (1u << FPGA_REG_ADDR_BITS)  /* 128 */
#define FPGA_REG_WIDTH      16

/* ---- Global control / status (0x00-0x0F) ------------------------------- */
#define REG_MAGIC           0x00  /* RO: 0xA51D on power-on if FPGA alive */
#define REG_VERSION         0x01  /* RO: signal-core firmware version */
#define REG_CTRL            0x02  /* RW: global control bits */
#define   CTRL_ENABLE          (1u << 0)  /* master enable (gate all cores) */
#define   CTRL_SPAD_ENABLE     (1u << 1)  /* SPAD bias + TDC enable */
#define   CTRL_LOCKIN_ENABLE   (1u << 2)  /* lock-in demod enable */
#define   CTRL_REFRESH_ENABLE  (1u << 3)  /* refresh-rate estimator enable */
#define   CTRL_STREAM_EVENTS   (1u << 4)  /* stream raw events to FIFO */
#define   CTRL_STREAM_IQ       (1u << 5)  /* stream IQ accumulator to FIFO */
#define   CTRL_RESET           (1u << 6)  /* self-clearing soft reset */
#define   CTRL_RF_SILENCE      (1u << 7)  /* mirror of PMIC RF-rail state */
#define REG_STATUS          0x03  /* RO: global status bits */
#define   STAT_RUNNING         (1u << 0)
#define   STAT_LOCKED          (1u << 1)  /* refresh-rate PLL locked */
#define   STAT_FIFO_OVFL       (1u << 2)  /* event FIFO overflowed */
#define   STAT_FIFO_EMPTY      (1u << 3)
#define   STAT_FIFO_HALFFULL   (1u << 4)
#define   STAT_SPAD_SAT        (1u << 5)  /* any pixel saturated (dead-time) */
#define   STAT_MISSION_DONE    (1u << 6)
#define REG_INT_ENABLE     0x04  /* RW: interrupt-enable mask (-> ESP32 GPIO) */
#define REG_INT_STATUS     0x05  /* RO/W1C: interrupt pending bits */
#define REG_TICKS_HI       0x06  /* RO: 64-bit free counter high */
#define REG_TICKS_LO       0x07  /* RO: 64-bit free counter low  (1 tick=10ns) */
#define REG_SCRATCH        0x08  /* RW: scratch for comms test */

/* ---- Per-pixel SPAD gating & TDC (0x10-0x2F) --------------------------- */
#define REG_SPAD_BIAS      0x10  /* RW: global SPAD bias trim (0-4095) */
#define REG_SPAD_GATE_EN   0x11  /* RW: per-pixel gate-enable bitmask (low) */
#define REG_SPAD_GATE_EN2  0x12  /* RW: per-pixel gate-enable bitmask (high) */
#define REG_SPAD_TDC_RES   0x13  /* RW: TDC resolution select (0=1ns,1=0.5ns,2=0.25ns) */
#define REG_SPAD_DEADTM    0x14  /* RW: per-pixel dead-time override (ns) */
#define REG_SPAD_RATE0     0x15  /* RO: instantaneous rate pixel 0 (kHz) */
#define REG_SPAD_RATE1     0x16  /* RO: instantaneous rate pixel 1 (kHz) */
#define REG_SPAD_RATE_SUM  0x17  /* RO: aggregate rate across all pixels (kHz) */
#define REG_SPAD_SAT_MASK  0x18  /* RO: bitmask of saturated pixels (low) */
#define REG_SPAD_SAT_MASK2 0x19  /* RO: bitmask of saturated pixels (high) */

/* ---- Lock-in demodulator (0x30-0x4F) ----------------------------------- */
#define REG_LO_FREQ_HI     0x30  /* RW: LO frequency high 16 bits (0.01 Hz/lsb) */
#define REG_LO_FREQ_LO     0x31  /* RW: LO frequency low  16 bits */
#define REG_LO_PHASE       0x32  /* RW: LO phase offset (0-65535 -> 0-2pi) */
#define REG_LO_PHASE_STEP  0x33  /* RW: per-sample phase step for PLL track */
#define REG_INT_WIN_HI     0x34  /* RW: integration window high 16 bits (in ticks) */
#define REG_INT_WIN_LO     0x35  /* RW: integration window low  16 bits */
#define REG_INT_RESET      0x36  /* W: self-clearing integration reset */
#define REG_I_ACC0         0x37  /* RO: I accumulator pixel 0 */
#define REG_Q_ACC0         0x38  /* RO: Q accumulator pixel 0 */
#define REG_IQ_MAG0        0x39  /* RO: |IQ| pixel 0 (sqrt approx) */
/* ...per-pixel IQ accumulators are accessed via auto-increment at 0x40-0x4F */
#define REG_IQ_AUTOIDX     0x40  /* RW: auto-increment pixel index (0-255) */
#define REG_IQ_AUTO_I      0x41  /* RO: I for current index (auto-increments) */
#define REG_IQ_AUTO_Q      0x42  /* RO: Q for current index (auto-increments) */

/* ---- Refresh-rate estimator (0x50-0x5F) -------------------------------- */
#define REG_FFT_SIZE_LOG   0x50  /* RW: log2 FFT size (8..12 -> 256..4096) */
#define REG_FFT_BIN_HI     0x51  /* RO: peak bin high 16 bits */
#define REG_FFT_BIN_LO     0x52  /* RO: peak bin low  16 bits */
#define REG_FFT_PEAK_MAG   0x53  /* RO: peak magnitude */
#define REG_PLL_K_P        0x54  /* RW: PLL proportional gain (Q8.8) */
#define REG_PLL_K_I        0x55  /* RW: PLL integral gain (Q8.8) */
#define REG_PLL_LOCK_THRESH 0x56 /* RW: lock threshold (SNR dB*10) */
#define REG_PLL_FREQ_HI    0x57  /* RO: tracked frequency high (0.01 Hz/lsb) */
#define REG_PLL_FREQ_LO    0x58  /* RO: tracked frequency low  */
#define REFRESH_EST_DEF_LOG  10  /* default: 1024-bin FFT */

/* ---- Frame buffer / event stream (0x60-0x7F) --------------------------- */
#define REG_FIFO_DATA      0x60  /* RO: read next 16-bit word from stream FIFO */
#define REG_FIFO_COUNT     0x61  /* RO: 16-bit words available in FIFO */
#define REG_FIFO_FLUSH     0x62  /* W:  self-clearing FIFO flush */
#define REG_FRAME_IDX      0x63  /* RW: frame index for frame-buffer peek */
#define REG_FRAME_MAG_HI   0x64  /* RO: |IQ| magnitude high for frame idx */
#define REG_FRAME_MAG_LO   0x65  /* RO: |IQ| magnitude low  for frame idx */
#define REG_FRAME_COUNT    0x66  /* RO: number of completed frames */
#define REG_FRAME_PERIOD   0x67  /* RW: frame period in ticks (10 ns each) */
#define REG_MAX_ADDR       0x7F

/* ---- Accessor prototypes ---------------------------------------------- */
/* Implemented in fpga_bus.c (part of spad_driver compilation unit). */
uint16_t fpga_read(uint8_t addr);
void     fpga_write(uint8_t addr, uint16_t val);
void     fpga_read_burst(uint8_t addr, uint16_t *buf, uint32_t n);

#endif /* AURORA_PHANTOM_REGISTERS_H */