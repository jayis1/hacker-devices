/*
 * ptp_engine.h — IEEE 1588 PTP frame engine: parsing, BMCA, GM spoof, skew
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef CHRONOS_PHANTOM_PTP_ENGINE_H
#define CHRONOS_PHANTOM_PTP_ENGINE_H

#include <stdint.h>
#include "board.h"

/* --------------------------------------------------------------------- */
/*  PTP message types (IEEE 1588-2008 §13.3.2.1)                          */
/* --------------------------------------------------------------------- */
typedef enum {
    PTP_MSG_SYNC          = 0x0,
    PTP_MSG_DELAY_REQ     = 0x1,
    PTP_MSG_PDELAY_REQ    = 0x2,
    PTP_MSG_PDELAY_RESP   = 0x3,
    PTP_MSG_FOLLOW_UP     = 0x8,
    PTP_MSG_DELAY_RESP    = 0x9,
    PTP_MSG_PDELAY_RESP_FOLLOW_UP = 0xA,
    PTP_MSG_ANNOUNCE      = 0xB,
    PTP_MSG_SIGNALING     = 0xC,
    PTP_MSG_MANAGEMENT    = 0xD,
    PTP_MSG_UNKNOWN       = 0xFF
} ptp_msg_type_t;

/* --------------------------------------------------------------------- */
/*  PTP transport                                                         */
/* --------------------------------------------------------------------- */
#define PTP_TRANSPORT_ETHERNET  0x88F7   /* EtherType                      */
#define PTP_TRANSPORT_UDP        0x11     /* IP protocol field               */

/* --------------------------------------------------------------------- */
/*  PTP header field offsets (in bytes from start of PTP header)          */
/*  Note: PTP header follows 14-byte Ethernet header + optional VLAN tag */
/* --------------------------------------------------------------------- */
#define PTP_HDR_VERSION_PTP      0     /* 1 byte: versionPTP (4 bits)    */
#define PTP_HDR_MSG_TYPE         0     /* 1 byte: msgType (4 bits) + rsv  */
#define PTP_HDR_MSG_LENGTH       2     /* 2 bytes                          */
#define PTP_HDR_DOMAIN_NUMBER    4     /* 1 byte                           */
#define PTP_HDR_FLAG_FIELD       6     /* 2 bytes: flags                    */
#define PTP_HDR_CORRECTION       8     /* 8 bytes: correctionField (signed) */
#define PTP_HDR_CLOCK_ID         20    /* 8 bytes: clockIdentity            */
#define PTP_HDR_SOURCE_PORT_ID   20    /* sourcePortIdentity: 10 bytes      */
#define PTP_HDR_SEQ_ID           30    /* 2 bytes                           */
#define PTP_HDR_CONTROL         32     /* 1 byte                            */
#define PTP_HDR_LOG_MSG_INT     33     /* 1 byte                            */

/* Announce-specific fields (after common header)                        */
#define PTP_ANN_CURRENT_OFFSET   34    /* 12 bytes (48-bit secs + 32-bit ns) */
#define PTP_ANN_ORIG_TS          40    /* 10 bytes timestamp                */
#define PTP_ANN_CUR_UTC_OFFSET    54    /* 2 bytes                          */
#define PTP_ANN_GRANDMASTER_PRIO1 56    /* 1 byte                           */
#define PTP_ANN_GRANDMASTER_CLK_QUAL 57  /* 1 byte (clockClass + accuracy)   */
#define PTP_ANN_GM_CLK_CLASS      57    /* 1 byte                            */
#define PTP_ANN_GM_CLK_ACCURACY  58    /* 1 byte                           */
#define PTP_ANN_GM_CLK_VARIANCE  59    /* 2 bytes (signed)                  */
#define PTP_ANN_GM_PRIO2         61    /* 1 byte                            */
#define PTP_ANN_GM_IDENTITY      62    /* 8 bytes                          */
#define PTP_ANN_STEPS_REMOVED    70    /* 2 bytes                          */
#define PTP_ANN_END              76    /* Announce ends (no TLVs)           */

/* --------------------------------------------------------------------- */
/*  PTP timestamp (48-bit seconds + 32-bit nanoseconds)                  */
/* --------------------------------------------------------------------- */
typedef struct {
    uint64_t seconds;
    uint32_t nanoseconds;
} ptp_timestamp_t;

/* --------------------------------------------------------------------- */
/*  ClockQuality (IEEE 1588-2008 §5.7.3)                                 */
/* --------------------------------------------------------------------- */
typedef struct {
    uint8_t  clock_class;      /* lower = better; 6 = primary reference  */
    uint8_t  clock_accuracy;   /* 0x20 = sub-ns, 0x21 = 1ns ...          */
    int16_t  clock_variance;   /* more negative = better                */
} clock_quality_t;

/* --------------------------------------------------------------------- */
/*  PTP Grandmaster descriptor (observed or spoofed)                     */
/* --------------------------------------------------------------------- */
typedef struct {
    uint8_t          grandmaster_priority1;
    clock_quality_t  grandmaster_clock_quality;
    uint8_t          grandmaster_priority2;
    uint8_t          grandmaster_identity[8];
    uint8_t          domain_number;
    uint16_t         steps_removed;
    ptp_timestamp_t  current_offset;
} ptp_gm_descriptor_t;

/* --------------------------------------------------------------------- */
/*  Skew profile configuration                                            */
/* --------------------------------------------------------------------- */
typedef enum {
    SKEW_STEP      = 0,
    SKEW_RAMP      = 1,
    SKEW_STEALTH   = 2,
    SKEW_JITTER    = 3,
    SKEW_SAWTOOTH  = 4
} skew_profile_t;

typedef struct {
    skew_profile_t profile;
    int64_t         offset_ns;       /* target total offset                */
    int64_t         rate_nsps;       /* rate of change (ns/sec)           */
    uint32_t        jitter_amp_ns;   /* jitter amplitude                  */
    uint32_t        sawtooth_period_ms;
    uint8_t         active;
} skew_config_t;

/* --------------------------------------------------------------------- */
/*  Engine state                                                          */
/* --------------------------------------------------------------------- */
typedef struct {
    op_mode_t          mode;
    ptp_gm_descriptor_t observed_gm;       /* current legitimate GM        */
    ptp_gm_descriptor_t spoofed_gm;       /* our spoofed GM identity       */
    skew_config_t      skew;
    uint8_t            announce_counter;
    uint32_t           frames_captured;
    uint32_t           frames_modified;
    uint8_t            my_clock_identity[8];
    uint16_t           my_sequence_id;
    int64_t            current_skew_ns;
    uint32_t           last_announce_tick;
} ptp_engine_state_t;

/* --------------------------------------------------------------------- */
/*  API                                                                   */
/* --------------------------------------------------------------------- */
void ptp_engine_init(ptp_engine_state_t *st);
void ptp_engine_tick(ptp_engine_state_t *st, uint32_t now_ms);

/* Process an incoming Ethernet frame.
 *  frame   — raw Ethernet frame (from ETH RX)
 *  len     — frame length
 *  ts_ns   — hardware timestamp in nanoseconds
 *  out     — modified frame to transmit (may equal input if passthrough)
 *  out_len — length of output frame
 * Returns 1 if frame should be forwarded, 0 if dropped.
 */
int ptp_engine_rx_frame(ptp_engine_state_t *st,
                         const uint8_t *frame, uint32_t len,
                         uint64_t ts_ns,
                         uint8_t *out, uint32_t *out_len);

/* Generate a PTP Announce frame for rogue-GM mode.
 * Returns frame length, 0 if no frame generated this tick.
 */
uint32_t ptp_engine_generate_announce(ptp_engine_state_t *st,
                                       uint8_t *out, uint32_t max_len,
                                       uint32_t now_ms);

/* Set skew profile from app command. */
void ptp_engine_set_skew(ptp_engine_state_t *st, const skew_config_t *cfg);

/* Set spoofed GM descriptor. */
void ptp_engine_set_spoof_gm(ptp_engine_state_t *st,
                              const ptp_gm_descriptor_t *gm);

/* Compute current skew offset in ns based on profile + elapsed time. */
int64_t ptp_engine_compute_skew(ptp_engine_state_t *st, uint32_t now_ms);

/* Parse a PTP Announce message to extract GM descriptor.
 * Returns 0 on success, -1 if not a valid Announce.
 */
int ptp_engine_parse_announce(const uint8_t *ptp_hdr, uint32_t hdr_len,
                                ptp_gm_descriptor_t *out);

/* Decide whether our spoofed GM would win BMCA against observed GM.
 * Returns 1 if we win, 0 otherwise.
 */
int ptp_engine_would_win_bmca(const ptp_gm_descriptor_t *us,
                                const ptp_gm_descriptor_t *them);

/* Build a PTP Announce frame in `out`. Returns frame length. */
uint32_t ptp_engine_build_announce(const ptp_gm_descriptor_t *gm,
                                     const uint8_t *src_mac,
                                     uint16_t sequence_id,
                                     uint8_t *out, uint32_t max_len);

/* Encode 1 byte into correctionField low bits (covert channel). */
void ptp_engine_covert_encode_byte(uint8_t *correction_field_8, uint8_t byte_val);

/* Decode 1 byte from correctionField low bits (covert channel). */
uint8_t ptp_engine_covert_decode_byte(const uint8_t *correction_field_8);

#endif /* CHRONOS_PHANTOM_PTP_ENGINE_H */