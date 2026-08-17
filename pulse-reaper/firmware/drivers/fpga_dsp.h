/*
 * fpga_dsp.h — FPGA command interface
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_FPGA_DSP_H
#define PULSEREAPER_FPGA_DSP_H

#include <stdint.h>

/* FPGA command opcodes (sent over SPI1) */
#define FPGA_CMD_ARM_TDR         0x01u
#define FPGA_CMD_READ_REFLECTOGRAM 0x02u
#define FPGA_CMD_CONFIGURE_FILTER  0x03u
#define FPGA_CMD_READ_FRAME       0x04u
#define FPGA_CMD_COVERT_TX         0x05u
#define FPGA_CMD_COVERT_RX         0x06u
#define FPGA_CMD_SET_GAIN          0x07u
#define FPGA_CMD_VERSION           0x0Fu

/* Reflectogram buffer size (samples x 2 bytes) */
#define FPGA_REFLECTOGRAM_SAMPLES  2048u

/* Frame buffer (one recovered frame) */
#define FPGA_MAX_FRAME_LEN          512u

/* FPGA status bits */
#define FPGA_STATUS_TDR_READY      0x01u
#define FPGA_STATUS_FRAME_AVAIL    0x02u
#define FPGA_STATUS_COVERT_RX_AVAIL 0x04u
#define FPGA_STATUS_ERROR           0x80u

void fpga_dsp_init(void);

/* Reset and configure the FPGA with the default bitstream params. */
int  fpga_dsp_configure(void);

/* Read the FPGA status byte. */
uint8_t fpga_dsp_status(void);

/* Arm the TDR engine; the FPGA will drive the pulse and sample. */
int  fpga_dsp_arm_tdr(void);

/* Read the reflectogram (raw 16-bit samples). Returns samples read. */
int  fpga_dsp_read_reflectogram(int16_t *out, int max_samples);

/* Configure the FIR equaliser (64 taps, signed 16-bit coefficients). */
int  fpga_dsp_configure_filter(const int16_t *taps, int n_taps);

/* Read one recovered frame from the frame buffer. Returns bytes read
 * (0 if no frame available). */
int  fpga_dsp_read_frame(uint8_t *out, uint16_t *out_len, int max_len);

/* Covert-channel pump: TX one low-rate beacon or RX one beacon. */
void fpga_dsp_covert_pump(void);

/* Covert TX: queue a byte to be sent over the low-amplitude modulation. */
int  fpga_dsp_covert_tx_byte(uint8_t b);

/* Covert RX: read a received byte (returns -1 if none). */
int  fpga_dsp_covert_rx_byte(void);

#endif