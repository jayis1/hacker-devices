/*
 * pd_engine.h — USB-PD BMC engine on FUSB302B + state machine + fuzzer
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef EMBER_PD_ENGINE_H
#define EMBER_PD_ENGINE_H

#include <stdint.h>
#include "board.h"

/* Decoded PD message header (16-bit) */
typedef struct {
    uint8_t  msg_type;      /* bits 0..4  */
    uint8_t  port_data_role;/* bit 5  (0=data UFP, 1=data DFP) */
    uint8_t  spec_rev;      /* bits 6..7 */
    uint8_t  port_power_role;/* bit 8 (0=sink, 1=source) */
    uint8_t  msg_id;        /* bits 9..13 */
    uint8_t  numobj;        /* bits 14..15 */
    uint16_t raw;
} pd_header_t;

/* A captured PD frame */
typedef struct {
    uint32_t       ts_ms;       /* capture timestamp (ms) */
    uint8_t        sof;         /* SOP / SOP' / SOP'' */
    pd_header_t    hdr;
    uint8_t        objs[28];    /* up to 7 × 4-byte data objects */
    uint8_t        obj_len;
    uint8_t        crc_ok;
} pd_frame_t;

/* Fuzz mutation profile */
typedef enum {
    FUZZ_PROF_HEADER   = 0,   /* flip/corrupt header bitfields */
    FUZZ_PROF_PDO      = 1,   /* corrupt PDO voltage/current fields */
    FUZZ_PROF_TIMING   = 2,   /* violate spec timers */
    FUZZ_PROF_CHUNK    = 3,   /* malformed chunked extended messages */
    FUZZ_PROF_COUNT
} fuzz_profile_t;

/* Fuzz campaign config */
typedef struct {
    uint32_t       count;       /* total frames to send */
    uint32_t       sent;        /* frames sent so far */
    uint32_t       seed;        /* PRNG seed */
    fuzz_profile_t profile;
    uint8_t        running;
    uint32_t       crash_cnt;   /* observed DUT resets/timeouts */
} fuzz_campaign_t;

/* Public API */

void pd_engine_init(void);

/* Start passive sniffing (sets FUSB302 to listen on both CC). */
void pd_sniff_start(void);
void pd_sniff_stop(void);

/* Pop a captured frame from the ring. Returns 0 if none, 1 if got one. */
int  pd_get_captured(pd_frame_t *out);

/* Send a raw PD message (SOP/SOP'/SOP''). */
int  pd_send(uint8_t sof, const pd_header_t *hdr, const uint8_t *objs,
             uint8_t obj_len);

/* Build a standard header. */
uint16_t pd_build_header(uint8_t msg_type, uint8_t spec_rev,
                         uint8_t port_power_role, uint8_t port_data_role,
                         uint8_t msg_id, uint8_t numobj);

/* Decode a raw 16-bit header. */
void pd_decode_header(uint16_t raw, pd_header_t *out);

/* Source Capabilities advertisement (spoof mode). */
int  pd_advertise_src(uint16_t *pdos, uint8_t cnt);

/* Request a specific voltage/current from a real source (sink masquerade). */
int  pd_request(uint16_t mv, uint16_t ma);

/* Send Hard Reset on SOP. */
int  pd_hard_reset(void);

/* Force a role swap. */
int  pd_role_swap(void);

/* Dead-battery emulation: toggle Rp on CC lines. */
void pd_dead_battery(int on);

/* Fuzzer */
void fuzz_start(fuzz_campaign_t *cfg);
void fuzz_stop(void);
void fuzz_tick(void);   /* called from main scheduler */

/* PRNG (xorshift32) — deterministic, seedable */
uint32_t pd_rand(void);
void     pd_srand(uint32_t s);

#endif /* EMBER_PD_ENGINE_H */
/* end of file — author: jayis1 */