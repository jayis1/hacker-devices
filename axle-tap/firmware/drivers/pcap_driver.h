/*
 * pcap_driver.h — PCAP capture (USB-CDC stream + SD-card circular buffer)
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 */
#ifndef AXLETAP_PCAP_DRIVER_H
#define AXLETAP_PCAP_DRIVER_H

#include <stdint.h>

/* libpcap classic format — single linktype = LINKTYPE_ETHERNET (1) */
#define PCAP_MAGIC          0xA1B2C3D4U
#define PCAP_VERSION_MAJOR  2
#define PCAP_VERSION_MINOR  4
#define PCAP_LINKTYPE_ETH   1

typedef struct {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
} pcap_record_header_t;

void pcap_init(void);
void pcap_start(void);
void pcap_stop(void);
uint8_t pcap_is_running(void);

/* Called by the bridge mirror for every observed frame. */
void pcap_on_frame(const uint8_t *frame, uint16_t len, uint8_t dir, uint64_t ts_ns);

/* Drain SD card ring to USB-CDC (bulk PCAP stream). */
void pcap_dump_usb(void);

/* Stats */
typedef struct {
    uint64_t captured;
    uint64_t dropped;
    uint64_t sd_bytes;
} pcap_stats_t;
void pcap_get_stats(pcap_stats_t *st);

#endif /* AXLETAP_PCAP_DRIVER_H */