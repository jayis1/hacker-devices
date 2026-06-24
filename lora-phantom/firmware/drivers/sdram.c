/*
 * lora-phantom / drivers/sdram.c
 * External SDRAM (IS42S16160G, 16 MB) capture ring buffer management.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * The SDRAM provides a 4 MB circular capture buffer for high-rate LoRa
 * frame sniffing. Frames are stored with a small header (length + metadata)
 * and can be popped later for replay or analysis.
 */

#include "../board.h"
#include "../registers.h"
#include <string.h>

/* Capture ring buffer structure in SDRAM.
 * Each entry: [magic(2) | len(2) | freq(4) | sf(1) | bw(1) | reserved(2) | data(len)]
 * The ring wraps around at CAPTURE_BUF_SIZE. */

#define ENTRY_MAGIC 0xCAFE
#define ENTRY_HEADER_LEN 12

static volatile uint8_t *s_ring = (volatile uint8_t *)SDRAM_BASE_ADDR;
static volatile uint32_t s_write_ptr = 0;
static volatile uint32_t s_read_ptr = 0;
static volatile uint32_t s_count = 0;

void sdram_init(void) {
    /* FMC SDRAM init is done in board_init.c (fmc_sdram_init).
     * Here we just verify the SDRAM is accessible by writing/reading a test. */
    volatile uint32_t *p = (volatile uint32_t *)SDRAM_BASE_ADDR;
    p[0] = 0x55AA55AA;
    p[1] = 0x12345678;
    if (p[0] != 0x55AA55AA || p[1] != 0x12345678) {
        /* SDRAM not accessible — captures will fall back to SRAM (smaller) */
    }
}

void *sdram_alloc(uint32_t bytes) {
    /* Simple bump allocator from the SDRAM region (after the capture ring). */
    static uint32_t s_alloc_ptr = CAPTURE_BUF_SIZE;
    if (s_alloc_ptr + bytes > SDRAM_SIZE_BYTES) return 0;
    void *p = (void *)(SDRAM_BASE_ADDR + s_alloc_ptr);
    s_alloc_ptr += bytes;
    return p;
}

void sdram_capture_reset(void) {
    s_write_ptr = 0;
    s_read_ptr = 0;
    s_count = 0;
}

uint32_t sdram_capture_push(const uint8_t *data, uint32_t len) {
    if (len > 256) len = 256;
    uint32_t total = ENTRY_HEADER_LEN + len;
    if (s_write_ptr + total > CAPTURE_BUF_SIZE) {
        s_write_ptr = 0;  /* wrap around */
    }

    /* Write entry header */
    s_ring[s_write_ptr + 0] = (uint8_t)(ENTRY_MAGIC & 0xFF);
    s_ring[s_write_ptr + 1] = (uint8_t)(ENTRY_MAGIC >> 8);
    s_ring[s_write_ptr + 2] = (uint8_t)(len & 0xFF);
    s_ring[s_write_ptr + 3] = (uint8_t)(len >> 8);
    /* freq/sf/bw are set by the caller via a separate path; here we store
     * just the raw frame for replay. */
    s_ring[s_write_ptr + 4] = 0;
    s_ring[s_write_ptr + 5] = 0;
    s_ring[s_write_ptr + 6] = 0;
    s_ring[s_write_ptr + 7] = 0;
    s_ring[s_write_ptr + 8] = 0;
    s_ring[s_write_ptr + 9] = 0;
    s_ring[s_write_ptr + 10] = 0;
    s_ring[s_write_ptr + 11] = 0;

    /* Write data */
    for (uint32_t i = 0; i < len; i++) {
        s_ring[s_write_ptr + ENTRY_HEADER_LEN + i] = data[i];
    }

    s_write_ptr += total;
    s_count++;
    return len;
}

uint32_t sdram_capture_pop(uint8_t *dst, uint32_t maxlen) {
    if (s_read_ptr == s_write_ptr) return 0;  /* empty */

    /* Read entry header */
    uint16_t magic = (uint16_t)s_ring[s_read_ptr] | ((uint16_t)s_ring[s_read_ptr + 1] << 8);
    if (magic != ENTRY_MAGIC) {
        /* Corrupted — reset */
        s_read_ptr = s_write_ptr;
        return 0;
    }
    uint16_t len = (uint16_t)s_ring[s_read_ptr + 2] | ((uint16_t)s_ring[s_read_ptr + 3] << 8);
    if (len > maxlen) len = (uint16_t)maxlen;

    for (uint16_t i = 0; i < len; i++) {
        dst[i] = s_ring[s_read_ptr + ENTRY_HEADER_LEN + i];
    }

    s_read_ptr += ENTRY_HEADER_LEN + len;
    if (s_read_ptr >= CAPTURE_BUF_SIZE) s_read_ptr = 0;
    s_count--;
    return len;
}

uint32_t sdram_capture_count(void) {
    return s_count;
}