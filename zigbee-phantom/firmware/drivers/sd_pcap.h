/*
 * drivers/sd_pcap.h — MicroSD pcap writer for 802.15.4 captures
 * Author: jayis1
 * License: GPL-2.0
 */
#ifndef ZIGBEE_PHANTOM_SD_PCAP_H
#define ZIGBEE_PHANTOM_SD_PCAP_H

#include <stdint.h>
#include <stdbool.h>

int  sd_pcap_init(void);                          /* mount + open capture file */
int  sd_pcap_write_frame(uint8_t channel, int8_t rssi, uint8_t lqi,
                         uint32_t ts_us, const uint8_t *frame, uint8_t len);
void sd_pcap_flush(void);
void sd_pcap_close(void);
bool sd_present(void);

#endif