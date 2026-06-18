/*
 * drivers/gnss_engine.h — GPS/GNSS spoofing signal generator engine
 *
 * Generates BPSK-modulated C/A code + navigation data stream for up to
 * 12 simultaneous satellites.  Outputs a bit stream that feeds the Si4463
 * TX FIFO at 1.023 Mcps (the GPS C/A code chipping rate).
 *
 * Supports:
 *   - GPS L1 C/A (PRN 1-32) with full C/A code generation
 *   - Galileo E1 (BOC(1,1)) simplified
 *   - GLONASS L1 (FDMA, channel-specific frequency offset)
 *   - BeiDou B1 (similar to GPS C/A)
 *
 * Author:  jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

#ifndef GNSS_PHANTOM_ENGINE_H
#define GNSS_PHANTOM_ENGINE_H

#include <stdint.h>
#include "../board.h"

#define MAX_SV_PER_CONSTELLATION 12
#define NAV_BUFFER_SIZE          2048

/* Constellation types */
typedef enum {
    CONSTELLATION_GPS = 0,
    CONSTELLATION_GLONASS = 1,
    CONSTELLATION_GALILEO = 2,
    CONSTELLATION_BEIDOU = 3,
} constellation_t;

/* Satellite vehicle descriptor */
typedef struct {
    uint8_t prn;              /* PRN number */
    constellation_t cons;     /* constellation */
    uint8_t ca_code[1023];    /* pre-computed C/A code (1 bit per chip) */
    uint8_t nav_data[NAV_BUFFER_SIZE]; /* navigation message bits */
    uint16_t nav_bit_pos;    /* current bit position in nav_data */
    uint16_t nav_chip_offset;/* current chip offset within current bit */
    int8_t power_db;         /* relative power offset */
    uint8_t active;          /* 1 = transmit this SV */
    uint32_t doppler_hz;     /* Doppler frequency offset */
} sv_channel_t;

/* Spoofing engine state */
typedef enum {
    ENGINE_IDLE = 0,
    ENGINE_ARMED = 1,
    ENGINE_TRANSMITTING = 2,
    ENGINE_RX_CALIBRATE = 3,
} engine_state_t;

typedef struct {
    sv_channel_t channels[MAX_SV_PER_CONSTELLATION];
    uint8_t channel_count;
    engine_state_t state;
    uint32_t sample_rate_hz;   /* output sample rate */
    uint32_t tx_fifo_threshold; /* refill threshold */
    uint32_t total_bits_tx;     /* total bits transmitted */
    uint32_t frames_sent;       /* complete nav frames sent */
    /* Trajectory simulation */
    double lat_deg;
    double lon_deg;
    double alt_m;
    double vel_n_ms;
    double vel_e_ms;
    double vel_d_ms;
    uint32_t gps_week;
    uint32_t gps_tow_ms;
    /* Output buffer for DMA to Si4463 */
    uint8_t output_buf[512];
    uint16_t output_len;
} gnss_engine_t;

/* C/A code G2 tap selection table for GPS PRNs 1-32.
 * Each entry is the G2 delay (in chips) that creates the unique PRN code.
 * PRN[i] = G1(t) XOR G2(t + delay[i])
 */
static const uint16_t gps_g2_delay[33] = {
    0,    /* index 0 unused */
    5,    6,    7,    8,    17,   18,   139,  140,  141,  5,    /* PRN 1-10 */
    6,    15,   62,   155,  156,  157,  118,  24,   25,   26,   /* PRN 11-20 */
    57,   92,   132,  140,  15,   42,   89,   99,   124,  126,  /* PRN 21-30 */
    127,  28                                        /* PRN 31-32 */
};

/* GPS G2 tap configuration: G2 = x^10 + x^3 + x^G2_taps
 * The standard G2 generator taps: x^10+x^3+x^G2_shifts
 */
#define GPS_G1_INIT  0x03FF  /* G1 shift reg init: all ones (10-bit) */
#define GPS_G2_INIT  0x03FF  /* G2 shift reg init: all ones (10-bit) */

/* API */
void gnss_engine_init(gnss_engine_t *eng);
int  gnss_engine_add_sv(gnss_engine_t *eng, constellation_t cons,
                         uint8_t prn, int8_t power_db);
int  gnss_engine_remove_sv(gnss_engine_t *eng, uint8_t prn);
void gnss_engine_set_position(gnss_engine_t *eng, double lat, double lon,
                               double alt);
void gnss_engine_set_velocity(gnss_engine_t *eng, double vn, double ve,
                               double vd);
void gnss_engine_set_time(gnss_engine_t *eng, uint32_t week, uint32_t tow_ms);
int  gnss_engine_generate_ca_code(sv_channel_t *ch);
int  gnss_engine_generate_nav_data(sv_channel_t *ch);
uint16_t gnss_engine_produce_samples(gnss_engine_t *eng, uint8_t *buf,
                                      uint16_t max_len);
int  gnss_engine_start(gnss_engine_t *eng);
int  gnss_engine_stop(gnss_engine_t *eng);
void gnss_engine_arm(gnss_engine_t *eng);
void gnss_engine_disarm(gnss_engine_t *eng);

/* Ephemeris loading from external flash */
int  gnss_engine_load_ephemeris(gnss_engine_t *eng, uint8_t prn,
                                const uint8_t *data, uint16_t len);
int  gnss_engine_load_scenario(gnss_engine_t *eng, uint32_t flash_offset);

#endif /* GNSS_PHANTOM_ENGINE_H */