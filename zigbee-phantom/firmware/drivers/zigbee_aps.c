/*
 * drivers/zigbee_aps.c — Zigbee APS layer parser, Transport-Key dissector,
 * and ZCL command builders.
 * Author: jayis1
 * License: GPL-2.0
 *
 * The APS (Application Support Sublayer) is the layer above 802.15.4 MAC and
 * below ZCL. It carries Zigbee's key-transport, device-update and command
 * frames. This file dissects APS payloads captured by the MAC layer and
 * extracts the encrypted network-key blob from Transport-Key frames — the
 * single most important capture in the key-recovery attack chain.
 */
#include "zigbee_aps.h"
#include "../board.h"
#include <string.h>

static inline uint16_t rd16(const uint8_t *p){ return (uint16_t)p[0] | ((uint16_t)p[1]<<8); }
static inline void wr16(uint8_t *p, uint16_t v){ p[0]=v&0xFF; p[1]=(v>>8)&0xFF; }

/* Parse APS frame from MAC data payload.
 * Returns 0 on success, -1 on malformed. */
int zb_aps_parse(const uint8_t *aps, uint8_t len, zb_aps_frm_t *out)
{
    if (!aps || !out || len < 2) return -1;
    memset(out, 0, sizeof(*out));

    uint8_t fc = aps[0];
    out->frame_ctrl = fc;
    out->delivery_mode = (fc >> 4) & 0x07;
    out->security       = (fc >> 2) & 0x07;   /* APS security level */
    out->ack_format     = (fc >> 1) & 1;
    out->ext_hdr        = fc & 1;
    uint8_t frame_type  = fc & 0x03;           /* 0=data, 1=cmd, 2=ack, 3=interpan */

    uint8_t idx = 1;

    if (frame_type == 0x01) {
        /* APS command frame — used for Transport-Key, Update-Device, etc. */
        if (idx >= len) return -1;
        out->cmd_id = aps[idx++];
        /* APS command security header (if secured): sec level + frame ctr */
        if (out->security) {
            /* sec ctrl + frame ctr (4 bytes) — skip for transport-key blob */
            idx++;                       /* sec ctrl */
            idx += 4;                    /* frame counter */
        }
        /* For Transport-Key (cmd_id=0x05) the payload is:
         *   key_type (1) | key (16) | key_seq (1) | dest_eui (8) | src_eui (8) */
        if (out->cmd_id == APS_CMD_TRANSPORT_KEY && (len - idx) >= (1 + 16 + 1 + 8 + 8)) {
            out->key_type = aps[idx++];
            memcpy(out->key_bytes, &aps[idx], 16); idx += 16;
            out->key_seq = aps[idx++];
            memcpy(out->dst_eui, &aps[idx], 8); idx += 8;
            memcpy(out->src_eui, &aps[idx], 8); idx += 8;
        }
        return 0;
    }

    if (frame_type == 0x00) {
        /* APS data frame — carries ZCL. */
        if (out->delivery_mode == 0x02) {
            /* group addressing: dst_group (2) | cluster (2) | src_ep (1) | counter(1) */
            if ((len - idx) < 6) return -1;
            out->dst_group = rd16(&aps[idx]); idx += 2;
            out->cluster  = rd16(&aps[idx]); idx += 2;
            out->src_ep   = aps[idx++];
            out->counter  = aps[idx++];
        } else {
            /* direct addressing: dst_ep(1) | cluster(2) | profile(2) | src_ep(1) | counter(1) */
            if ((len - idx) < 7) return -1;
            out->dst_ep  = aps[idx++];
            out->cluster = rd16(&aps[idx]); idx += 2;
            out->profile = rd16(&aps[idx]); idx += 2;
            out->src_ep  = aps[idx++];
            out->counter = aps[idx++];
        }
        /* ext header optional */
        if (out->ext_hdr) {
            uint8_t ext_len = aps[idx++];
            idx += ext_len;
        }
        /* security header (if APS-secured) */
        if (out->security) {
            idx++;                       /* sec ctrl */
            idx += 4;                    /* frame ctr */
            /* key id varies; skip */
        }
        /* Remaining is ZCL payload */
        return 0;
    }
    return -1;
}

/* Build ZCL OnOff Toggle (cluster 0x0006) command payload.
 * ZCL frame: frame ctrl(1) | seq(1) | cmd(1) */
int zb_zcl_build_onoff(uint8_t *out, uint8_t seq, uint8_t cmd)
{
    out[0] = 0x01;            /* cluster-specific, client-to-server */
    out[1] = seq;
    out[2] = cmd;             /* OFF=0, ON=1, TOGGLE=2 */
    return 3;
}

/* Build ZCL DoorLock Unlock (cluster 0x0101, cmd 0x01 Unlock Door). */
int zb_zcl_build_doorlock_unlock(uint8_t *out, uint8_t seq)
{
    out[0] = 0x01;            /* cluster-specific */
    out[1] = seq;
    out[2] = 0x01;            /* UnlockDoor */
    return 3;
}

/* Extract the encrypted network-key blob from a Transport-Key APS command.
 * The blob is AES-CCM* encrypted under the Trust Center Link Key (TCLK);
 * recovering the plaintext requires the TCLK (derived from install code).
 * Returns 0 on success. */
int zb_aps_extract_transport_key(const zb_aps_frm_t *aps,
                                 uint8_t out_key[16],
                                 uint8_t *out_key_type,
                                 uint8_t *out_key_seq)
{
    if (!aps || aps->cmd_id != APS_CMD_TRANSPORT_KEY) return -1;
    memcpy(out_key, aps->key_bytes, 16);
    if (out_key_type) *out_key_type = aps->key_type;
    if (out_key_seq)  *out_key_seq  = aps->key_seq;
    return 0;
}