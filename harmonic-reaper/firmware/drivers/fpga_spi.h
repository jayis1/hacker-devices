/*
 * fpga_spi.h — Spartan-7 FPGA control & classifier interface
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The FPGA runs two 1024-point FFTs (one per harmonic channel), integrates
 * N=16 spectra coherently, and computes the 2f0/3f0 ratio. It raises an IRQ
 * (FPGA_IRQ_PIN) when a fresh result is ready. The MCU reads the result
 * over SPI and feeds it into the state machine in main.c.
 */
#ifndef HARMONIC_REAPER_FPGA_SPI_H
#define HARMONIC_REAPER_FPGA_SPI_H

#include <stdint.h>

typedef struct {
    int16_t p2_dbfs;    /* 2f0 power in dBFS                  */
    int16_t p3_dbfs;    /* 3f0 power in dBFS                  */
    int8_t  ratio_db;  /* 10*log10(P2/P3), clamped to ±40     */
} fpga_result_t;

/* Boot the FPGA (pulse PROGRAM_B, wait for DONE). */
void fpga_boot(void);

/* Send classifier thresholds to the FPGA. */
void fpga_set_thresholds(int8_t semi_db, int8_t metal_db);

/* Non-blocking: returns 1 if a new result is ready, fills *out. */
uint8_t fpga_read_result(fpga_result_t *out);

/* Configure integration length (number of spectra averaged). */
void fpga_set_integration(uint8_t n_spectra);

/* Get FPGA status word (fault flags, version). */
uint32_t fpga_get_status(void);

#endif /* HARMONIC_REAPER_FPGA_SPI_H */
/* EOF — fpga_spi.h — jayis1 */