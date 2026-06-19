/*
 * sd_pcap.h — SD pcap writer interface
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef SD_PCAP_H
#define SD_PCAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PCAP_MAX_BYTES  (64 * 1024 * 1024) /* 64 MB per file before rotate */

int      sd_pcap_init(void);
int      sd_pcap_open_next(void);
int      sd_pcap_write(const uint8_t *data, uint32_t len);
void     sd_pcap_flush_tick(uint32_t now_ms);
void     sd_pcap_crypto_erase(void);
int      sd_pcap_present(void);
uint32_t sd_pcap_written(void);

#ifdef __cplusplus
}
#endif

#endif /* SD_PCAP_H */