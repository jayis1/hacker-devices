/*
 * ntp_engine.h — NTP responder, skew injection, covert encoding
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef CHRONOS_PHANTOM_NTP_ENGINE_H
#define CHRONOS_PHANTOM_NTP_ENGINE_H

#include <stdint.h>
#include "ptp_engine.h"   /* for skew_config_t */

/* --------------------------------------------------------------------- */
/*  NTP packet layout (RFC 5905)                                          */
/* --------------------------------------------------------------------- */
typedef struct {
    uint8_t  li_vn_mode;        /* LI(2) | VN(3) | Mode(3)                  */
    uint8_t  stratum;           /* 0 = unspec, 1 = primary, 2-15 = valid   */
    int8_t   poll;              /* log2 poll interval                       */
    int8_t   precision;         /* log2 precision                           */
    uint32_t root_delay;        /* 32-bit signed fixed point               */
    uint32_t root_dispersion;   /* 32-bit unsigned fixed point             */
    uint8_t  ref_id[4];         /* reference ID                            */
    uint64_t ref_ts_sec;        /* reference timestamp seconds (high 32)   */
    uint32_t ref_ts_frac;       /* reference timestamp fraction             */
    uint64_t orig_ts_sec;
    uint32_t orig_ts_frac;
    uint64_t recv_ts_sec;
    uint32_t recv_ts_frac;
    uint64_t xmit_ts_sec;
    uint32_t xmit_ts_frac;
} ntp_packet_t;

/* NTP modes */
#define NTP_MODE_RESERVED   0
#define NTP_MODE_SYMMETRIC_ACTIVE 1
#define NTP_MODE_SYMMETRIC_PASSIVE 2
#define NTP_MODE_CLIENT    3
#define NTP_MODE_SERVER    4
#define NTP_MODE_BROADCAST 5
#define NTP_MODE_CONTROL   6
#define NTP_MODE_PRIVATE   7

/* NTP versions */
#define NTP_VERSION_3      3
#define NTP_VERSION_4      4

/* Stratum values for spoofing */
#define NTP_STRATUM_UNSPEC   0
#define NTP_STRATUM_PRIMARY  1   /* GNSS / atomic */
#define NTP_STRATUM_SECONDARY 2

/* Reference IDs for stratum-1 spoofing */
#define NTP_REFID_GNSS       0x47505300u   /* "GPS\0" */
#define NTP_REFID_ATOM       0x41434F31u   /* "ACO1" — atomic */
#define NTP_REFID_PPS        0x50505300u   /* "PPS\0" */

/* --------------------------------------------------------------------- */
/*  NTP engine state                                                      */
/* --------------------------------------------------------------------- */
typedef struct {
    uint8_t          spoof_stratum;
    uint32_t         spoof_ref_id;
    int64_t          skew_ns;           /* applied to xmit timestamp */
    skew_config_t    skew_cfg;
    uint32_t         requests_received;
    uint32_t         responses_sent;
    uint16_t         covert_pending;    /* bytes left in covert buffer */
    uint16_t         covert_idx;
    uint8_t          covert_buf[64];
} ntp_engine_state_t;

/* --------------------------------------------------------------------- */
/*  API                                                                   */
/* --------------------------------------------------------------------- */
void ntp_engine_init(ntp_engine_state_t *st);
void ntp_engine_set_skew(ntp_engine_state_t *st, const skew_config_t *cfg);
void ntp_engine_set_spoof(ntp_engine_state_t *st, uint8_t stratum,
                           uint32_t ref_id);

/* Process an NTP frame (UDP port 123).
 * Returns 1 if a response should be sent, 0 otherwise.
 * If 1, `response` is filled and `*resp_len` set.
 */
int ntp_engine_process(ntp_engine_state_t *st,
                       const uint8_t *ntp_payload, uint32_t len,
                       uint64_t recv_ts_ns,
                       uint8_t *response, uint32_t *resp_len,
                       uint32_t now_ms);

/* Queue a byte for covert-channel transmission via root_delay/dispersion. */
void ntp_engine_covert_queue(ntp_engine_state_t *st, uint8_t b);

/* Encode a byte into NTP root_delay (16 bits usable). */
void ntp_engine_covert_encode_root_delay(uint32_t *root_delay, uint8_t b);

/* Decode a byte from NTP root_delay. */
uint8_t ntp_engine_covert_decode_root_delay(uint32_t root_delay);

#endif /* CHRONOS_PHANTOM_NTP_ENGINE_H */