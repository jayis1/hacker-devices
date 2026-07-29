/*
 * drivers/sd_pcap.h — PCAP-NG capture writer header
 *
 * Author:  jayis1
 * License: GPL-2.0
 */

#ifndef NVME_PHANTOM_SD_PCAP_H
#define NVME_PHANTOM_SD_PCAP_H

#include <stdint.h>
#include "nvme_parser.h"

int pcap_open(uint32_t session_id);
int pcap_write_cmd(const nvme_cmd_t *cmd, const nvme_cpl_t *cpl,
                   uint64_t timestamp_ns, const uint8_t *data, uint32_t data_len);
int pcap_close(uint32_t *out_records, uint32_t *out_duration_s);
uint32_t pcap_sd_free_kb(void);

#endif /* NVME_PHANTOM_SD_PCAP_H */