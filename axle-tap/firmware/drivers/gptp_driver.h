/*
 * gptp_driver.h — IEEE 802.1AS (gPTP) grandmaster spoofing engine
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 */
#ifndef AXLETAP_GPTP_DRIVER_H
#define AXLETAP_GPTP_DRIVER_H

#include <stdint.h>

/* gPTP message types (PTPv2 / 802.1AS) */
#define PTP_MSG_SYNC         0x0
#define PTP_MSG_DELAY_REQ    0x1
#define PTP_MSG_PDELAY_REQ   0x2
#define PTP_MSG_PDELAY_RESP  0x3
#define PTP_MSG_FOLLOW_UP    0x8
#define PTP_MSG_DELAY_RESP   0x9
#define PTP_MSG_PDELAY_RESP_FOLLOW_UP 0xA
#define PTP_MSG_ANNOUNCE     0xB
#define PTP_MSG_MANAGEMENT   0xD
#define PTP_MSG_SIGNALING    0xC

/* PTP multicast Ethernet destination */
#define PTP_MAC0 0x01
#define PTP_MAC1 0x80
#define PTP_MAC2 0xC2
#define PTP_MAC3 0x00
#define PTP_MAC4 0x00
#define PTP_MAC5 0x0E

/* gPTP attack mode */
typedef enum {
    GPTP_PASSIVE = 0,    /* Listen only, record observed master */
    GPTP_GRANDMASTER = 1,/* Spoof grandmaster with priority1=0 */
    GPTP_SLIP = 2,       /* Slip time at a rate (ppb) */
    GPTP_FREEZE = 3,     /* Hold time constant */
    GPTP_JUMP = 4        /* Step the time base */
} gptp_mode_t;

/* Observed grandmaster identity */
typedef struct {
    uint8_t  clock_identity[8];
    uint8_t  priority1;
    uint8_t  priority2;
    uint8_t  clock_class;
    uint16_t clock_accuracy;
    uint16_t steps_removed;
    int      is_local_master;
} gptp_master_t;

/* State of the gPTP engine */
typedef struct {
    gptp_mode_t mode;
    int         armed;
    int32_t     slip_ppb;       /* parts per billion rate of time slip */
    int64_t     time_offset_ns; /* accumulated offset from real time */
    uint8_t     domain_number;
    uint16_t    sequence_id;
    gptp_master_t observed_master;
    uint64_t    last_sync_tx_ns;
    uint64_t    last_announce_tx_ns;
} gptp_state_t;

void gptp_init(void);
void gptp_set_mode(gptp_mode_t m);
void gptp_set_slip(int32_t ppb);
void gptp_jump(int64_t delta_ns);
gptp_state_t *gptp_get_state(void);

/* Called for every gPTP frame observed on the link (from bridge mirror). */
void gptp_on_frame(const uint8_t *frame, uint16_t len);

/* Called periodically from the scheduler (gPTP transmit timing). */
void gptp_poll(void);

#endif /* AXLETAP_GPTP_DRIVER_H */