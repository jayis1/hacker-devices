/*
 * lora-phantom / firmware / types.h
 * Shared type definitions used across main.c and drivers.
 * Author: jayis1
 * License: GPL-2.0
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef LORA_PHANTOM_TYPES_H
#define LORA_PHANTOM_TYPES_H

#include <stdint.h>

/* ---- LoRaWAN frame types ---- */
typedef enum {
    LW_MHDR_JOIN_REQ    = 0x00,
    LW_MHDR_JOIN_ACCEPT = 0x20,
    LW_MHDR_UNCONF_UP   = 0x40,
    LW_MHDR_UNCONF_DOWN = 0x60,
    LW_MHDR_CONF_UP     = 0x80,
    LW_MHDR_CONF_DOWN   = 0xA0,
    LW_MHDR_RFU         = 0xC0,
    LW_MHDR_PROP        = 0xE0
} lw_mhdr_type_t;

typedef struct {
    uint8_t  mhdr;
    uint8_t  dev_addr[4];
    uint8_t  fctrl;
    uint16_t fcnt;
    uint8_t  fopts[15];
    uint8_t  fport;
    uint8_t  payload[242];
    uint16_t payload_len;
    uint8_t  mic[4];
    int      valid_mic;
    int16_t  rssi;
    int8_t   snr;
    uint32_t freq_hz;
    uint8_t  sf;
    uint8_t  bw;
    uint32_t ts_ms;
} lw_frame_t;

typedef struct {
    uint8_t join_eui[8];
    uint8_t dev_eui[8];
    uint16_t dev_nonce;
    uint8_t  mic[4];
    int16_t  rssi;
    int8_t   snr;
    uint32_t freq_hz;
    uint8_t  sf;
    uint32_t ts_ms;
} lw_join_req_t;

/* ---- Fuzzer modes ---- */
typedef enum {
    FUZZ_BAD_HEADER_CRC   = 0,
    FUZZ_INVALID_CR       = 1,
    FUZZ_PHANTOM_HEADER   = 2,
    FUZZ_IMPLICIT_MISMATCH= 3,
    FUZZ_OVERSIZED_PAYLOAD= 4,
    FUZZ_RANDOM_BYTES     = 5
} fuzz_mode_t;

/* ---- Spectrum scan channel result ---- */
typedef struct {
    uint32_t freq_hz;
    int16_t rssi;
    uint8_t activity;
    uint32_t hits;
} scan_channel_t;

/* ---- Function prototypes (implemented in drivers) ---- */
/* lorawan.c */
int  lw_parse_frame(const uint8_t *buf, uint16_t len, lw_frame_t *out);
int  lw_parse_join_request(const uint8_t *buf, uint16_t len, lw_join_req_t *out);
int  lw_decrypt_payload(const uint8_t *skey, uint32_t dev_addr, uint8_t dir,
                        uint16_t fcnt, uint8_t *payload, uint16_t len);
int  lw_compute_mic(const uint8_t *nwk_skey, const uint8_t *msg, uint32_t len, uint8_t mic[4]);
int  lw_verify_mic(const uint8_t *nwk_skey, const uint8_t *msg, uint32_t len, const uint8_t mic[4]);
int  lw_build_join_accept(const uint8_t *nwk_key, const lw_join_req_t *jr,
                          uint32_t dev_addr, uint8_t *out, uint16_t *out_len);
int  lw_derive_session_keys(const uint8_t *nwk_key, const uint8_t *app_key,
                            const lw_join_req_t *jr, uint32_t dev_addr,
                            uint8_t nwk_skey[16], uint8_t app_skey[16]);

/* keysearch.c */
int  keysearch_dict(const lw_join_req_t *jr, const uint8_t (*dict)[16], uint32_t dict_len,
                    uint8_t out_key[16], uint32_t *trials);
int  keysearch_bruteforce_seq(const lw_join_req_t *jr, uint64_t start, uint64_t end,
                              uint8_t out_key[16], uint64_t *trials);

/* replay.c */
void replay_queue_reset(void);
int  replay_queue_push(const uint8_t *frame, uint16_t len, uint32_t freq, uint8_t sf, uint8_t bw);
int  replay_queue_pop(uint8_t *frame, uint16_t *len, uint32_t *freq, uint8_t *sf, uint8_t *bw);
int  replay_send_next(uint32_t freq_override, int8_t power_dbm, uint32_t timeout_ms);
int  replay_set_counter_override(uint16_t fcnt);

/* injector.c */
int  injector_send_downlink(uint32_t dev_addr, uint8_t fctrl, uint16_t fcnt,
                            uint8_t fport, const uint8_t *payload, uint16_t plen,
                            const uint8_t nwk_skey[16], const uint8_t app_skey[16],
                            int confirmed, uint32_t freq_hz, uint8_t sf, int8_t power_dbm);
int  injector_send_join_accept(const lw_join_req_t *jr, const uint8_t nwk_key[16],
                               uint32_t dev_addr, uint32_t freq_hz, uint8_t sf,
                               int8_t power_dbm, uint32_t delay_ms);

/* fuzzer.c */
int  fuzzer_tx(fuzz_mode_t mode, uint32_t freq_hz, uint8_t sf, uint8_t bw,
               int8_t power_dbm, uint16_t count, uint32_t delay_ms);

/* spectrum.c */
int  spectrum_scan(const uint32_t *freqs, uint8_t n_freqs, uint8_t sf, uint8_t bw,
                   uint32_t dwell_ms, scan_channel_t *out, uint8_t out_max);

/* AES (aes_hw.c) */
void aes128_ecb_pub(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]);
void aes_cmac_pub(const uint8_t key[16], const uint8_t *msg, uint32_t len, uint8_t out[16]);

#endif /* LORA_PHANTOM_TYPES_H */