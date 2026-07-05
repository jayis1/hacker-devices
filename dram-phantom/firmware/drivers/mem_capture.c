/*
 * mem_capture.c — warm-boot memory acquisition
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * When the host loses power (or the operator forces a reset), the FPGA takes
 * over the DDR4 CA bus and issues REFRESH commands every tREFI to keep the
 * DRAM alive from battery. The MCU then walks every row in every bank issuing
 * READ commands, draining the DQ bytes over SPI to the STM32, which streams
 * them to a file on the SD card (and/or over USB CDC to the app).
 *
 * The drain rate must keep up with the FPGA's READ sweep. At ~20 MHz SPI and
 * 64-byte bursts per READ, we sustain ~3 MB/s, enough for an 8 GB module in
 * roughly 45 minutes — but we only have ~90 s of battery. For that reason the
 * default capture window is a configurable subset (e.g., a 256 MB region where
 * keys are likely). Full-module capture requires the operator to keep the host
 * VDD alive (e.g., an external bench supply) so refresh draws from host power.
 */

#include "board.h"
#include "registers.h"
#include <string.h>
#include <stdio.h>

static uint32_t g_cap_base_row;
static uint32_t g_cap_row_count;

int mem_capture_start(uint32_t base_row, uint32_t row_count) {
    uint32_t st = fpga_status();
    if (!(st & FSTAT_DDR_PRESENT)) return -1;
    if (!(st & FSTAT_TARGET_ARMED)) return -2;
    if (!(st & FSTAT_REFRESH_ACTIVE)) return -3; /* must have refresh running */
    g_cap_base_row = base_row;
    g_cap_row_count = row_count;
    return 0;
}

/* Drain one chunk (4 KB) from the FPGA image FIFO and append to the SD file. */
static int drain_chunk(const char *fname) {
    uint8_t buf[4096];
    uint32_t got = 0;
    if (fpga_drain_image(buf, sizeof(buf), &got) != 0 || got == 0) return -1;
    return sdcard_write_file(fname, buf, got);
}

int mem_capture_drain_to_sd(void) {
    char fname[24];
    snprintf(fname, sizeof(fname), "dump_%lu.bin", (unsigned long)millis());

    uint32_t total_bytes = g_cap_row_count * 8192UL; /* assume 8 KB/row */
    uint32_t done = 0;
    uint32_t t0 = millis();

    while (done < total_bytes) {
        if (drain_chunk(fname) != 0) {
            /* FIFO empty or error — check FPGA still refreshing */
            uint32_t st = fpga_status();
            if (!(st & FSTAT_REFRESH_ACTIVE)) {
                return -1; /* lost refresh — abort */
            }
            delay_ms(1);
            continue;
        }
        done += 4096;
        if ((done % (256*1024)) == 0) {
            oled_printf(5, "cap %luKB", (unsigned long)(done/1024));
        }
        /* Watchdog: if we exceed the battery budget, abort gracefully */
        if ((millis() - t0) > 85000) {
            return -2;
        }
    }
    return 0;
}