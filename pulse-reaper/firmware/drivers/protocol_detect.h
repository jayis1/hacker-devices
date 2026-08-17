/*
 * protocol_detect.h — auto-detect fieldbus protocol from a recovered frame
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef PULSEREAPER_PROTOCOL_DETECT_H
#define PULSEREAPER_PROTOCOL_DETECT_H

#include <stdint.h>

/* Parsed frame — filled in by the protocol parser */
typedef struct {
    uint8_t  protocol_id;   /* PR_PROTO_* */
    uint32_t timestamp_ms;
    uint16_t src_addr;
    uint16_t dst_addr;
    uint8_t  function;      /* protocol-specific function/command code */
    uint16_t length;
    uint8_t  payload[256];
    uint16_t crc_ok;
} pr_parsed_t;

/* Protocol descriptor — one per supported protocol */
typedef struct {
    uint8_t  id;
    const char *name;
    int  (*detect)(const uint8_t *frame, uint16_t len);
    int  (*parse)(const uint8_t *frame, uint16_t len, pr_parsed_t *out);
    int  (*inject)(const uint8_t *frame, uint16_t len);
} pr_protocol_t;

#define PR_PROTO_NONE       0u
#define PR_PROTO_MODBUS_RTU 1u
#define PR_PROTO_PROFIBUS_DP 2u
#define PR_PROTO_HART_FSK   3u
#define PR_PROTO_CAN        4u
#define PR_PROTO_POTS_DTMF  5u

void protocol_detect_init(void);

/* Detect the protocol of a frame. Returns the protocol descriptor or NULL. */
const pr_protocol_t *protocol_detect(const uint8_t *frame, uint16_t len);

#endif