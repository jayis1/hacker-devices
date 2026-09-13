/* Framed USB/BLE command service
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "command.h"
#include "capture.h"
#include "protocol.h"
#include <string.h>

#define SOF 0xCAu
#define RESP 0x80u
enum { CMD_INFO=1, CMD_STATUS=2, CMD_CAPTURE_START=3, CMD_CAPTURE_STOP=4, CMD_SET_MODE=5, CMD_MARK=6, CMD_RESET_POLICY=7 };
static uint8_t rx[COMMAND_MAX];
static uint16_t rx_length;
static bool streaming;
static operating_mode_t selected_mode;

static uint16_t crc16(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFFu;
    for (uint16_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (unsigned b = 0; b < 8u; ++b) crc = (crc & 1u) ? (uint16_t)((crc >> 1u) ^ 0xA001u) : (uint16_t)(crc >> 1u);
    }
    return crc;
}

static void send_packet(uint8_t type, const uint8_t *payload, uint8_t length) {
    uint8_t out[64];
    if (length > 58u) length = 58u;
    out[0] = SOF; out[1] = type; out[2] = length;
    if (length && payload) memcpy(&out[3], payload, length);
    uint16_t check = crc16(out, (uint16_t)(3u + length));
    out[3u + length] = (uint8_t)check;
    out[4u + length] = (uint8_t)(check >> 8u);
    board_usb_write(out, (uint16_t)(5u + length));
}

static void respond_status(void) {
    uint8_t p[16];
    uint32_t now = board_millis();
    uint32_t alert_count = policy_alert_count();
    uint32_t drops = capture_dropped();
    p[0] = (uint8_t)selected_mode; p[1] = streaming; p[2] = board_tamper_asserted(); p[3] = 0u;
    p[4] = (uint8_t)now; p[5] = (uint8_t)(now >> 8u); p[6] = (uint8_t)(now >> 16u); p[7] = (uint8_t)(now >> 24u);
    p[8] = (uint8_t)alert_count; p[9] = (uint8_t)(alert_count >> 8u); p[10] = (uint8_t)(alert_count >> 16u); p[11] = (uint8_t)(alert_count >> 24u);
    p[12] = (uint8_t)drops; p[13] = (uint8_t)(drops >> 8u); p[14] = (uint8_t)capture_count(); p[15] = (uint8_t)(capture_count() >> 8u);
    send_packet(CMD_STATUS | RESP, p, sizeof p);
}

static void execute(const uint8_t *packet, uint16_t length) {
    if (length < 5u || packet[0] != SOF) return;
    uint8_t command = packet[1], payload_length = packet[2];
    if ((uint16_t)(payload_length + 5u) != length) return;
    uint16_t received = (uint16_t)packet[length-2u] | ((uint16_t)packet[length-1u] << 8u);
    if (crc16(packet, length - 2u) != received) { uint8_t error=1u; send_packet(RESP, &error, 1u); return; }
    const uint8_t *p = &packet[3];
    switch (command) {
        case CMD_INFO: {
            static const uint8_t info[] = "Credential Canary 1.0;author=jayis1";
            send_packet(CMD_INFO | RESP, info, sizeof info - 1u);
            break;
        }
        case CMD_STATUS: respond_status(); break;
        case CMD_CAPTURE_START: streaming = true; capture_mark("capture-start"); send_packet(command | RESP, 0, 0); break;
        case CMD_CAPTURE_STOP: streaming = false; capture_mark("capture-stop"); send_packet(command | RESP, 0, 0); break;
        case CMD_SET_MODE:
            if (payload_length != 1u || p[0] > MODE_LAB_TEST) { uint8_t e=2u; send_packet(RESP, &e, 1u); break; }
            if (p[0] == MODE_LAB_TEST && !board_authorization_asserted()) { uint8_t e=4u; send_packet(RESP, &e, 1u); break; }
            selected_mode = (operating_mode_t)p[0];
            /* Active forwarding is deliberately limited to bridge mode. Lab
               tests remain locally gated by physical authorization button. */
            protocol_set_forwarding(selected_mode == MODE_BRIDGE);
            board_set_bypass(selected_mode == MODE_SAFE_BYPASS);
            send_packet(command | RESP, p, 1u);
            break;
        case CMD_MARK:
            if (payload_length) { char text[33]; uint8_t n=(uint8_t)MIN_U32(payload_length,32u); memcpy(text,p,n); text[n]=0; capture_mark(text); }
            send_packet(command | RESP, 0, 0);
            break;
        case CMD_RESET_POLICY: policy_reset_session(); send_packet(command | RESP, 0, 0); break;
        default: { uint8_t e=3u; send_packet(RESP, &e, 1u); break; }
    }
}

void command_init(void) { rx_length = 0u; streaming = false; selected_mode = MODE_SAFE_BYPASS; }

void command_rx(const uint8_t *data, uint16_t length) {
    for (uint16_t i = 0; i < length; ++i) {
        uint8_t byte = data[i];
        if (rx_length == 0u && byte != SOF) continue;
        if (rx_length >= sizeof rx) { rx_length = 0u; continue; }
        rx[rx_length++] = byte;
        if (rx_length >= 3u) {
            uint16_t expected = (uint16_t)rx[2] + 5u;
            if (expected > sizeof rx) { rx_length = 0u; continue; }
            if (rx_length == expected) { execute(rx, rx_length); rx_length = 0u; }
        }
    }
}

void command_stream_event(const capture_event_t *event) {
    if (!streaming || !event) return;
    uint8_t payload[48];
    payload[0]=(uint8_t)event->sequence; payload[1]=(uint8_t)(event->sequence>>8u);
    payload[2]=(uint8_t)event->timestamp_us; payload[3]=(uint8_t)(event->timestamp_us>>8u);
    payload[4]=(uint8_t)(event->timestamp_us>>16u); payload[5]=(uint8_t)(event->timestamp_us>>24u);
    payload[6]=event->type; payload[7]=event->interface_id; payload[8]=event->flags; payload[9]=event->length;
    memcpy(&payload[10], event->data, event->length);
    send_packet(0xF0u, payload, (uint8_t)(10u + event->length));
}

void command_poll(void) {
    uint8_t incoming[64];
    int got = board_usb_read(incoming, sizeof incoming);
    if (got > 0) command_rx(incoming, (uint16_t)got);
}
