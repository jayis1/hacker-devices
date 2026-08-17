/*
 * hart_fsk.c — HART FSK (Bell 202) parser
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * HART uses Bell 202 FSK: 1200 Hz (bit 1) and 2200 Hz (bit 0) superimposed
 * on the 4-20 mA loop. The FPGA recovers the bitstream; this parser deals
 * with the HART UART frame (8 data bits, 1 start, 1 stop, odd parity,
 * 1200 baud).
 *
 * HART frame format (per HCF_SPEC-127):
 *   [PREAMBLE: 0xFF x 5..20][START: 0x82|0x06][ADDR: 1-5B][CMD: 1B]
 *   [BYTECNT: 1B][DATA: nB][CHKSUM: 1B]
 *
 * The preamble is a run of 0xFF bytes (at least 5) used by the receiver
 * to lock onto the bit stream. The start byte indicates short (0x82 long
 * frame) or long frame format (0x06). Checksum is XOR of all bytes
 * from start through the last data byte.
 */

#include "hart_fsk.h"
#include "protocol_detect.h"
#include <string.h>

void hart_fsk_init(void) {
}

/* ----------------------------------------------------------------------- */
/*  Detect                                                                  */
/* ----------------------------------------------------------------------- */

static int hart_detect(const uint8_t *frame, uint16_t len) {
    if (len < 5u) return 0;

    /* Look for a run of 0xFF preamble bytes followed by a start byte. */
    int pre = 0;
    while ((uint16_t)pre < len && frame[pre] == 0xFFu) pre++;
    if (pre < 5u) return 0;  /* need at least 5 preamble bytes */

    uint16_t idx = (uint16_t)pre;
    if (idx >= len) return 0;
    uint8_t start = frame[idx];
    if (start != 0x82u && start != 0x06u) return 0;

    /* Verify the checksum (XOR of all bytes from start onward) */
    uint8_t chk = 0u;
    for (uint16_t i = idx; i < len; i++) {
        chk ^= frame[i];
    }
    /* If the last byte is the checksum, XOR should yield 0. */
    if (chk != 0u) return 0;

    return 85;
}

/* ----------------------------------------------------------------------- */
/*  Parse                                                                   */
/* ----------------------------------------------------------------------- */

static int hart_parse(const uint8_t *frame, uint16_t len, pr_parsed_t *out) {
    if (!out || len < 5u) return -1;
    memset(out, 0, sizeof(*out));
    out->protocol_id = PR_PROTO_HART_FSK;

    /* Skip preamble */
    uint16_t idx = 0u;
    while (idx < len && frame[idx] == 0xFFu) idx++;
    if (idx >= len) return -1;

    uint8_t start = frame[idx++];
    out->function = 0u;
    out->length = 0u;

    if (start == 0x82u) {
        /* Short frame: [0x82][ADDR 1B][CMD 1B][BYTECNT 1B][DATA nB][CHKSUM 1B] */
        if ((uint16_t)(idx + 4u) > len) return -1;
        out->dst_addr = frame[idx++];  /* polling address */
        out->function = frame[idx++];
        uint8_t bytecnt = frame[idx++];
        out->length = bytecnt;
        uint16_t copy = (bytecnt > sizeof(out->payload)) ? sizeof(out->payload) : bytecnt;
        if ((uint16_t)(idx + copy) > len) copy = (uint16_t)(len - idx);
        memcpy(out->payload, frame + idx, copy);
        out->crc_ok = 1u;
    } else if (start == 0x06u) {
        /* Long frame: [0x06][ADDR 5B][CMD 1B][BYTECNT 1B][DATA nB][CHKSUM 1B] */
        if ((uint16_t)(idx + 7u) > len) return -1;
        /* 5-byte address: first byte is the polling address */
        out->dst_addr = frame[idx];
        idx += 5u;
        out->function = frame[idx++];
        uint8_t bytecnt = frame[idx++];
        out->length = bytecnt;
        uint16_t copy = (bytecnt > sizeof(out->payload)) ? sizeof(out->payload) : bytecnt;
        if ((uint16_t)(idx + copy) > len) copy = (uint16_t)(len - idx);
        memcpy(out->payload, frame + idx, copy);
        out->crc_ok = 1u;
    } else {
        return -1;
    }
    return 0;
}

/* ----------------------------------------------------------------------- */
/*  Inject                                                                  */
/* ----------------------------------------------------------------------- */

static int hart_inject(const uint8_t *frame, uint16_t len) {
    (void)frame; (void)len;
    return 0;  /* not implemented in this reference build */
}

/* ----------------------------------------------------------------------- */
/*  Descriptor                                                              */
/* ----------------------------------------------------------------------- */

const pr_protocol_t pr_proto_hart_fsk = {
    .id     = PR_PROTO_HART_FSK,
    .name   = "HART FSK",
    .detect = hart_detect,
    .parse  = hart_parse,
    .inject = hart_inject,
};