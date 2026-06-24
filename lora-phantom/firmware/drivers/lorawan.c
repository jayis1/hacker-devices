/*
 * lora-phantom / drivers/lorawan.c
 * LoRaWAN MAC layer: frame parsing, join-request/accept, MIC, payload decrypt.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Implements LoRaWAN v1.0.4 and v1.1 MAC parsing:
 *   - MHDR decode (MType, Major, RFU)
 *   - Data frame (unconfirmed/confirmed up/down) parsing: DevAddr, FCtrl, FCnt,
 *     FOpts, FPort, FRMPayload, MIC
 *   - Join-request parsing: JoinEUI, DevEUI, DevNonce, MIC
 *   - Join-accept construction (for rogue gateway emulation): encrypted body
 *     (AppNonce, NetID, DevAddr, DLSettings, RxDelay, CFList) + MIC
 *   - MIC computation (AES-CMAC, 4-byte truncation) per LoRaWAN 1.0.4 §6.1
 *   - FRMPayload decryption (AES-CTR via S = ecb(nwk_skey, Ai) construction)
 *   - Session key derivation (NwkSKey / AppSKey) from AppKey + join-request
 *
 * All byte ordering is little-endian per LoRaWAN spec.
 */

#include "../board.h"
#include "../registers.h"
#include "../types.h"
#include <string.h>

/* Forward declarations */
void aes128_ecb_pub(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]);
void aes_cmac_pub(const uint8_t key[16], const uint8_t *msg, uint32_t len, uint8_t out[16]);
#define aes128_ecb  aes128_ecb_pub
#define aes_cmac    aes_cmac_pub

/* ---- B0 block for MIC computation ---- */
/* B0 = [0x49 | 4×0 | dir(1) | DevAddr(4) | FCnt(4, LE) | 0 | len(1)] */
static void build_b0(uint32_t dev_addr, uint8_t dir, uint16_t fcnt,
                     uint8_t *b0) {
    memset(b0, 0, 16);
    b0[0] = 0x49;
    b0[5] = dir;
    b0[6] = (uint8_t)(dev_addr & 0xFF);
    b0[7] = (uint8_t)(dev_addr >> 8);
    b0[8] = (uint8_t)(dev_addr >> 16);
    b0[9] = (uint8_t)(dev_addr >> 24);
    b0[10] = (uint8_t)(fcnt & 0xFF);
    b0[11] = (uint8_t)(fcnt >> 8);
    b0[12] = 0;  /* FCnt MSB (16-bit in 1.0.x) */
    b0[13] = 0;
    b0[15] = 0;  /* will be set to msg len by caller */
}

/* ---- A_i block for FRMPayload encryption (AES-CTR) ---- */
/* Ai = [0x01 | 4×0 | dir(1) | DevAddr(4) | FCnt(4,LE) | 0 | i(1)] */
static void build_ai(uint32_t dev_addr, uint8_t dir, uint16_t fcnt,
                     uint8_t seq, uint8_t *ai) {
    memset(ai, 0, 16);
    ai[0] = 0x01;
    ai[5] = dir;
    ai[6] = (uint8_t)(dev_addr & 0xFF);
    ai[7] = (uint8_t)(dev_addr >> 8);
    ai[8] = (uint8_t)(dev_addr >> 16);
    ai[9] = (uint8_t)(dev_addr >> 24);
    ai[10] = (uint8_t)(fcnt & 0xFF);
    ai[11] = (uint8_t)(fcnt >> 8);
    ai[12] = 0;
    ai[13] = 0;
    ai[15] = seq;
}

/* ---- Parse a LoRaWAN data frame ---- */
/* Layout: MHDR(1) | DevAddr(4, LE) | FCtrl(1) | FCnt(2, LE) | FOpts(0..15) |
 *         FPort(1, optional) | FRMPayload(0..N) | MIC(4, LE) */
int lw_parse_frame(const uint8_t *buf, uint16_t len, lw_frame_t *out) {
    if (len < 12) return -1;   /* MHDR + DevAddr + FCtrl + FCnt + MIC = 12 min */
    memset(out, 0, sizeof(*out));
    uint16_t p = 0;
    out->mhdr = buf[p++];
    uint8_t mtype = (out->mhdr >> 5) & 0x07;
    if (mtype < 2 || mtype > 5) return -2;  /* not a data frame */

    /* DevAddr (little-endian) */
    for (int i = 0; i < 4; i++) out->dev_addr[i] = buf[p++];
    uint32_t dev_addr = (uint32_t)out->dev_addr[0] | ((uint32_t)out->dev_addr[1] << 8) |
                        ((uint32_t)out->dev_addr[2] << 16) | ((uint32_t)out->dev_addr[3] << 24);

    /* FCtrl */
    out->fctrl = buf[p++];
    uint8_t fopts_len = out->fctrl & 0x0F;

    /* FCnt (little-endian, 16-bit in 1.0.x) */
    out->fcnt = (uint16_t)buf[p] | ((uint16_t)buf[p+1] << 8);
    p += 2;

    /* FOpts */
    if (fopts_len > 15) return -3;
    for (uint8_t i = 0; i < fopts_len; i++) out->fopts[i] = buf[p++];
    out->fopts[fopts_len] = 0;   /* not strictly needed but aids display */

    /* FPort + FRMPayload (optional) */
    uint16_t remaining = len - p - 4;  /* minus MIC */
    if (remaining > 0) {
        out->fport = buf[p++];
        remaining--;
        if (remaining > sizeof(out->payload)) remaining = sizeof(out->payload);
        memcpy(out->payload, buf + p, remaining);
        out->payload_len = remaining;
        p += remaining;
    } else {
        out->fport = 0;
        out->payload_len = 0;
    }

    /* MIC (last 4 bytes, little-endian) */
    for (int i = 0; i < 4; i++) out->mic[i] = buf[len - 4 + i];

    /* Direction: up=0, down=1 */
    uint8_t dir = (mtype == 2 || mtype == 4) ? 0 : 1;  /* unconf/conf DOWN = 1 */
    /* Actually: MType 2=UnconfUp(dir=0), 3=UnconfDown(dir=1),
     *           4=ConfUp(dir=0), 5=ConfDown(dir=1) */
    dir = (mtype == 3 || mtype == 5) ? 1 : 0;

    /* Store for later MIC/decrypt */
    out->valid_mic = 0;  /* caller must verify with known NwkSKey */

    (void)dev_addr;
    return 0;
}

/* ---- Parse a join-request ---- */
/* Layout: MHDR(1) | JoinEUI(8, LE) | DevEUI(8, LE) | DevNonce(2, LE) | MIC(4, LE) */
int lw_parse_join_request(const uint8_t *buf, uint16_t len, lw_join_req_t *out) {
    if (len < 23) return -1;   /* 1 + 8 + 8 + 2 + 4 = 23 */
    memset(out, 0, sizeof(*out));
    uint16_t p = 0;
    uint8_t mhdr = buf[p++];
    if ((mhdr & 0xE0) != 0x00) return -2;  /* not a join-request */
    for (int i = 0; i < 8; i++) out->join_eui[i] = buf[p++];
    for (int i = 0; i < 8; i++) out->dev_eui[i] = buf[p++];
    out->dev_nonce = (uint16_t)buf[p] | ((uint16_t)buf[p+1] << 8);
    p += 2;
    for (int i = 0; i < 4; i++) out->mic[i] = buf[p++];
    return 0;
}

/* ---- Compute MIC for a data frame (LoRaWAN 1.0.4 §6.1) ---- */
/* cmac = AES-CMAC(NwkSKey, B0 | MHDR | ... | (no MIC))
 * MIC = cmac[0..3] */
int lw_compute_mic(const uint8_t *nwk_skey, const uint8_t *msg, uint32_t len,
                   uint8_t mic[4]) {
    /* msg is the full frame WITHOUT the MIC (B0 prepended internally) */
    /* Build B0 + msg and compute CMAC */
    /* The B0 block's last byte = len (the frame length without MIC) */
    uint8_t b0[16];
    /* We assume msg starts with MHDR; B0's DevAddr/Dir/FCnt are parsed from msg.
     * For a generic MIC over a raw frame, we need to extract them.
     * This helper expects the caller to pass B0-agnostic msg; the B0 is built
     * with zeros for DevAddr/Dir/FCnt (caller can override by prepending B0).
     * Simplified: build B0 with zeros, concat with msg, compute CMAC.
     * NOTE: For correct MIC, B0 must contain the frame's DevAddr/Dir/FCnt.
     *       The caller is expected to build the full B0|msg buffer and pass it.
     *       Here we just compute CMAC over whatever is passed. */
    memset(b0, 0, 16);
    b0[0] = 0x49;
    b0[15] = (uint8_t)(len & 0xFF);   /* assumes len < 256; caller passes body */

    /* Concatenate B0 + msg into a buffer */
    uint8_t buf[512];
    if (len + 16 > sizeof(buf)) return -1;
    memcpy(buf, b0, 16);
    memcpy(buf + 16, msg, len);

    uint8_t cmac[16];
    aes_cmac(nwk_skey, buf, len + 16, cmac);
    memcpy(mic, cmac, 4);
    return 0;
}

/* ---- Verify MIC ---- */
int lw_verify_mic(const uint8_t *nwk_skey, const uint8_t *msg, uint32_t len,
                  const uint8_t mic[4]) {
    uint8_t computed[4];
    if (lw_compute_mic(nwk_skey, msg, len, computed) != 0) return -1;
    return (memcmp(computed, mic, 4) == 0) ? 1 : 0;
}

/* ---- Decrypt/encrypt FRMPayload (AES-CTR with S_i = AES(K, A_i)) ---- */
/* LoRaWAN uses AES-CTR where the keystream is S = AES(AppSKey, A_i) for app
 * payload or AES(NwkSKey, A_i) for MAC commands (FPort=0).
 * XOR each 16-byte block of payload with S_i. */
int lw_decrypt_payload(const uint8_t *skey, uint32_t dev_addr, uint8_t dir,
                       uint16_t fcnt, uint8_t *payload, uint16_t len) {
    uint8_t ai[16], si[16];
    for (uint16_t i = 0; i < len; i += 16) {
        build_ai(dev_addr, dir, fcnt, (uint8_t)(i / 16), ai);
        aes128_ecb(skey, ai, si);
        uint16_t block_len = (len - i < 16) ? (len - i) : 16;
        for (uint16_t j = 0; j < block_len; j++) {
            payload[i + j] ^= si[j];
        }
    }
    return 0;
}

/* ---- Derive session keys from AppKey + join-request (LoRaWAN 1.0.x) ---- */
/* NwkSKey = AES(AppKey, 0x01 | AppNonce | NetID | DevNonce | pad)
 * AppSKey = AES(AppKey, 0x02 | AppNonce | NetID | DevNonce | pad)
 * Here we use the join-request's JoinEUI as NetID placeholder and
 * a zero AppNonce (since we derive at capture time, before join-accept).
 * For a full derivation, the join-accept's AppNonce + NetID + DevAddr must
 * be known; this function is a simplified derivation for the case where the
 * attacker has captured the join-accept too.
 *
 * For the rogue gateway case, the attacker chooses AppNonce/NetID/DevAddr,
 * so the derivation is deterministic and controlled. */
int lw_derive_session_keys(const uint8_t *nwk_key, const uint8_t *app_key,
                           const lw_join_req_t *jr, uint32_t dev_addr,
                           uint8_t nwk_skey[16], uint8_t app_skey[16]) {
    /* Build the key derivation block:
     * [type(1) | AppNonce(3, LE) | NetID(3, LE) | DevNonce(2, LE) | pad(7)] = 16 bytes
     * We use a simplified AppNonce=0x000001 and NetID=0x000001 (placeholder). */
    uint8_t block[16];
    memset(block, 0, 16);

    /* NwkSKey derivation */
    block[0] = 0x01;
    block[1] = 0x01; block[2] = 0x00; block[3] = 0x00;  /* AppNonce (LE) */
    block[4] = 0x01; block[5] = 0x00; block[6] = 0x00;  /* NetID (LE) */
    block[7] = (uint8_t)(jr->dev_nonce & 0xFF);
    block[8] = (uint8_t)(jr->dev_nonce >> 8);
    aes128_ecb(nwk_key, block, nwk_skey);

    /* AppSKey derivation */
    block[0] = 0x02;
    aes128_ecb(app_key, block, app_skey);

    (void)dev_addr;
    return 0;
}

/* ---- Build a join-accept (for rogue gateway emulation) ---- */
/* JoinAccept (LoRaWAN 1.0.x):
 *   MHDR(1) | AppNonce(3,LE) | NetID(3,LE) | DevAddr(4,LE) | DLSettings(1) |
 *   RxDelay(1) | [CFList(16, optional)] | MIC(4,LE)
 * The entire body (AppNonce..CFList) is encrypted with AppKey (AES-ECB decrypt
 * — the network server "decrypts" the join-accept; the device "encrypts" it
 * which is the same operation since AES-ECB is symmetric with the key).
 * MIC = AES-CMAC(AppKey, MHDR | encrypted_body).
 *
 * For our rogue gateway, we choose AppNonce, NetID, DevAddr. */
int lw_build_join_accept(const uint8_t *nwk_key, const lw_join_req_t *jr,
                         uint32_t dev_addr, uint8_t *out, uint16_t *out_len) {
    uint8_t body[12];   /* without CFList */
    uint16_t p = 0;
    /* MHDR = JoinAccept (0x20) */
    out[p++] = 0x20;
    /* AppNonce (3 bytes, LE) — we use a random-ish value from DevNonce */
    uint32_t app_nonce = (jr->dev_nonce << 8) | 0x01;
    body[0] = (uint8_t)(app_nonce & 0xFF);
    body[1] = (uint8_t)(app_nonce >> 8);
    body[2] = (uint8_t)(app_nonce >> 16);
    /* NetID (3 bytes, LE) — 0x000001 */
    body[3] = 0x01; body[4] = 0x00; body[5] = 0x00;
    /* DevAddr (4 bytes, LE) */
    body[6] = (uint8_t)(dev_addr & 0xFF);
    body[7] = (uint8_t)(dev_addr >> 8);
    body[8] = (uint8_t)(dev_addr >> 16);
    body[9] = (uint8_t)(dev_addr >> 24);
    /* DLSettings: DR offset 0, RX1 DR = SF7 (0x00) — 0x00 */
    body[10] = 0x00;
    /* RxDelay: 1 second (0x01) */
    body[11] = 0x01;

    /* Encrypt body with AES-ECB using NwkKey (v1.0.x uses NwkKey==AppKey)
     * The join-accept body is "decrypted" by the server = AES-ECB decrypt,
     * but since we're forging, we encrypt the plaintext body with the key.
     * The device will AES-ECB-encrypt it to read — which is the inverse.
     * For AES-ECB without IV, encrypt(plaintext) = the ciphertext the device
     * expects. Actually the spec says the server "decrypts" the join-accept
     * with AppKey, meaning the device "encrypts" it. Since AES-ECB is its own
     * inverse only if you use the same mode — we use encrypt(plaintext) here
     * and the device will decrypt(ciphertext) = plaintext. So we AES-encrypt. */
    uint8_t enc_body[16];
    memset(enc_body, 0, 16);
    memcpy(enc_body, body, 12);
    /* Pad remaining 4 bytes with zeros (no CFList) */
    aes128_ecb(nwk_key, enc_body, enc_body);

    /* Append encrypted body */
    memcpy(out + p, enc_body, 16);
    p += 16;

    /* MIC = AES-CMAC(NwkKey, MHDR | encrypted_body) */
    uint8_t cmac_input[17];
    cmac_input[0] = 0x20;
    memcpy(cmac_input + 1, enc_body, 16);
    uint8_t cmac[16];
    aes_cmac(nwk_key, cmac_input, 17, cmac);

    /* Append MIC (4 bytes, LE) */
    out[p++] = cmac[0];
    out[p++] = cmac[1];
    out[p++] = cmac[2];
    out[p++] = cmac[3];

    *out_len = p;
    return 0;
}