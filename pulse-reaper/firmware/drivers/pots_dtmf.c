/*
 * pots_dtmf.c — POTS DTMF / caller-ID (Bell 202 FSK) parser
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The FPGA recovers DTMF digit events (Goertzel detection on the two
 * tone frequencies) and caller-ID FSK frames (Bell 202, 1200 baud).
 *
 * Cooked DTMF frame from FPGA: [0xD0][digit_ascii 1B][duration_ms 2B]
 * Cooked caller-ID frame:       [0xD1][len 1B][message_type 1B][data len B][chksum 1B]
 */

#include "pots_dtmf.h"
#include "protocol_detect.h"
#include <string.h>

void pots_dtmf_init(void) {
}

/* DTMF frequency pairs (row, col) */
static const char s_dtmf_map[4][4] = {
    /* 1209 1336 1477 1633 */
    { '1','2','3','A' },  /* 697 */
    { '4','5','6','B' },  /* 770 */
    { '7','8','9','C' },  /* 852 */
    { '*','0','#','D' },  /* 941 */
};

static int pots_detect(const uint8_t *frame, uint16_t len) {
    if (len < 3u) return 0;
    uint8_t tag = frame[0];
    if (tag == 0xD0u && len == 4u) {
        /* DTMF digit event: validate the digit is in the map */
        uint8_t d = frame[1];
        int found = 0;
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                if ((uint8_t)s_dtmf_map[r][c] == d) found = 1;
        return found ? 60 : 0;
    }
    if (tag == 0xD1u && len >= 4u) {
        /* caller-ID: check checksum (XOR of bytes 2..len-1 should be 0) */
        uint8_t chk = 0u;
        for (uint16_t i = 1u; i < len; i++) chk ^= frame[i];
        if (chk != 0u) return 0;
        return 65;
    }
    return 0;
}

static int pots_parse(const uint8_t *frame, uint16_t len, pr_parsed_t *out) {
    if (!out || len < 3u) return -1;
    memset(out, 0, sizeof(*out));
    out->protocol_id = PR_PROTO_POTS_DTMF;

    if (frame[0] == 0xD0u && len == 4u) {
        /* DTMF digit */
        out->function = 0xD0u;
        out->length = 1u;
        out->payload[0] = frame[1];   /* digit ASCII */
        out->crc_ok = 1u;
        /* duration in ms is frame[2] | frame[3]<<8 */
    } else if (frame[0] == 0xD1u && len >= 4u) {
        /* caller-ID */
        out->function = 0xD1u;
        uint8_t mlen = frame[1];
        out->length = mlen;
        if (mlen > sizeof(out->payload)) mlen = (uint8_t)sizeof(out->payload);
        if ((uint16_t)(2u + mlen) > len) mlen = (uint8_t)(len - 2u);
        memcpy(out->payload, frame + 2, mlen);
        out->crc_ok = 1u;
    } else {
        return -1;
    }
    return 0;
}

static int pots_inject(const uint8_t *frame, uint16_t len) {
    (void)frame; (void)len;
    return 0;  /* not implemented */
}

const pr_protocol_t pr_proto_pots_dtmf = {
    .id     = PR_PROTO_POTS_DTMF,
    .name   = "POTS DTMF/CID",
    .detect = pots_detect,
    .parse  = pots_parse,
    .inject = pots_inject,
};