/**
 * drivers/tlp_sniffer.h — TLP Sniffer Driver Interface
 *
 * PCIe Screamer — NVMe Interposer
 * Author: jayis1
 * Date: 2026-06-16
 */

#ifndef TLP_SNIFFER_H
#define TLP_SNIFFER_H

#include <stdint.h>
#include "pcie.h"

/* Sniffer control flags */
#define SNIFFER_ENABLE_HOST2DEV    (1 << 0)
#define SNIFFER_ENABLE_DEV2HOST    (1 << 1)
#define SNIFFER_ENABLE_CFG         (1 << 2)
#define SNIFFER_ENABLE_MSG         (1 << 3)
#define SNIFFER_ENABLE_CPL         (1 << 4)
#define SNIFFER_ENABLE_MEM         (1 << 5)
#define SNIFFER_ENABLE_IO          (1 << 6)
#define SNIFFER_TIMESTAMP_EN       (1 << 7)
#define SNIFFER_FILTER_EN          (1 << 8)

int tlp_sniffer_init(void);
void tlp_sniffer_start(uint8_t flags, uint32_t filter_mask,
                       uint64_t addr_filter, uint16_t reqid_filter);
void tlp_sniffer_stop(void);
int tlp_sniffer_capture_one(uint8_t *buf, uint16_t max_len);
uint32_t tlp_sniffer_get_count(void);
uint32_t tlp_sniffer_get_dropped(void);
int tlp_sniffer_is_active(void);

#endif
