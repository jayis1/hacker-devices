/*
 * drivers/gnss_engine.c — GPS/GNSS spoofing signal generator engine
 *
 * Generates BPSK-modulated C/A code + navigation data for GPS L1 C/A.
 * The output is a packed bitstream at 1.023 Mcps that streams to the
 * Si4463 TX FIFO.  Each navigation bit (20 ms) spans 20460 chips.
 *
 * C/A code generation:
 *   G1 = x^10 + x^3 + 1            (polynomial: 0x0201)
 *   G2 = x^10 + x^9 + x^8 + x^6 + x^3 + x^2 + 1  (polynomial: 0x0386)
 *   PRN_n(t) = G1(t) XOR G2(t + delay[n])
 *
 * Author:  jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

#include "gnss_engine.h"
#include "../registers.h"
#include <string.h>
#include <math.h>

/* ----- C/A code generation -----
 * Two 10-bit LFSRs (G1, G2) running at 1.023 Mcps.
 * G1 feedback: taps at bits 3 and 10 (0-indexed from LSB).
 * G2 feedback: taps at bits 2,3,6,8,9,10.
 */
static uint16_t g1_lfsr;
static uint16_t g2_lfsr;

static uint8_t g1_step(void) {
    /* G1: x^10 + x^3 + 1 → feedback = bit10 XOR bit3 */
    uint8_t fb = ((g1_lfsr >> 9) ^ (g1_lfsr >> 2)) & 1;
    g1_lfsr = ((g1_lfsr << 1) | fb) & 0x3FF;
    return (g1_lfsr >> 0) & 1;  /* output = bit 0 after shift */
}

static uint8_t g2_step(void) {
    /* G2: x^10+x^9+x^8+x^6+x^3+x^2+1
     * feedback = bit10 XOR bit9 XOR bit8 XOR bit6 XOR bit3 XOR bit2
     */
    uint8_t fb = ((g2_lfsr >> 9) ^ (g2_lfsr >> 8) ^ (g2_lfsr >> 7)
                  ^ (g2_lfsr >> 5) ^ (g2_lfsr >> 2) ^ (g2_lfsr >> 1)) & 1;
    g2_lfsr = ((g2_lfsr << 1) | fb) & 0x3FF;
    return (g2_lfsr >> 0) & 1;
}

/* Generate full 1023-chip C/A code for a given PRN.
 * The G2 output is delayed by gps_g2_delay[prn] chips (phase selection).
 */
int gnss_engine_generate_ca_code(sv_channel_t *ch) {
    if (ch->prn < 1 || ch->prn > 32) {
        if (ch->cons == CONSTELLATION_GPS) return -1;
    }

    g1_lfsr = GPS_G1_INIT;
    g2_lfsr = GPS_G2_INIT;

    /* Advance G2 by the PRN-specific delay to select the phase */
    uint16_t delay = 0;
    if (ch->cons == CONSTELLATION_GPS && ch->prn <= 32) {
        delay = gps_g2_delay[ch->prn];
    } else if (ch->cons == CONSTELLATION_BEIDOU) {
        /* BeiDou uses similar but different delays; simplified mapping */
        delay = gps_g2_delay[ch->prn % 32 + 1];
    }

    /* Pre-roll G2 by 'delay' chips */
    for (uint16_t i = 0; i < delay; i++) {
        g2_step();
    }

    /* Generate 1023 chips */
    for (uint16_t i = 0; i < 1023; i++) {
        uint8_t g1_bit = g1_step();
        uint8_t g2_bit = g2_step();
        ch->ca_code[i] = g1_bit ^ g2_bit;
    }

    return 0;
}

/* ----- Navigation data generation -----
 * GPS L1 C/A navigation message: 1500 bits per frame, 50 bps,
 * 30 bits per subframe word, 10 words per subframe, 5 subframes per frame.
 *
 * We generate a simplified but structurally valid nav message:
 *   - Preamble (0x8B = 10001011) at start of each word
 *   - Parity (6 bits) computed via Hamming code
 *   - Ephemeris / clock parameters from loaded scenario
 */
static const uint8_t nav_preamble[8] = {1,0,0,0,1,0,1,1};

static uint8_t nav_parity(const uint8_t *bits29) {
    /* GPS nav parity: D25..D30 from D1..D24 via parity matrix.
     * Simplified parity computation (GPS ICD 20.3.2).
     */
    uint8_t p = 0;
    /* Even parity over specific bit combinations */
    static const uint8_t parity_matrix[6][24] = {
        {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0},
        {0,1,1,1,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,1,1,1,1,1,1,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0},
    };
    for (int i = 0; i < 6; i++) {
        uint8_t bit = 0;
        for (int j = 0; j < 24; j++)
            bit ^= (bits29[j] & parity_matrix[i][j]);
        p = (p << 1) | (bit & 1);
    }
    return p;
}

static void encode_nav_word(uint8_t *out, const uint8_t *data24) {
    /* Encode one 30-bit word: preamble + 24 data + 6 parity
     * (Simplified: in real GPS, D29/D30 from previous word are XORed in.)
     */
    for (int i = 0; i < 8; i++)
        out[i] = nav_preamble[i];
    for (int i = 0; i < 24; i++)
        out[8 + i] = data24[i];
    uint8_t par = nav_parity(data24);
    for (int i = 0; i < 6; i++)
        out[32 + i] = (par >> (5 - i)) & 1;
}

int gnss_engine_generate_nav_data(sv_channel_t *ch) {
    memset(ch->nav_data, 0, NAV_BUFFER_SIZE);
    uint16_t bit_pos = 0;

    /* Subframe 1: TLM word, HOW word, clock correction, health, week number */
    /* TLM: preamble + 14 bits TLM message + 2 reserved + 6 parity */
    uint8_t word_bits[30];
    uint8_t data24[24] = {0};

    /* Subframe 1 — simplified ephemeris clock data */
    for (int sub = 0; sub < 5; sub++) {
        /* 10 words per subframe */
        for (int word = 0; word < 10; word++) {
            memset(data24, 0, 24);
            if (word == 0) {
                /* TLM word: 14-bit TLM message (0s) + 2 reserved + 6 parity */
                /* data24 = 0s, parity computed */
            } else if (word == 1) {
                /* HOW word: bits 1-17 = TOW count, bit 18 = alert, bits 19-22 = subframe ID, bits 23-24 = spare */
                uint32_t tow = (ch->nav_bit_pos / 1500) * 6 + sub * 6;
                for (int b = 0; b < 17; b++)
                    data24[b] = (tow >> (16 - b)) & 1;
                data24[20] = (sub + 1) & 1;
                data24[21] = ((sub + 1) >> 1) & 1;
                data24[22] = ((sub + 1) >> 2) & 1;
            } else if (sub == 0 && word >= 2 && word <= 9) {
                /* Clock correction: af0, af1, af2, toc, etc. (simplified) */
                data24[0] = (ch->prn >> 0) & 1;
                data24[1] = (ch->prn >> 1) & 1;
                data24[2] = (ch->prn >> 2) & 1;
                data24[3] = (ch->prn >> 3) & 1;
                data24[4] = (ch->prn >> 4) & 1;
                /* Fill remaining with pseudo-ephemeris data */
                for (int b = 5; b < 24; b++)
                    data24[b] = (b ^ ch->prn) & 1;
            } else {
                /* Ephemeris data (subframes 2,3) or almanac (4,5) */
                for (int b = 0; b < 24; b++)
                    data24[b] = ((b + word * 7 + sub * 13 + ch->prn) ^ 0x55) & 1;
            }
            encode_nav_word(word_bits, data24);
            for (int b = 0; b < 30 && bit_pos < NAV_BUFFER_SIZE; b++) {
                ch->nav_data[bit_pos++] = word_bits[b];
            }
        }
    }

    ch->nav_bit_pos = 0;
    ch->nav_chip_offset = 0;
    return (int)bit_pos;
}

/* ----- Sample production -----
 * Produce packed bits for the Si4463 TX FIFO.
 * Each GPS bit spans 20 ms = 20460 chips.  We pack 8 bits per byte.
 * Each output bit = C/A code chip XOR nav data bit (BPSK).
 */
uint16_t gnss_engine_produce_samples(gnss_engine_t *eng, uint8_t *buf,
                                      uint16_t max_len) {
    uint16_t produced = 0;
    uint8_t bit_accum = 0;
    uint8_t bit_count = 0;

    while (produced < max_len) {
        /* XOR all active channels (simplified — real impl sums I/Q) */
        uint8_t chip = 0;
        int active = 0;
        for (int i = 0; i < eng->channel_count; i++) {
            sv_channel_t *ch = &eng->channels[i];
            if (!ch->active) continue;
            active = 1;

            /* Get current C/A chip */
            uint16_t code_idx = ch->nav_chip_offset % 1023;
            uint8_t ca_chip = ch->ca_code[code_idx];

            /* Get current nav bit (each bit = 20460 chips) */
            uint16_t nav_bit_idx = ch->nav_bit_pos;
            if (nav_bit_idx >= NAV_BUFFER_SIZE) {
                /* Regenerate nav data for next frame */
                gnss_engine_generate_nav_data(ch);
                nav_bit_idx = 0;
            }
            uint8_t nav_bit = ch->nav_data[nav_bit_idx];

            /* BPSK: chip XOR nav_bit */
            uint8_t bpsk_bit = ca_chip ^ nav_bit;
            chip ^= bpsk_bit;

            /* Advance chip offset */
            ch->nav_chip_offset++;
            if (ch->nav_chip_offset >= 20460) {
                ch->nav_chip_offset = 0;
                ch->nav_bit_pos++;
                if (ch->nav_bit_pos >= NAV_BUFFER_SIZE) {
                    ch->nav_bit_pos = 0;
                    eng->frames_sent++;
                }
            }
        }

        if (!active) {
            /* Output zero when no SVs active (silence) */
            chip = 0;
        }

        /* Pack bit into accumulator (MSB first) */
        bit_accum = (bit_accum << 1) | (chip & 1);
        bit_count++;
        if (bit_count == 8) {
            buf[produced++] = bit_accum;
            bit_accum = 0;
            bit_count = 0;
            eng->total_bits_tx += 8;
        }
    }

    return produced;
}

/* ----- Engine management ----- */

void gnss_engine_init(gnss_engine_t *eng) {
    memset(eng, 0, sizeof(gnss_engine_t));
    eng->state = ENGINE_IDLE;
    eng->sample_rate_hz = 1023000;  /* 1.023 Mcps C/A code rate */
    eng->tx_fifo_threshold = 64;    /* refill when FIFO < 64 bytes */
    eng->lat_deg = 37.7749;        /* default: San Francisco */
    eng->lon_deg = -122.4194;
    eng->alt_m = 0.0;
    eng->gps_week = 2300;
    eng->gps_tow_ms = 0;
    eng->channel_count = 0;
    eng->output_len = 0;
}

int gnss_engine_add_sv(gnss_engine_t *eng, constellation_t cons,
                        uint8_t prn, int8_t power_db) {
    if (eng->channel_count >= MAX_SV_PER_CONSTELLATION) return -1;

    sv_channel_t *ch = &eng->channels[eng->channel_count];
    memset(ch, 0, sizeof(sv_channel_t));
    ch->prn = prn;
    ch->cons = cons;
    ch->power_db = power_db;
    ch->active = 1;
    ch->doppler_hz = 0;

    /* Generate C/A code for this SV */
    gnss_engine_generate_ca_code(ch);

    /* Generate navigation data */
    gnss_engine_generate_nav_data(ch);

    eng->channel_count++;
    return 0;
}

int gnss_engine_remove_sv(gnss_engine_t *eng, uint8_t prn) {
    for (int i = 0; i < eng->channel_count; i++) {
        if (eng->channels[i].prn == prn) {
            /* Shift remaining channels down */
            for (int j = i; j < eng->channel_count - 1; j++)
                eng->channels[j] = eng->channels[j + 1];
            eng->channel_count--;
            memset(&eng->channels[eng->channel_count], 0,
                   sizeof(sv_channel_t));
            return 0;
        }
    }
    return -1;
}

void gnss_engine_set_position(gnss_engine_t *eng, double lat, double lon,
                               double alt) {
    eng->lat_deg = lat;
    eng->lon_deg = lon;
    eng->alt_m = alt;
}

void gnss_engine_set_velocity(gnss_engine_t *eng, double vn, double ve,
                               double vd) {
    eng->vel_n_ms = vn;
    eng->vel_e_ms = ve;
    eng->vel_d_ms = vd;
}

void gnss_engine_set_time(gnss_engine_t *eng, uint32_t week, uint32_t tow_ms) {
    eng->gps_week = week;
    eng->gps_tow_ms = tow_ms;
}

int gnss_engine_start(gnss_engine_t *eng) {
    if (eng->state == ENGINE_TRANSMITTING) return -1;
    if (eng->state != ENGINE_ARMED) return -2;
    eng->state = ENGINE_TRANSMITTING;
    eng->total_bits_tx = 0;
    eng->frames_sent = 0;
    return 0;
}

int gnss_engine_stop(gnss_engine_t *eng) {
    eng->state = ENGINE_IDLE;
    return 0;
}

void gnss_engine_arm(gnss_engine_t *eng) {
    eng->state = ENGINE_ARMED;
}

void gnss_engine_disarm(gnss_engine_t *eng) {
    if (eng->state == ENGINE_TRANSMITTING)
        gnss_engine_stop(eng);
    eng->state = ENGINE_IDLE;
}

int gnss_engine_load_ephemeris(gnss_engine_t *eng, uint8_t prn,
                                const uint8_t *data, uint16_t len) {
    /* Find channel for this PRN and update nav data */
    for (int i = 0; i < eng->channel_count; i++) {
        if (eng->channels[i].prn == prn) {
            uint16_t copy_len = len;
            if (copy_len > NAV_BUFFER_SIZE) copy_len = NAV_BUFFER_SIZE;
            memcpy(eng->channels[i].nav_data, data, copy_len);
            return 0;
        }
    }
    return -1;
}

int gnss_engine_load_scenario(gnss_engine_t *eng, uint32_t flash_offset) {
    /* Read scenario from external QSPI flash.
     * Scenario format:
     *   [4 bytes: scenario ID]
     *   [4 bytes: SV count]
     *   [per SV: 1 byte PRN, 1 byte cons, 1 byte power, 230 bytes ephemeris]
     *   [8 bytes: lat, 8 bytes: lon, 4 bytes: alt]
     *   [4 bytes: GPS week, 4 bytes: TOW]
     */
    (void)flash_offset;
    /* Stub: in production, read from QSPI via octospi_read() */
    return 0;
}