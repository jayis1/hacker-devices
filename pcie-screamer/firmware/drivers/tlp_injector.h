/**
 * drivers/tlp_injector.h — TLP Injector Driver Interface
 *
 * PCIe Screamer — NVMe Interposer
 * Author: jayis1
 * Date: 2026-06-16
 */

#ifndef TLP_INJECTOR_H
#define TLP_INJECTOR_H

#include <stdint.h>
#include "pcie.h"

/* Injection TLP types (simplified enum) */
#define INJECT_MRD      0x00
#define INJECT_MWR      0x01
#define INJECT_CFGRD0   0x05
#define INJECT_CFGWR0   0x06
#define INJECT_CFGRD1   0x07
#define INJECT_CFGWR1   0x08
#define INJECT_MSG      0x09
#define INJECT_MSGD     0x0A

/* Injection result codes */
#define INJECT_OK        0
#define INJECT_TIMEOUT   1
#define INJECT_UR        2
#define INJECT_CA        3
#define INJECT_CRS       4

int tlp_injector_init(void);
int tlp_injector_inject_mrd(uint16_t requester_id, uint8_t tag,
                            uint64_t address, uint16_t length_dw,
                            uint8_t *completion_data, uint16_t *cpl_len);
int tlp_injector_inject_mwr(uint16_t requester_id, uint64_t address,
                            const uint8_t *data, uint16_t length_dw);
int tlp_injector_inject_cfgrd(uint8_t bus, uint8_t device, uint8_t function,
                              uint16_t reg_offset, uint32_t *value);
int tlp_injector_inject_cfgwr(uint8_t bus, uint8_t device, uint8_t function,
                              uint16_t reg_offset, uint32_t value);
int tlp_injector_is_busy(void);
uint32_t tlp_injector_get_count(void);
uint32_t tlp_injector_get_error_count(void);

#endif
