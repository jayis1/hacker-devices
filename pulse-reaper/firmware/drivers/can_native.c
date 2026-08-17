/*
 * can_native.c — CAN / CAN-FD bit-slicer and parser
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Classic CAN frame (11-bit ID):
 *   SOF(1) | ID(11) | RTR(1) | IDE(1) | r0(1) | DLC(4) | DATA(0..64) |
 *   CRC(15) | CRCdel(1) | ACK(1) | ACKdel(1) | EOF(7) | IFS(3)
 *
 * The FPGA's bit-slicer recovers the raw bitstream and hands the MCU a
 * "cooked" frame: [ID(2B)][DLC(1B)][DATA(0..64B)][flags(1B)].
 * This file's detect() recognises the cooked format and parse() fills
 * the pr_parsed_t. This avoids re-implementing bit-stuffing in C.
 */

#include "can_native.h"
#include "protocol_detect.h"
#include <string.h>

void can_native_init(void) {
}

/* ----------------------------------------------------------------------- */
/*  Detect                                                                  */
/* ----------------------------------------------------------------------- */

static int can_detect(const uint8_t *frame, uint16_t len) {
    /* Cooked CAN frame: ID(2) + DLC(1) + DATA(DLC) + flags(1) = 4 + DLC.
     * The "flags" byte has bit 0 = RTR, bit 1 = IDE (extended), bit 2 = FD,
     * bit 3 = BRS, bit 4 = ESI. */
    if (len < 4u) return 0;
    uint8_t dlc = frame[2];
    if (dlc > 64u) return 0;
    if (len != (uint16_t)(4u + dlc)) return 0;
    uint8_t flags = frame[3u + dlc];
    /* Only the low 5 bits of flags are defined */
    if (flags & 0xE0u) return 0;
    /* ID upper bits should be 0 for 11-bit; allow 29-bit (IDE set) */
    if ((flags & 0x02u) == 0u) {
        /* 11-bit: ID must fit in 11 bits (top 5 bits of ID16 zero) */
        uint16_t id = (uint16_t)((uint16_t)frame[0] | ((uint16_t)frame[1] << 8));
        if (id > 0x07FFu) return 0;
    }
    return 75;
}

/* ----------------------------------------------------------------------- */
/*  Parse                                                                   */
/* ----------------------------------------------------------------------- */

static int can_parse(const uint8_t *frame, uint16_t len, pr_parsed_t *out) {
    if (!out || len < 4u) return -1;
    memset(out, 0, sizeof(*out));
    out->protocol_id = PR_PROTO_CAN;

    uint16_t id = (uint16_t)((uint16_t)frame[0] | ((uint16_t)frame[1] << 8));
    uint8_t  dlc = frame[2];
    if (dlc > 64u) dlc = 64u;
    uint8_t  flags = frame[3u + dlc];

    /* CAN has no source/destination address in the traditional sense;
     * the ID is an arbitration priority. We put it in dst_addr. */
    out->dst_addr = id;
    out->function = dlc;
    out->length = dlc;
    if (dlc > sizeof(out->payload)) dlc = (uint8_t)sizeof(out->payload);
    memcpy(out->payload, frame + 3, dlc);
    out->crc_ok = 1u;  /* CRC already checked by the FPGA bit-slicer */
    (void)flags;
    return 0;
}

/* ----------------------------------------------------------------------- */
/*  Inject                                                                  */
/* ----------------------------------------------------------------------- */

static int can_inject(const uint8_t *frame, uint16_t len) {
    (void)frame; (void)len;
    return 0;  /* not implemented in this reference build */
}

/* ----------------------------------------------------------------------- */
/*  Descriptor                                                              */
/* ----------------------------------------------------------------------- */

const pr_protocol_t pr_proto_can = {
    .id     = PR_PROTO_CAN,
    .name   = "CAN/CAN-FD",
    .detect = can_detect,
    .parse  = can_parse,
    .inject = can_inject,
};