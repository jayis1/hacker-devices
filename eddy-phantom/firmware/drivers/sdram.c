/*
 * eddy-phantom — sdram.c
 * SDRAM burst ring buffer management.
 * Uses the FMC-mapped 32 MB SDRAM at 0xC0000000 to store
 * captured burst records in a circular buffer.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* SDRAM base and slot management */
#define SDRAM_BASE_ADDR  FMC_BANK1_SDRAM
#define SLOT_SIZE        sizeof(burst_record_t)

/* Slot allocation bitmap (in DTCM for fast access) */
static uint8_t slot_bitmap[SDRAM_BURST_SLOTS / 8 + 1];
static uint32_t g_write_idx = 0;   /* next slot to write */
static uint32_t g_oldest_idx = 0;  /* oldest valid slot */
static uint32_t g_count = 0;       /* number of valid slots */

/* ── Initialize SDRAM ring buffer ─────────────────────────────── */
int sdram_init(void)
{
    /* Clear slot bitmap */
    for (int i = 0; i < (int)(SDRAM_BURST_SLOTS / 8 + 1); i++) {
        slot_bitmap[i] = 0;
    }
    g_write_idx = 0;
    g_oldest_idx = 0;
    g_count = 0;

    /* Verify SDRAM is accessible by writing a test pattern */
    volatile uint32_t *sdram = (volatile uint32_t *)SDRAM_BASE_ADDR;
    sdram[0] = 0x12345678;
    sdram[1] = 0xDEADBEEF;

    if (sdram[0] != 0x12345678 || sdram[1] != 0xDEADBEEF) {
        return -1;  /* SDRAM not accessible */
    }

    /* Clear first few slots to zero */
    volatile uint8_t *base = (volatile uint8_t *)SDRAM_BASE_ADDR;
    for (uint32_t i = 0; i < 4096; i++) {
        base[i] = 0;
    }

    return 0;
}

/* ── Allocate a burst slot ────────────────────────────────────── */
int sdram_alloc_burst_slot(uint32_t *slot_idx)
{
    if (g_count >= SDRAM_BURST_SLOTS) {
        /* Buffer full — overwrite oldest */
        *slot_idx = g_oldest_idx;
        g_oldest_idx = (g_oldest_idx + 1) % SDRAM_BURST_SLOTS;
        /* g_count stays at SDRAM_BURST_SLOTS */
    } else {
        *slot_idx = g_write_idx;
        g_write_idx = (g_write_idx + 1) % SDRAM_BURST_SLOTS;
        g_count++;
    }

    /* Mark slot as used in bitmap */
    slot_bitmap[*slot_idx / 8] |= (1U << (*slot_idx % 8));

    return 0;
}

/* ── Store a burst record into a slot ─────────────────────────── */
int sdram_store_burst(uint32_t slot_idx, burst_record_t *rec)
{
    if (slot_idx >= SDRAM_BURST_SLOTS)
        return -1;

    /* Calculate address: base + slot_idx * SLOT_SIZE */
    volatile burst_record_t *dst =
        (volatile burst_record_t *)(SDRAM_BASE_ADDR + slot_idx * SLOT_SIZE);

    /* Copy the record to SDRAM.
     * We copy field by field to ensure alignment safety. */
    dst->timestamp_ms = rec->timestamp_ms;
    dst->trigger_channel = rec->trigger_channel;

    for (int i = 0; i < ADC_CHANNELS; i++)
        dst->vga_gain[i] = rec->vga_gain[i];

    /* Copy sample data in chunks for SDRAM efficiency */
    volatile int16_t *src_samples = rec->samples;
    volatile int16_t *dst_samples = dst->samples;
    for (uint32_t i = 0; i < BURST_SAMPLES * ADC_CHANNELS; i++) {
        dst_samples[i] = src_samples[i];
    }

    dst->classified = rec->classified;
    dst->scancode = rec->scancode;
    dst->confidence = rec->confidence;
    dst->reserved = 0;

    return 0;
}

/* ── Retrieve a burst record from a slot ──────────────────────── */
int sdram_get_burst(uint32_t slot_idx, burst_record_t *rec)
{
    if (slot_idx >= SDRAM_BURST_SLOTS)
        return -1;

    /* Check if slot is allocated */
    if (!(slot_bitmap[slot_idx / 8] & (1U << (slot_idx % 8))))
        return -1;  /* slot not in use */

    volatile burst_record_t *src =
        (volatile burst_record_t *)(SDRAM_BASE_ADDR + slot_idx * SLOT_SIZE);

    rec->timestamp_ms = src->timestamp_ms;
    rec->trigger_channel = src->trigger_channel;

    for (int i = 0; i < ADC_CHANNELS; i++)
        rec->vga_gain[i] = src->vga_gain[i];

    volatile int16_t *src_samples = src->samples;
    volatile int16_t *dst_samples = rec->samples;
    for (uint32_t i = 0; i < BURST_SAMPLES * ADC_CHANNELS; i++) {
        dst_samples[i] = src_samples[i];
    }

    rec->classified = src->classified;
    rec->scancode = src->scancode;
    rec->confidence = src->confidence;

    return 0;
}

/* ── Free a burst slot ────────────────────────────────────────── */
void sdram_free_burst_slot(uint32_t slot_idx)
{
    if (slot_idx >= SDRAM_BURST_SLOTS)
        return;

    slot_bitmap[slot_idx / 8] &= ~(1U << (slot_idx % 8));

    if (slot_idx == g_oldest_idx) {
        g_oldest_idx = (g_oldest_idx + 1) % SDRAM_BURST_SLOTS;
        if (g_count > 0) g_count--;
    }
}

/* ── Get the number of stored bursts ──────────────────────────── */
uint32_t sdram_get_count(void)
{
    return g_count;
}

/* ── Get the oldest slot index ────────────────────────────────── */
uint32_t sdram_get_oldest(void)
{
    return g_oldest_idx;
}

/* ── Clear all stored bursts ──────────────────────────────────── */
void sdram_clear_all(void)
{
    for (int i = 0; i < (int)(SDRAM_BURST_SLOTS / 8 + 1); i++) {
        slot_bitmap[i] = 0;
    }
    g_write_idx = 0;
    g_oldest_idx = 0;
    g_count = 0;
}