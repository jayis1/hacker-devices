/*
 * TRACE-REAPER — QSPI-backed trace ring buffer driver
 *
 * Holds the most recent traces in external QSPI NOR (W25Q64) as a circular
 * ring buffer. Each trace is stored as a header (16 bytes) + window of
 * int16 samples. The buffer is per-session and is encrypted at the page
 * level with a session key (XOR-keystream derived from the operator passkey
 * for v1; a real build would use AES-CTR).
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "trace_buf.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

/* QSPI NOR is memory-mapped after board_init(). We model it as a flat array. */
#define QSPI_MEM_BASE  0x90000000UL
#define QSPI_SIZE      (8UL * 1024UL * 1024UL)  /* 8 MB */

#define TRACE_HDR_MAGIC 0x54524143UL  /* 'TRAC' */

/* Ring buffer layout: header at QSPI offset 0:
 *   magic, write_idx, count, capacity, session_id[16], key_xor[16]
 * Each slot: header { magic, ts_ms, nsamp } + nsamp * int16 samples
 */
typedef struct {
    uint32_t magic;
    uint32_t ts_ms;
    uint16_t nsamp;
    uint16_t _pad;
} trace_hdr_t;

typedef struct {
    uint32_t magic;
    uint32_t write_idx;
    uint32_t count;
    uint32_t capacity;
    uint8_t  session_id[SESSION_ID_LEN];
    uint8_t  key_xor[16];
} ring_hdr_t;

static volatile uint8_t *qspi = (volatile uint8_t *)QSPI_MEM_BASE;
static ring_hdr_t *ring = (ring_hdr_t *)QSPI_MEM_BASE;
static uint8_t g_key_xor[16];

/* Simple keystream from a 16-byte seed (XOR repeat). Real build uses AES-CTR. */
static void enc_apply(uint8_t *buf, uint32_t len, uint32_t offset)
{
    for (uint32_t i = 0; i < len; i++) {
        buf[i] ^= g_key_xor[(offset + i) & 15];
    }
}

void trace_buf_init(const uint8_t session_id[SESSION_ID_LEN],
                    const uint8_t passkey[16])
{
    /* Derive a per-session XOR key from the passkey (toy v1; replace w/ KDF) */
    for (int i = 0; i < 16; i++) g_key_xor[i] = passkey[i] ^ session_id[i % SESSION_ID_LEN];

    /* Wipe the ring header region */
    memset((void *)ring, 0, sizeof(ring_hdr_t));
    ring->magic = TRACE_HDR_MAGIC;
    ring->write_idx = 0;
    ring->count = 0;
    ring->capacity = (QSPI_SIZE - sizeof(ring_hdr_t)) /
                     (sizeof(trace_hdr_t) + TRACE_MAX_SAMPLES * 2);
    memcpy(ring->session_id, session_id, SESSION_ID_LEN);
    memcpy(ring->key_xor, g_key_xor, 16);
}

static uint8_t *slot_addr(uint32_t idx)
{
    uint32_t slot_size = sizeof(trace_hdr_t) + TRACE_MAX_SAMPLES * 2;
    return (uint8_t *)QSPI_MEM_BASE + sizeof(ring_hdr_t) + idx * slot_size;
}

int trace_buf_push(const int16_t *samples, uint16_t nsamp, uint32_t ts_ms)
{
    if (nsamp > TRACE_MAX_SAMPLES) return -1;
    uint8_t *slot = slot_addr(ring->write_idx);
    trace_hdr_t hdr = { TRACE_HDR_MAGIC, ts_ms, nsamp, 0 };

    /* Encrypt-in-place (XOR keystream) */
    uint8_t tmp[TRACE_MAX_SAMPLES * 2];
    memcpy(tmp, samples, nsamp * 2);
    enc_apply(tmp, nsamp * 2, ring->write_idx * (sizeof(trace_hdr_t) + TRACE_MAX_SAMPLES*2));

    /* Write header + encrypted samples */
    memcpy(slot, &hdr, sizeof(hdr));
    memcpy(slot + sizeof(hdr), tmp, nsamp * 2);

    ring->write_idx = (ring->write_idx + 1) % ring->capacity;
    if (ring->count < ring->capacity) ring->count++;
    return 0;
}

int trace_buf_get(uint32_t idx, int16_t *out, uint16_t *nsamp, uint32_t *ts_ms)
{
    if (idx >= ring->count) return -1;
    uint8_t *slot = slot_addr(idx);
    trace_hdr_t hdr;
    memcpy(&hdr, slot, sizeof(hdr));
    if (hdr.magic != TRACE_HDR_MAGIC) return -1;
    if (hdr.nsamp > TRACE_MAX_SAMPLES) return -1;

    uint8_t tmp[TRACE_MAX_SAMPLES * 2];
    memcpy(tmp, slot + sizeof(hdr), hdr.nsamp * 2);
    enc_apply(tmp, hdr.nsamp * 2, idx * (sizeof(trace_hdr_t) + TRACE_MAX_SAMPLES*2));
    memcpy(out, tmp, hdr.nsamp * 2);
    *nsamp = hdr.nsamp;
    if (ts_ms) *ts_ms = hdr.ts_ms;
    return 0;
}

uint32_t trace_buf_count(void) { return ring->count; }

void trace_buf_wipe(void)
{
    /* Overwrite entire QSPI ring region with zeros, then header. */
    memset((void *)QSPI_MEM_BASE, 0, QSPI_SIZE);
    ring->magic = TRACE_HDR_MAGIC;
    ring->write_idx = 0;
    ring->count = 0;
    ring->capacity = 0;
}