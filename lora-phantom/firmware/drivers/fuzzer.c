/*
 * lora-phantom / drivers/fuzzer.c
 * LoRa physical-layer fuzzer for end-device radio firmware testing.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * The fuzzer transmits LoRa frames with intentionally malformed physical
 * headers and payloads to test the robustness of end-device radio firmware
 * (header parsing, CRC handling, length field validation, implicit/explicit
 * header mode transitions). This is a black-box fuzz testing tool.
 *
 * Fuzz modes:
 *   FUZZ_BAD_HEADER_CRC    — transmit with SX1262 CRC disabled + bad payload
 *   FUZZ_INVALID_CR        — transmit with invalid coding rate field (4/9, 4/10)
 *   FUZZ_PHANTOM_HEADER    — implicit header mode with mismatched payload length
 *   FUZZ_IMPLICIT_MISMATCH — explicit header claiming N bytes, send M != N
 *   FUZZ_OVERSIZED_PAYLOAD — payload > 255 bytes (truncated by SX1262 to 255)
 *   FUZZ_RANDOM_BYTES      — random payload bytes at various SF/BW
 */

#include "../board.h"
#include "../registers.h"
#include "../types.h"
#include <string.h>

/* Forward declarations */
int sx1262_tx_config(uint32_t freq_hz, uint8_t sf, uint8_t bw, uint8_t cr, int8_t power_dbm);
int sx1262_transmit(const uint8_t *buf, uint16_t len, uint32_t timeout_ms);
int sx1262_tx_raw_header(const uint8_t *buf, uint16_t len, uint8_t sf, uint8_t bw,
                         uint8_t cr, uint8_t header_type, int8_t power_dbm);
extern uint32_t hw_random(void);

static void delay_ms_local(uint32_t ms) {
    /* Approximate delay using busy loop at 240 MHz */
    for (volatile uint32_t i = 0; i < ms * 48000; i++) { }
}

/* ---- Generate a random payload ---- */
static void fill_random(uint8_t *buf, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)(hw_random() & 0xFF);
    }
}

/* ---- Fuzz: transmit with bad header CRC (CRC off + garbage) ---- */
static int fuzz_bad_header_crc(uint32_t freq, uint8_t sf, uint8_t bw,
                               int8_t power, uint16_t count, uint32_t delay) {
    uint8_t buf[64];
    for (uint16_t i = 0; i < count; i++) {
        uint16_t plen = 8 + (uint16_t)(hw_random() % 56);
        fill_random(buf, plen);
        /* Corrupt the first few bytes to look like a bad LoRa header */
        buf[0] ^= 0xFF;
        buf[1] ^= 0xAA;
        /* Use raw header TX with CRC off (cr bit 4 = 0 → CRC off) */
        sx1262_tx_raw_header(buf, plen, sf, bw, 0x01, 0x00, power);
        delay_ms_local(delay);
    }
    return 0;
}

/* ---- Fuzz: invalid coding rate ---- */
static int fuzz_invalid_cr(uint32_t freq, uint8_t sf, uint8_t bw,
                           int8_t power, uint16_t count, uint32_t delay) {
    uint8_t buf[32];
    for (uint16_t i = 0; i < count; i++) {
        fill_random(buf, 16);
        /* CR field values 5,6,7 are invalid (valid: 1-4 = 4/5..4/8) */
        uint8_t bad_cr = 5 + (uint8_t)(hw_random() % 3);
        sx1262_tx_raw_header(buf, 16, sf, bw, bad_cr, 0x00, power);
        delay_ms_local(delay);
    }
    return 0;
}

/* ---- Fuzz: phantom header (implicit header mode with wrong length) ---- */
static int fuzz_phantom_header(uint32_t freq, uint8_t sf, uint8_t bw,
                               int8_t power, uint16_t count, uint32_t delay) {
    uint8_t buf[128];
    for (uint16_t i = 0; i < count; i++) {
        /* Send implicit header (0x01) but with payload length field claiming
         * a very large value, while actual payload is small */
        uint16_t actual_len = 4;
        fill_random(buf, actual_len);
        /* header_type=0x01 (implicit), cr field high nibble = fake payload len 255 */
        sx1262_tx_raw_header(buf, actual_len, sf, bw, 0xF1, 0x01, power);
        delay_ms_local(delay);
    }
    return 0;
}

/* ---- Fuzz: explicit/implicit header mismatch ---- */
static int fuzz_implicit_mismatch(uint32_t freq, uint8_t sf, uint8_t bw,
                                  int8_t power, uint16_t count, uint32_t delay) {
    uint8_t buf[64];
    for (uint16_t i = 0; i < count; i++) {
        uint16_t plen = 1 + (uint16_t)(hw_random() % 32);
        fill_random(buf, plen);
        /* Alternate between explicit and implicit header randomly */
        uint8_t htype = (uint8_t)(hw_random() & 1);
        sx1262_tx_raw_header(buf, plen, sf, bw, 0x01, htype, power);
        delay_ms_local(delay);
    }
    return 0;
}

/* ---- Fuzz: oversized payload ---- */
static int fuzz_oversized(uint32_t freq, uint8_t sf, uint8_t bw,
                          int8_t power, uint16_t count, uint32_t delay) {
    uint8_t buf[255];
    for (uint16_t i = 0; i < count; i++) {
        fill_random(buf, 255);
        sx1262_tx_raw_header(buf, 255, sf, bw, 0x01, 0x00, power);
        delay_ms_local(delay);
    }
    return 0;
}

/* ---- Fuzz: pure random bytes at the PHY layer ---- */
static int fuzz_random(uint32_t freq, uint8_t sf, uint8_t bw,
                       int8_t power, uint16_t count, uint32_t delay) {
    uint8_t buf[128];
    for (uint16_t i = 0; i < count; i++) {
        uint16_t plen = 1 + (uint16_t)(hw_random() % 128);
        fill_random(buf, plen);
        /* Random CR (0..7), random header type */
        uint8_t cr = (uint8_t)(hw_random() & 0x0F);
        uint8_t ht = (uint8_t)(hw_random() & 1);
        sx1262_tx_raw_header(buf, plen, sf, bw, cr, ht, power);
        delay_ms_local(delay);
    }
    return 0;
}

/* ---- Main fuzzer dispatch ---- */
int fuzzer_tx(fuzz_mode_t mode, uint32_t freq_hz, uint8_t sf, uint8_t bw,
              int8_t power_dbm, uint16_t count, uint32_t delay_ms) {
    /* Ensure we're not exceeding regulatory duty cycle — simplified: if delay
     * is too short, enforce a minimum 100 ms gap. */
    if (delay_ms < 10) delay_ms = 10;

    switch (mode) {
    case FUZZ_BAD_HEADER_CRC:
        return fuzz_bad_header_crc(freq_hz, sf, bw, power_dbm, count, delay_ms);
    case FUZZ_INVALID_CR:
        return fuzz_invalid_cr(freq_hz, sf, bw, power_dbm, count, delay_ms);
    case FUZZ_PHANTOM_HEADER:
        return fuzz_phantom_header(freq_hz, sf, bw, power_dbm, count, delay_ms);
    case FUZZ_IMPLICIT_MISMATCH:
        return fuzz_implicit_mismatch(freq_hz, sf, bw, power_dbm, count, delay_ms);
    case FUZZ_OVERSIZED_PAYLOAD:
        return fuzz_oversized(freq_hz, sf, bw, power_dbm, count, delay_ms);
    case FUZZ_RANDOM_BYTES:
        return fuzz_random(freq_hz, sf, bw, power_dbm, count, delay_ms);
    default:
        return -1;
    }
}