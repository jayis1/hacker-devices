/*
 * profibus_dp.c — Profibus DP parser
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Profibus DP UART frame format (per IEC 61158):
 *   SD1 = 0x10 (fixed-length, no data)
 *   SD2 = 0x68 (variable length, with data)  -- most common
 *   SD3 = 0xA2 (fixed length, with data)
 *   SD4 = 0xDC (token frame, master-master)
 *
 * SD2 frame:
 *   [SD2][LE][LEr][SD2][DA][SA][FC][DSAP][SSAP][DU...][FCS][ED=0x16]
 *   DA = destination address (low 7 bits) | 0x80 if extension
 *   SA = source address      (low 7 bits) | 0x80 if extension
 *   FC = function code
 *   ED = end delimiter 0x16
 */

#include "profibus_dp.h"
#include "protocol_detect.h"
#include "board.h"
#include <string.h>

void profibus_dp_init(void) {
}

/* ----------------------------------------------------------------------- */
/*  Detect                                                                  */
/* ----------------------------------------------------------------------- */

static int profibus_detect(const uint8_t *frame, uint16_t len) {
    if (len < 4u) return 0;
    uint8_t sd = frame[0];

    /* SD2: 0x68, len, len, 0x68, ..., 0x16 */
    if (sd == 0x68u && len >= 6u) {
        uint8_t le = frame[1];
        uint8_t ler = frame[2];
        if (le != ler) return 0;
        if (frame[3] != 0x68u) return 0;
        /* End delimiter must be 0x16 at position len-1 */
        if (frame[len - 1u] != 0x16u) return 0;
        /* Total length check: 4 header + le + 3 (FCS+ED) ... simplified */
        if (len < (uint16_t)(4u + le + 3u)) return 0;
        return 90;
    }

    /* SD1: 0x10, DA, SA, FC, FCS, 0x16 (6 bytes) */
    if (sd == 0x10u && len == 6u && frame[5] == 0x16u) {
        return 80;
    }

    /* SD4: token frame 0xDC, DA, SA, 0x00 (4 bytes) */
    if (sd == 0xDCu && len == 4u) {
        return 70;
    }

    return 0;
}

/* ----------------------------------------------------------------------- */
/*  Parse                                                                   */
/* ----------------------------------------------------------------------- */

static int profibus_parse(const uint8_t *frame, uint16_t len, pr_parsed_t *out) {
    if (!out || len < 4u) return -1;
    memset(out, 0, sizeof(*out));
    out->protocol_id = PR_PROTO_PROFIBUS_DP;

    uint8_t sd = frame[0];
    if (sd == 0x68u) {
        /* SD2 variable-length */
        uint8_t le = frame[1];
        /* DA at index 4, SA at index 5, FC at index 6 */
        if (len < 7u) return -1;
        out->dst_addr = frame[4] & 0x7Fu;
        out->src_addr = frame[5] & 0x7Fu;
        out->function = frame[6];
        /* DSAP/SSAP may follow; data unit starts at 7 or 9 */
        uint16_t du_off = 7u;
        if ((frame[4] & 0x80u) && (frame[5] & 0x80u)) du_off = 9u; /* extended addr */
        uint16_t du_len = le;
        if (du_off + du_len > len) du_len = (uint16_t)(len - du_off - 3u);
        if (du_len > sizeof(out->payload)) du_len = sizeof(out->payload);
        out->length = du_len;
        if (du_len > 0u) {
            memcpy(out->payload, frame + du_off, du_len);
        }
        out->crc_ok = 1u;  /* FCS byte is at len-2; we accept for now */
    } else if (sd == 0x10u) {
        /* SD1 fixed */
        out->dst_addr = frame[1] & 0x7Fu;
        out->src_addr = frame[2] & 0x7Fu;
        out->function = frame[3];
        out->length = 0u;
        out->crc_ok = 1u;
    } else if (sd == 0xDCu) {
        /* Token */
        out->dst_addr = frame[1] & 0x7Fu;
        out->src_addr = frame[2] & 0x7Fu;
        out->function = 0u;
        out->length = 0u;
        out->crc_ok = 1u;
    } else {
        return -1;
    }
    return 0;
}

/* ----------------------------------------------------------------------- */
/*  Inject                                                                  */
/* ----------------------------------------------------------------------- */

static int profibus_inject(const uint8_t *frame, uint16_t len) {
    /* Same path as Modbus — drive the clamp inject driver via the FPGA. */
    (void)frame; (void)len;
    return 0;  /* not implemented in this reference build */
}

/* ----------------------------------------------------------------------- */
/*  Descriptor                                                              */
/* ----------------------------------------------------------------------- */

const pr_protocol_t pr_proto_profibus_dp = {
    .id     = PR_PROTO_PROFIBUS_DP,
    .name   = "Profibus DP",
    .detect = profibus_detect,
    .parse  = profibus_parse,
    .inject = profibus_inject,
};