/*
 * modbus_rtu.c — Modbus RTU parser/injector
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Modbus RTU frame format (over RS-485):
 *   [address 1B][function 1B][data nB][CRC-16 2B (little-endian)]
 * Valid function codes: 1..6, 15, 16, 23, 24, etc.
 */

#include "modbus_rtu.h"
#include "protocol_detect.h"
#include "clamp_afc.h"
#include "fpga_dsp.h"
#include "board.h"
#include <string.h>

/* ----------------------------------------------------------------------- */
/*  CRC-16 (Modbus)                                                         */
/* ----------------------------------------------------------------------- */

uint16_t modbus_crc16(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFFu;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 1u) {
                crc = (crc >> 1) ^ 0xA001u;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* ----------------------------------------------------------------------- */
/*  Init                                                                    */
/* ----------------------------------------------------------------------- */

void modbus_rtu_init(void) {
    /* Nothing to init — stateless parser */
}

/* ----------------------------------------------------------------------- */
/*  Detect                                                                  */
/* ----------------------------------------------------------------------- */

static int modbus_detect(const uint8_t *frame, uint16_t len) {
    /* Minimum Modbus RTU frame = addr(1) + func(1) + crc(2) = 4 bytes */
    if (len < 4u || len > 256u) return 0;

    uint8_t addr  = frame[0];
    uint8_t func  = frame[1];
    /* Broadcast addr 0 is valid; 1..247 are slave addresses. */
    if (addr > 247u) return 0;

    /* Valid function codes (common) */
    static const uint8_t valid_funcs[] = {1, 2, 3, 4, 5, 6, 7, 8, 11, 15, 16, 22, 23, 24};
    int ok = 0;
    for (uint16_t i = 0; i < (uint16_t)sizeof(valid_funcs); i++) {
        if (func == valid_funcs[i]) { ok = 1; break; }
    }
    if (!ok) return 0;

    /* Check CRC */
    uint16_t crc_calc = modbus_crc16(frame, (uint16_t)(len - 2u));
    uint16_t crc_recv = (uint16_t)frame[len - 2u] | ((uint16_t)frame[len - 1u] << 8);
    if (crc_calc != crc_recv) return 0;

    /* Strong score: valid address, valid function, valid CRC */
    return 100;
}

/* ----------------------------------------------------------------------- */
/*  Parse                                                                   */
/* ----------------------------------------------------------------------- */

static int modbus_parse(const uint8_t *frame, uint16_t len, pr_parsed_t *out) {
    if (len < 4u || !out) return -1;
    memset(out, 0, sizeof(*out));

    out->protocol_id = PR_PROTO_MODBUS_RTU;
    out->dst_addr   = frame[0];
    out->function   = frame[1];
    out->length     = (uint16_t)(len - 4u);  /* minus addr, func, crc */
    if (out->length > sizeof(out->payload)) out->length = sizeof(out->payload);
    memcpy(out->payload, frame + 2, out->length);

    uint16_t crc_calc = modbus_crc16(frame, (uint16_t)(len - 2u));
    uint16_t crc_recv = (uint16_t)frame[len - 2u] | ((uint16_t)frame[len - 1u] << 8);
    out->crc_ok = (crc_calc == crc_recv) ? 1u : 0u;
    return 0;
}

/* ----------------------------------------------------------------------- */
/*  Inject                                                                  */
/* ----------------------------------------------------------------------- */

static int modbus_inject(const uint8_t *frame, uint16_t len) {
    /* The frame is assumed already CRC-correct. We drive the clamp's
     * inject driver for the duration of the frame. */
    if (!clamp_afc_inject_safety_ok()) return -1;
    clamp_afc_inject_enable(1);
    /* The actual bit-streaming is done by the FPGA; here we hand the
     * frame to the FPGA's covert/inject path. */
    /* (In a real build, fpga_dsp_inject_frame() would stream the bits.) */
    for (uint16_t i = 0; i < len; i++) {
        fpga_dsp_covert_tx_byte(frame[i]);  /* reuse the low-rate path */
    }
    board_delay_us(500);  /* let the last byte drain */
    clamp_afc_inject_enable(0);
    return 0;
}

void modbus_inject_test_frame(void) {
    /* Read Coils: addr=1, func=1, start=0x0000, qty=8 */
    uint8_t frame[8];
    frame[0] = 0x01;  /* slave address 1 */
    frame[1] = 0x01;  /* function 1 (read coils) */
    frame[2] = 0x00; frame[3] = 0x00;  /* start address 0 */
    frame[4] = 0x00; frame[5] = 0x08;  /* quantity 8 */
    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = (uint8_t)(crc & 0xFFu);
    frame[7] = (uint8_t)((crc >> 8) & 0xFFu);
    modbus_inject(frame, 8);
}

/* ----------------------------------------------------------------------- */
/*  Descriptor                                                              */
/* ----------------------------------------------------------------------- */

const pr_protocol_t pr_proto_modbus_rtu = {
    .id     = PR_PROTO_MODBUS_RTU,
    .name   = "Modbus RTU",
    .detect = modbus_detect,
    .parse  = modbus_parse,
    .inject = modbus_inject,
};