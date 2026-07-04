/*
 * aperture-phantom / firmware / drivers / sdram.c
 *
 * FMC SDRAM controller setup for the IS42S16160J 16 MB SDRAM used as the
 * frame staging buffer. Configures FMC Bank 5/6 SDRAM timing and issues
 * the initialization sequence (precharge, auto-refresh, mode register set).
 * Provides simple byte/word accessors and a bump allocator.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include "board.h"

static uint32_t g_alloc_off;

void sdram_init(void) {
    /* SDCR1: 9-bit column, 13-bit row, 4 banks, CAS latency 3, 32-bit bus? *
     * IS42S16160J: 16-bit data, 4 banks, 9 col, 13 row, CAS3.               */
    FMC_Bank5_SDCR1 = (0u << 0)      /* CT = 8-bit col? no → 9 bit */
                    | (1u << 4)      /* NB = 4 banks */
                    | (2u << 2)      /* MWID = 16-bit */
                    | (2u << 0);     /* NR = 13-row */
    FMC_Bank5_SDCR1 |= (3u << 12);   /* CAS = 3 */

    /* SDCR2 is the same for 16-bit bus (no second chip). */
    FMC_Bank5_SDCR2 = FMC_Bank5_SDCR1;

    /* SDTR1 timing (in SDCLK cycles). SDCLK = 120 MHz. */
    FMC_Bank5_SDTR1 = (1u << 0)     /* TMRD = 1 */
                    | (6u << 4)     /* TXSR = 6 */
                    | (4u << 8)     /* TRAS = 4 */
                    | (6u << 12)    /* TRC  = 6 */
                    | (1u << 16)    /* TWR  = 1 */
                    | (1u << 20)    /* TRP  = 1 */
                    | (1u << 24);   /* TRCD = 1 */

    /* Initialization sequence via SDCMR. */
    FMC_Bank5_SDCMR = (1u << 0);     /* MODE = clock config enable */
    for (volatile int i = 0; i < 100000; i++) { }
    FMC_Bank5_SDCMR = (2u << 0) | (4u << 4); /* PRECHARGE ALL, bank select */
    for (volatile int i = 0; i < 50000; i++) { }
    FMC_Bank5_SDCMR = (3u << 0) | (4u << 4); /* AUTO REFRESH, 8 cycles */
    for (volatile int i = 0; i < 50000; i++) { }
    FMC_Bank5_SDCMR = (4u << 0) | (4u << 4) | (0x023u << 9); /* MODE REG: burst 1, CAS3 */

    /* Wait for SDRAM ready. */
    extern volatile uint32_t g_ticks_ms;
    uint32_t t0 = g_ticks_ms;
    while ((FMC_Bank5_SDSR & 1u) == 0) {
        if ((g_ticks_ms - t0) > 100) break;
    }

    g_alloc_off = 0;
}

void *sdram_alloc(uint32_t bytes) {
    if (g_alloc_off + bytes > SDRAM_SIZE_BYTES) return (void *)0;
    void *p = (void *)(SDRAM_BANK_ADDR + g_alloc_off);
    g_alloc_off += bytes;
    /* 4-byte align */
    g_alloc_off = (g_alloc_off + 3u) & ~3u;
    return p;
}

void sdram_write32(uint32_t off, uint32_t v) {
    *(volatile uint32_t *)(SDRAM_BANK_ADDR + off) = v;
}
uint32_t sdram_read32(uint32_t off) {
    return *(volatile uint32_t *)(SDRAM_BANK_ADDR + off);
}