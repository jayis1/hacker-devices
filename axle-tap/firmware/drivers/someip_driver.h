/*
 * someip_driver.h — SOME/IP service discovery + fuzzer
 * Author: jayis1
 * License: GPL-2.0
 * Date:   2026-07-22
 */
#ifndef AXLETAP_SOMEIP_DRIVER_H
#define AXLETAP_SOMEIP_DRIVER_H

#include <stdint.h>

/* SOME/IP message types */
#define SOMEIP_MSG_REQUEST      0x00
#define SOMEIP_MSG_REQUEST_ACK  0x40
#define SOMEIP_MSG_NOTIFY       0x80
#define SOMEIP_MSG_NOTIFICATION 0x02
#define SOMEIP_MSG_RESPONSE     0x80
#define SOMEIP_MSG_ERROR        0x81

/* SOME/IP-SD entry types */
#define SOMEIP_SD_FIND         0x00
#define SOMEIP_SD_OFFER       0x01
#define SOMEIP_SD_SUBSCRIBE   0x06
#define SOMEIP_SD_SUBSCRIBE_ACK 0x07

/* SOME/IP-SD UDP port (defined by AUTOSAR) */
#define SOMEIP_SD_PORT 30490

/* Discovered service map entry */
typedef struct {
    uint16_t service_id;
    uint16_t instance_id;
    uint16_t method_id;
    uint8_t  major_version;
    uint32_t minor_version;
    uint16_t port;
    uint8_t  protocol;  /* 0x06 TCP, 0x11 UDP */
    uint32_t ipv4_addr;
    uint32_t last_seen_ms;
    uint8_t  active;
} someip_service_t;

#define SOMEIP_MAX_SERVICES 32

typedef enum {
    SOMEIP_FUZZ_OFF = 0,
    SOMEIP_FUZZ_HEADER_TRUNC,    /* Truncate the header */
    SOMEIP_FUZZ_OVERSIZE,        /* Oversized payload */
    SOMEIP_FUZZ_BAD_LEN,         /* Bad length field */
    SOMEIP_FUZZ_BAD_MSGID,       /* Invalid message ID */
    SOMEIP_FUZZ_BAD_VER,         /* Invalid interface version */
    SOMEIP_FUZZ_FRAG_OVERLAP,    /* TP fragment overlap */
    SOMEIP_FUZZ_RANDOM           /* Random byte mutation */
} someip_fuzz_mode_t;

void someip_init(void);
void someip_on_frame(const uint8_t *frame, uint16_t len, uint8_t dir);

/* Send a SOME/IP-SD Offer message to discover all services on the link. */
int  someip_send_find_all(void);

/* Start fuzzing a discovered service. */
int  someip_fuzz_start(uint16_t service_id, uint16_t method_id,
                       someip_fuzz_mode_t mode, uint32_t rate_hz);
void someip_fuzz_stop(void);

const someip_service_t *someip_get_services(int *count);

#endif /* AXLETAP_SOMEIP_DRIVER_H */