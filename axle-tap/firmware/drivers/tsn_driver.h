/*
 * tsn_driver.h — TSN (802.1CB / 802.1Qbv) manipulation engine
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 */
#ifndef AXLETAP_TSN_DRIVER_H
#define AXLETAP_TSN_DRIVER_H

#include <stdint.h>

/* 802.1CB sequence-tagged frame structure (tag EtherType 0xF1C1) */
typedef struct {
    uint8_t  subtype;       /* 0x01 = RTC */
    uint8_t  reserved;
    uint16_t sequence_number;
    uint8_t  stream_handle[8];
} cb_tag_t;

/* 802.1Qbv gate-control-list entry */
typedef struct {
    uint32_t interval_nsec;
    uint8_t  gate_state;   /* bit per queue 0..7 */
    uint8_t  reserved[3];
} qbv_gcl_entry_t;

/* 802.1CB sequence-number forgery mode */
typedef enum {
    TSN_CB_OFF = 0,
    TSN_CB_FORGE_DUPLICATE,    /* make receiver think frame is duplicate */
    TSN_CB_FORGE_OUTOFORDER,   /* inject out-of-sequence number */
    TSN_CB_DROP originals      /* drop the originals entirely */
} tsn_cb_mode_t;

void tsn_init(void);
void tsn_cb_set_mode(tsn_cb_mode_t m);

/* Observe 802.1CB-tagged frames and apply forge mode. Called from
 * bridge mirror for every observed frame.
 */
void tsn_on_frame(const uint8_t *frame, uint16_t len, uint8_t dir, uint64_t ts);

/* Inject an out-of-window frame into the 802.1Qbv schedule.
 * gate_mask = which queue gates are open for the injected frame.
 */
int tsn_qbv_inject(uint8_t gate_mask, uint8_t priority, const uint8_t *payload, uint16_t plen);

/* Set the SR-class reservation spoof state (SR-A = priority 3, SR-B = priority 2).
 * When enabled, AxleTap injects forged StreamReservation frames to claim
 * bandwidth and starve best-effort traffic.
 */
void tsn_sr_spoof(int enable, uint8_t sr_class);

/* Parse and record the observed 802.1Qbv gate-control list. */
int tsn_qbv_parse_gcl(const uint8_t *frame, uint16_t len);

typedef struct {
    uint64_t cb_frames_seen;
    uint64_t cb_frames_forged;
    uint64_t qbv_injects;
    uint64_t sr_spoofs;
    qbv_gcl_entry_t observed_gcl[64];
    int gcl_entries;
} tsn_stats_t;
void tsn_get_stats(tsn_stats_t *st);

#endif /* AXLETAP_TSN_DRIVER_H */