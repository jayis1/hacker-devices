/* Wiegand and OSDP protocol engines for Credential Canary
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "protocol.h"
#include "capture.h"
#include <string.h>

static wiegand_decoder_t wg[2];
static osdp_decoder_t osdp[2];
static bool forwarding_enabled;

static unsigned wg_index(interface_t iface) { return iface == IFACE_WIEGAND_B ? 1u : 0u; }
static unsigned osdp_index(interface_t iface) { return iface == IFACE_OSDP_PANEL ? 1u : 0u; }

void protocol_init(void) {
    memset(wg, 0, sizeof wg);
    memset(osdp, 0, sizeof osdp);
    forwarding_enabled = false;
}

void protocol_set_forwarding(bool enabled) { forwarding_enabled = enabled; }

void wiegand_edge(interface_t iface, bool data_one, uint32_t timestamp_us) {
    wiegand_decoder_t *d = &wg[wg_index(iface)];
    if (d->bit_count && (uint32_t)(timestamp_us - d->last_edge_us) < WIEGAND_GLITCH_US) {
        d->glitches++;
        uint8_t detail[3] = { d->bit_count, (uint8_t)d->glitches, data_one ? 1u : 0u };
        capture_push(EVT_FAULT, iface, detail, sizeof detail, 1u, timestamp_us);
        return;
    }
    if (d->bit_count == 0u) d->first_edge_us = timestamp_us;
    d->last_edge_us = timestamp_us;
    if (d->bit_count < 64u) {
        d->bits = (d->bits << 1u) | (data_one ? 1u : 0u);
        d->bit_count++;
    } else {
        d->overflow = true;
    }
    uint8_t edge[2] = { d->bit_count, data_one ? 1u : 0u };
    capture_push(EVT_WIEGAND_BIT, iface, edge, sizeof edge, 0u, timestamp_us);
    /* The isolated output stage mirrors each pulse only in explicitly selected
       bridge mode. Pulse shaping is in a timer ISR on hardware. */
    (void)forwarding_enabled;
}

static unsigned popcount64(uint64_t value) {
    unsigned count = 0u;
    while (value) { count += (unsigned)(value & 1u); value >>= 1u; }
    return count;
}

bool wiegand_parity_valid(uint64_t raw, uint8_t bits) {
    if (bits == 26u) {
        uint32_t upper = (uint32_t)((raw >> 13u) & 0x0FFFu);
        uint32_t lower = (uint32_t)((raw >> 1u) & 0x0FFFu);
        bool lead_even = ((raw >> 25u) & 1u) != 0u;
        bool tail_odd = (raw & 1u) != 0u;
        return ((popcount64(upper) + lead_even) & 1u) == 0u &&
               ((popcount64(lower) + tail_odd) & 1u) == 1u;
    }
    if (bits == 34u) {
        uint64_t upper = (raw >> 17u) & 0xFFFFu;
        uint64_t lower = (raw >> 1u) & 0xFFFFu;
        bool lead_even = ((raw >> 33u) & 1u) != 0u;
        bool tail_odd = (raw & 1u) != 0u;
        return ((popcount64(upper) + lead_even) & 1u) == 0u &&
               ((popcount64(lower) + tail_odd) & 1u) == 1u;
    }
    return true; /* unknown formats are reported, not falsely condemned */
}

uint64_t wiegand_payload(uint64_t raw, uint8_t bit_count) {
    if (bit_count == 26u || bit_count == 34u) return (raw >> 1u) & ((1ULL << (bit_count - 2u)) - 1u);
    return raw;
}

static void finish_wiegand(interface_t iface, wiegand_decoder_t *d, uint32_t now_us) {
    uint8_t packet[12];
    bool parity = wiegand_parity_valid(d->bits, d->bit_count);
    packet[0] = d->bit_count;
    packet[1] = parity ? 1u : 0u;
    packet[2] = d->overflow ? 1u : 0u;
    packet[3] = (uint8_t)MIN_U32(d->glitches, 255u);
    for (unsigned i = 0; i < 8u; ++i) packet[4u+i] = (uint8_t)(d->bits >> (56u - i * 8u));
    capture_push(EVT_WIEGAND_FRAME, iface, packet, sizeof packet, parity ? 0u : 1u, now_us);
    policy_observe_wiegand(d->bits, d->bit_count, parity, now_us);
    memset(d, 0, sizeof *d);
}

void wiegand_poll(uint32_t now_us) {
    for (unsigned i = 0; i < ARRAY_SIZE(wg); ++i) {
        if (wg[i].bit_count && (uint32_t)(now_us - wg[i].last_edge_us) >= WIEGAND_GAP_US)
            finish_wiegand(i ? IFACE_WIEGAND_B : IFACE_WIEGAND_A, &wg[i], now_us);
    }
}

uint16_t osdp_crc16(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0x1D0Fu;
    for (uint16_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8u;
        for (unsigned b = 0; b < 8u; ++b) crc = (crc & 0x8000u) ? (uint16_t)((crc << 1u) ^ 0x1021u) : (uint16_t)(crc << 1u);
    }
    return crc;
}

uint8_t osdp_checksum(const uint8_t *data, uint16_t length) {
    uint8_t sum = 0u;
    for (uint16_t i = 0; i < length; ++i) sum = (uint8_t)(sum + data[i]);
    return (uint8_t)(0u - sum);
}

bool osdp_decode(const uint8_t *raw, uint16_t length, osdp_frame_t *frame) {
    if (!raw || !frame || length < 6u || raw[0] != 0x53u) return false;
    uint16_t declared = (uint16_t)raw[2] | ((uint16_t)raw[3] << 8u);
    if (declared != length || declared > OSDP_FRAME_MAX) return false;
    memset(frame, 0, sizeof *frame);
    frame->address = raw[1] & 0x7Fu;
    frame->control = raw[4];
    frame->uses_crc = (raw[4] & 0x04u) != 0u;
    frame->secure_block = (raw[4] & 0x08u) != 0u;
    uint16_t trailer = frame->uses_crc ? 2u : 1u;
    if (length < 6u + trailer) return false;
    frame->command = raw[5];
    frame->payload_length = length - 6u - trailer;
    if (frame->payload_length > sizeof frame->payload) return false;
    memcpy(frame->payload, &raw[6], frame->payload_length);
    if (frame->uses_crc) {
        uint16_t expected = (uint16_t)raw[length-2u] | ((uint16_t)raw[length-1u] << 8u);
        frame->valid_check = osdp_crc16(raw, length - 2u) == expected;
    } else {
        uint8_t sum = 0u;
        for (uint16_t i = 0; i < length; ++i) sum = (uint8_t)(sum + raw[i]);
        frame->valid_check = sum == 0u;
    }
    return true;
}

static void reset_osdp(osdp_decoder_t *d) { d->length = 0u; d->expected = 0u; d->escaped = false; }

static void finish_osdp(interface_t iface, osdp_decoder_t *d, uint32_t timestamp_us) {
    osdp_frame_t frame;
    bool decoded = osdp_decode(d->buffer, d->length, &frame);
    uint8_t summary[8] = {0};
    if (decoded) {
        summary[0] = frame.address; summary[1] = frame.control; summary[2] = frame.command;
        summary[3] = frame.secure_block; summary[4] = frame.valid_check;
        summary[5] = (uint8_t)frame.payload_length;
        summary[6] = (uint8_t)d->length; summary[7] = (uint8_t)(d->length >> 8u);
        capture_push(EVT_OSDP_FRAME, iface, summary, sizeof summary, frame.valid_check ? 0u : 1u, timestamp_us);
        policy_observe_osdp(&frame, iface, timestamp_us);
    } else {
        summary[0] = d->length ? d->buffer[0] : 0u;
        summary[1] = (uint8_t)d->length;
        capture_push(EVT_FAULT, iface, summary, 2u, 1u, timestamp_us);
    }
    if (forwarding_enabled && decoded && frame.valid_check) {
        interface_t peer = iface == IFACE_OSDP_READER ? IFACE_OSDP_PANEL : IFACE_OSDP_READER;
        board_rs485_write(peer, d->buffer, d->length);
    }
    reset_osdp(d);
}

void osdp_rx_byte(interface_t iface, uint8_t byte, uint32_t timestamp_us) {
    osdp_decoder_t *d = &osdp[osdp_index(iface)];
    d->last_byte_us = timestamp_us;
    if (d->length == 0u && byte != 0x53u) return;
    if (d->length >= OSDP_FRAME_MAX) { reset_osdp(d); return; }
    d->buffer[d->length++] = byte;
    if (d->length == 4u) {
        d->expected = (uint16_t)d->buffer[2] | ((uint16_t)d->buffer[3] << 8u);
        if (d->expected < 7u || d->expected > OSDP_FRAME_MAX) reset_osdp(d);
    }
    if (d->expected && d->length == d->expected) finish_osdp(iface, d, timestamp_us);
}

void osdp_poll(uint32_t now_us) {
    for (unsigned i = 0; i < ARRAY_SIZE(osdp); ++i) {
        if (osdp[i].length && (uint32_t)(now_us - osdp[i].last_byte_us) > 5000u) {
            uint8_t detail[2] = {(uint8_t)osdp[i].length, (uint8_t)(osdp[i].length >> 8u)};
            capture_push(EVT_FAULT, i ? IFACE_OSDP_PANEL : IFACE_OSDP_READER, detail, 2u, 1u, now_us);
            reset_osdp(&osdp[i]);
        }
    }
}
