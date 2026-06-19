/*
 * plc_capture.c — IEEE 1901 frame ring buffer + pcap (DLT_USER0) writer
 *
 * Captures promiscuous PLC frames into an in-RAM ring, then flushes them to
 * SD card as a pcap file with a custom link-layer header (DLT_USER0 = 147)
 * carrying TEI, SNR, tone-map-id and the raw 1901 MAC fields. The host app /
 * Wireshark dissector decodes this format.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <stdint.h>
#include <string.h>
#include "plc_capture.h"
#include "qca7420.h"
#include "sd_pcap.h"

/* In-RAM ring (holds frames between SD flushes) */
static plc_cap_frame_t cap_ring[CAP_RING_LEN];
static volatile uint32_t cap_head = 0, cap_tail = 0;
static volatile uint32_t cap_dropped = 0;
static uint32_t cap_total = 0;
static uint8_t cap_filter_mac[6];
static int      cap_filter_active = 0;
static uint16_t cap_filter_ethtype = 0;

/* pcap DLT_USER0 custom header:
 *  [0..3]   magic 0x52415043 ("CAPR")
 *  [4..7]   ts_lo (ms)
 *  [8..11]  ts_hi
 *  [12]     tei
 *  [13]     snr_db
 *  [14..15] tone_map_id
 *  [16..19] orig_len
 *  [20..23] cap_len
 *  [24..29] src mac
 *  [30..35] dst mac
 *  [36..37] opcode
 *  then payload bytes (cap_len)
 */
#define CAP_HDR_MAGIC 0x52415043UL

void plc_capture_init(void) {
    cap_head = cap_tail = 0;
    cap_dropped = 0;
    cap_total = 0;
    cap_filter_active = 0;
}

void plc_capture_set_filter(const uint8_t mac[6], uint16_t ethtype) {
    if (mac) {
        memcpy(cap_filter_mac, mac, 6);
        cap_filter_active = 1;
    } else {
        cap_filter_active = 0;
    }
    cap_filter_ethtype = ethtype;
}

static int frame_passes_filter(const qca7420_frame_t *f) {
    if (!cap_filter_active) return 1;
    if (cap_filter_ethtype) {
        uint16_t et = (f->data[14] << 8) | f->data[15];
        if (et != cap_filter_ethtype) return 0;
    }
    if (memcmp(f->src, cap_filter_mac, 6) != 0 &&
        memcmp(f->dst, cap_filter_mac, 6) != 0) return 0;
    return 1;
}

void plc_capture_push(const qca7420_frame_t *f) {
    if (!frame_passes_filter(f)) return;
    uint32_t next = (cap_head + 1) % CAP_RING_LEN;
    if (next == cap_tail) {
        cap_dropped++;
        return;
    }
    plc_cap_frame_t *c = &cap_ring[cap_head];
    c->ts_ms   = f->ts_ms;
    c->tei     = f->tei;
    c->snr_db  = f->snr_db;
    c->opcode  = f->opcode;
    c->len     = f->len;
    memcpy(c->src, f->src, 6);
    memcpy(c->dst, f->dst, 6);
    if (f->len <= PLC_MAX_FRAME) {
        memcpy(c->data, f->data, f->len);
    }
    cap_head = next;
    cap_total++;
}

/* Serialize one frame to a pcap record buffer; returns bytes written */
static uint32_t serialize_frame(uint8_t *out, const plc_cap_frame_t *c) {
    uint32_t *magic = (uint32_t *)out;
    magic[0] = CAP_HDR_MAGIC;
    out[4] = c->ts_ms & 0xFF;
    out[5] = (c->ts_ms >> 8) & 0xFF;
    out[6] = (c->ts_ms >> 16) & 0xFF;
    out[7] = (c->ts_ms >> 24) & 0xFF;
    out[8] = 0; out[9] = 0; out[10] = 0; out[11] = 0; /* ts_hi */
    out[12] = c->tei;
    out[13] = c->snr_db;
    out[14] = 0; out[15] = 0;   /* tone_map_id placeholder */
    uint32_t len = c->len;
    out[16] = len & 0xFF; out[17] = (len >> 8) & 0xFF;
    out[18] = (len >> 16) & 0xFF; out[19] = (len >> 24) & 0xFF;
    out[20] = len & 0xFF; out[21] = (len >> 8) & 0xFF;
    out[22] = (len >> 16) & 0xFF; out[23] = (len >> 24) & 0xFF;
    memcpy(&out[24], c->src, 6);
    memcpy(&out[30], c->dst, 6);
    out[36] = c->opcode; out[37] = 0;
    memcpy(&out[38], c->data, c->len);
    return 38 + c->len;
}

/* Flush ring to SD pcap (called periodically from main loop) */
void sd_pcap_flush_tick(uint32_t now_ms);
void plc_capture_flush_to_sd(void) {
    static uint8_t outbuf[SD_PCAP_BUF_BYTES];
    uint32_t pos = 0;
    while (cap_tail != cap_head) {
        plc_cap_frame_t *c = &cap_ring[cap_tail];
        uint32_t need = 38 + c->len;
        if (pos + need > sizeof(outbuf)) break;
        uint32_t n = serialize_frame(&outbuf[pos], c);
        pos += n;
        cap_tail = (cap_tail + 1) % CAP_RING_LEN;
    }
    if (pos > 0) {
        sd_pcap_write(outbuf, pos);
    }
}

uint32_t plc_capture_count(void)   { return cap_total; }
uint32_t plc_capture_dropped(void) { return cap_dropped; }

/* Crypto-erase: overwrite SD FAT with random and re-init */
void plc_capture_crypto_erase(void) {
    sd_pcap_crypto_erase();
    cap_head = cap_tail = 0;
    cap_total = 0;
    cap_dropped = 0;
}