/*
 * ble_uart.c — Encrypted BLE C2 channel over Nordic UART Service
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * The nRF52840-M.2 module presents a transparent UART over BLE (NUS).
 * CoilGrip frames commands and telemetry with a tiny binary protocol
 * and encrypts the payload with AES-256-CTR using a session key derived
 * from an ECDH P-256 handshake performed at pairing time.
 *
 * For the reference firmware we model the UART as USART2 (PE5/PE6) and
 * implement the framing + a minimal AES-CTR (software, 8-bit lookup
 * tables) so the firmware is self-contained.
 */

#include "board.h"
#include "registers.h"

/* USART2 base (we reuse the USART struct) */
#define USART2_BASE 0x40004400U
#define USART2      ((usart_t *) USART2_BASE)

#define BLE_MAX_FRAME  128U
#define BLE_SOF        0xAA
#define BLE_EOF        0x55
#define BLE_ESC        0x7D

/* AES-256 round constants & S-box (FIPS-197) — trimmed to what we need. */
static const uint8_t aes_sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static uint8_t g_key[32];
static uint8_t g_nonce[16];
static uint32_t g_ctr;
static bool g_key_set = false;
static volatile bool g_connected = false;

/* ---- USART2 (BLE UART) ------------------------------------------------- */

static void usart2_setup(void)
{
    /* USART2 on APB1 @ 120 MHz, baud 115200 */
    USART2->BRR = (APB_HZ / 115200U);
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void uart2_putc(uint8_t c)
{
    while (!(USART2->ISR & USART_ISR_TXE)) { }
    USART2->TDR = c;
}

static int uart2_getc(uint8_t *c, uint32_t timeout_loops)
{
    for (uint32_t i = 0; i < timeout_loops; i++) {
        if (USART2->ISR & USART_ISR_RXNE) {
            *c = (uint8_t)USART2->RDR;
            return 0;
        }
    }
    return -1;
}

/* ---- Tiny AES-256-CTR (single block) ----------------------------------- */
/* We only need to XOR a keystream into the frame payload. A full AES-256
 * implementation is ~400 lines; for the reference build we provide a
 * compact encrypt-one-block function using the S-box and round constants.
 * This is *not* constant-time hardened — it is for the C2 link only, not
 * for protecting secrets at rest. */

static uint8_t xtime(uint8_t x) { return (uint8_t)((x << 1) ^ ((x & 0x80) ? 0x1b : 0)); }

static void aes256_encrypt_block(const uint8_t *key, const uint8_t in[16], uint8_t out[16])
{
    /* Simplified: we implement SubBytes + ShiftRows + MixColumns + AddRoundKey
     * for 14 rounds. Round-key expansion is done inline. */
    uint8_t state[16];
    uint8_t rk[240];
    /* Key expansion (compact): copy key then 14 rounds of expansion */
    for (int i = 0; i < 32; i++) rk[i] = key[i];
    uint8_t rcon = 1;
    for (int i = 32; i < 240; i += 4) {
        uint8_t t[4];
        t[0] = rk[i - 4]; t[1] = rk[i - 3]; t[2] = rk[i - 2]; t[3] = rk[i - 1];
        if (i % 32 == 0) {
            uint8_t tmp = t[0];
            t[0] = aes_sbox[t[1]] ^ rcon;
            t[1] = aes_sbox[t[2]];
            t[2] = aes_sbox[t[3]];
            t[3] = aes_sbox[tmp];
            rcon = (rcon << 1) ^ ((rcon & 0x80) ? 0x1b : 0);
        }
        for (int j = 0; j < 4; j++) rk[i + j] = rk[i - 32 + j] ^ t[j];
    }
    for (int i = 0; i < 16; i++) state[i] = in[i] ^ rk[i];
    for (int round = 1; round < 14; round++) {
        for (int i = 0; i < 16; i++) state[i] = aes_sbox[state[i]];
        /* ShiftRows */
        uint8_t tmp;
        tmp = state[1]; state[1] = state[5]; state[5] = state[9]; state[9] = state[13]; state[13] = tmp;
        tmp = state[2]; state[2] = state[10]; state[10] = tmp; tmp = state[6]; state[6] = state[14]; state[14] = tmp;
        tmp = state[15]; state[15] = state[11]; state[11] = state[7]; state[7] = state[3]; state[3] = tmp;
        /* MixColumns (compact) */
        for (int c = 0; c < 4; c++) {
            uint8_t a0 = state[c*4], a1 = state[c*4+1], a2 = state[c*4+2], a3 = state[c*4+3];
            uint8_t t = a0 ^ a1 ^ a2 ^ a3;
            state[c*4]   ^= t ^ xtime(a0 ^ a1);
            state[c*4+1] ^= t ^ xtime(a1 ^ a2);
            state[c*4+2] ^= t ^ xtime(a2 ^ a3);
            state[c*4+3] ^= t ^ xtime(a3 ^ a0);
        }
        for (int i = 0; i < 16; i++) state[i] ^= rk[round * 16 + i];
    }
    /* Final round (no MixColumns) */
    for (int i = 0; i < 16; i++) state[i] = aes_sbox[state[i]];
    uint8_t tmp;
    tmp = state[1]; state[1] = state[5]; state[5] = state[9]; state[9] = state[13]; state[13] = tmp;
    tmp = state[2]; state[2] = state[10]; state[10] = tmp; tmp = state[6]; state[6] = state[14]; state[14] = tmp;
    tmp = state[15]; state[15] = state[11]; state[11] = state[7]; state[7] = state[3]; state[3] = tmp;
    for (int i = 0; i < 16; i++) state[i] ^= rk[14 * 16 + i];
    for (int i = 0; i < 16; i++) out[i] = state[i];
}

/* Generate a 16-byte keystream block from the current counter */
static void ctr_keystream(uint8_t out[16])
{
    uint8_t ctr_block[16];
    for (int i = 0; i < 16; i++) ctr_block[i] = g_nonce[i];
    ctr_block[15] ^= (uint8_t)(g_ctr & 0xFF);
    ctr_block[14] ^= (uint8_t)((g_ctr >> 8) & 0xFF);
    aes256_encrypt_block(g_key, ctr_block, out);
    g_ctr++;
}

/* ---- Framing ----------------------------------------------------------- */
/* Frame: [SOF][seq(2)][len(1)][payload(len)][crc16(2)][EOF]
 * SOF/EOF/CRC are not escaped for simplicity in the reference build; a
 * production build would use COBS or byte-stuffing. */

static uint16_t crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
    return crc;
}

static uint16_t g_seq = 0;

int ble_init(void)
{
    usart2_setup();
    g_seq = 0;
    g_ctr = 0;
    g_key_set = false;
    g_connected = false;
    return 0;
}

int ble_set_key(const uint8_t key[32])
{
    for (int i = 0; i < 32; i++) g_key[i] = key[i];
    for (int i = 0; i < 16; i++) g_nonce[i] = key[i] ^ key[i + 16];  /* derive nonce */
    g_ctr = 0;
    g_key_set = true;
    return 0;
}

int ble_send(const uint8_t *data, uint16_t len)
{
    if (len > BLE_MAX_FRAME - 7) return -1;
    uint8_t frame[BLE_MAX_FRAME + 8];
    uint16_t flen = 0;
    frame[flen++] = BLE_SOF;
    frame[flen++] = (uint8_t)(g_seq >> 8);
    frame[flen++] = (uint8_t)(g_seq & 0xFF);
    frame[flen++] = (uint8_t)len;
    /* Encrypt payload in place if key is set */
    uint8_t enc[BLE_MAX_FRAME];
    if (g_key_set) {
        uint16_t i = 0;
        uint8_t ks[16];
        uint8_t kidx = 16;
        while (i < len) {
            if (kidx >= 16) { ctr_keystream(ks); kidx = 0; }
            enc[i] = data[i] ^ ks[kidx];
            i++; kidx++;
        }
    } else {
        for (uint16_t i = 0; i < len; i++) enc[i] = data[i];
    }
    for (uint16_t i = 0; i < len; i++) frame[flen++] = enc[i];
    uint16_t crc = crc16(frame + 1, flen - 1);  /* over seq+len+payload */
    frame[flen++] = (uint8_t)(crc >> 8);
    frame[flen++] = (uint8_t)(crc & 0xFF);
    frame[flen++] = BLE_EOF;
    for (uint16_t i = 0; i < flen; i++) uart2_putc(frame[i]);
    g_seq++;
    return 0;
}

int ble_recv(uint8_t *buf, uint16_t buf_sz, uint16_t *out_len, uint32_t timeout_ms)
{
    uint8_t c;
    uint32_t to = timeout_ms * 10000U;
    /* Wait for SOF */
    do { if (uart2_getc(&c, to)) return -1; } while (c != BLE_SOF);
    uint8_t seq_hi, seq_lo, len;
    if (uart2_getc(&seq_hi, to)) return -1;
    if (uart2_getc(&seq_lo, to)) return -1;
    if (uart2_getc(&len, to)) return -1;
    if (len > buf_sz) return -1;
    uint8_t enc[BLE_MAX_FRAME];
    for (uint16_t i = 0; i < len; i++)
        if (uart2_getc(&enc[i], to)) return -1;
    uint8_t crc_hi, crc_lo, eof;
    if (uart2_getc(&crc_hi, to)) return -1;
    if (uart2_getc(&crc_lo, to)) return -1;
    if (uart2_getc(&eof, to)) return -1;
    if (eof != BLE_EOF) return -1;
    /* Verify CRC over seq+len+payload */
    uint8_t hdr[3 + BLE_MAX_FRAME];
    hdr[0] = seq_hi; hdr[1] = seq_lo; hdr[2] = len;
    for (uint16_t i = 0; i < len; i++) hdr[3 + i] = enc[i];
    uint16_t crc_calc = crc16(hdr, 3 + len);
    uint16_t crc_recv = ((uint16_t)crc_hi << 8) | crc_lo;
    if (crc_calc != crc_recv) return -1;
    /* Decrypt */
    if (g_key_set) {
        uint16_t i = 0; uint8_t ks[16]; uint8_t kidx = 16;
        while (i < len) {
            if (kidx >= 16) { ctr_keystream(ks); kidx = 0; }
            buf[i] = enc[i] ^ ks[kidx];
            i++; kidx++;
        }
    } else {
        for (uint16_t i = 0; i < len; i++) buf[i] = enc[i];
    }
    *out_len = len;
    g_connected = true;
    return 0;
}

bool ble_is_connected(void) { return g_connected; }
void ble_set_connected(bool c) { g_connected = c; }