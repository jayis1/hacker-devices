/*
 * plc_capture.h — PLC frame capture ring + pcap writer interface
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PLC_CAPTURE_H
#define PLC_CAPTURE_H

#include <stdint.h>
#include "qca7420.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAP_RING_LEN  64

typedef struct {
    uint32_t ts_ms;
    uint8_t  tei;
    uint8_t  snr_db;
    uint8_t  opcode;
    uint16_t len;
    uint8_t  src[6];
    uint8_t  dst[6];
    uint8_t  data[PLC_MAX_FRAME];
} plc_cap_frame_t;

void    plc_capture_init(void);
void    plc_capture_push(const qca7420_frame_t *f);
void    plc_capture_set_filter(const uint8_t mac[6], uint16_t ethtype);
void    plc_capture_flush_to_sd(void);
void    plc_capture_crypto_erase(void);
uint32_t plc_capture_count(void);
uint32_t plc_capture_dropped(void);

#ifdef __cplusplus
}
#endif

#endif /* PLC_CAPTURE_H */