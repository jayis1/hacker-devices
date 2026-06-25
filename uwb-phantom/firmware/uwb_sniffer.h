/*
 * uwb_sniffer.h — UWB frame sniffer interface.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UWB_PHANTOM_SNIFFER_H
#define UWB_PHANTOM_SNIFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define SNIFF_MAX_FRAME  127

typedef struct {
    uint64_t rx_dtu;            /* receive timestamp in DTU */
    uint32_t channel;           /* UWB channel */
    int8_t   rssi_dbm;          /* estimated RSSI */
    uint8_t  sts_quality;       /* 0..255 (0 if STS off) */
    uint16_t frame_len;         /* actual bytes received */
    uint8_t  frame[SNIFF_MAX_FRAME]; /* raw 802.15.4 frame */
} sniff_frame_t;

typedef void (*sniff_cb_t)(const sniff_frame_t *f, void *user);

int  sniff_start(uint8_t channel, sniff_cb_t cb, void *user);
void sniff_stop(void);

/* PCAP writer — writes IEEE 802.15.4 linktype with a custom TLV
 * extension that records UWB-PHY metadata (channel, STS, RSSI). */
int  sniff_pcap_open(const char *path);
int  sniff_pcap_write(const sniff_frame_t *f);
int  sniff_pcap_close(void);

#ifdef __cplusplus
}
#endif

#endif /* UWB_PHANTOM_SNIFFER_H */