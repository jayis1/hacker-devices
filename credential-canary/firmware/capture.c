/* Capture ring and defensive policy engine
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "capture.h"
#include "protocol.h"
#include <string.h>

static capture_event_t ring[CAPTURE_CAPACITY];
static volatile uint16_t write_index;
static volatile uint16_t read_index;
static volatile uint16_t event_sequence;
static volatile uint32_t dropped_events;
static uint32_t alerts;

typedef struct { uint64_t credential; uint8_t bits; uint32_t seen_us; } recent_credential_t;
static recent_credential_t recent[16];
static uint8_t recent_cursor;
static uint8_t known_osdp_address[2];
static bool known_address_valid[2];

void capture_init(void) {
    write_index = read_index = event_sequence = 0u;
    dropped_events = alerts = 0u;
    memset(recent, 0, sizeof recent);
    memset(known_address_valid, 0, sizeof known_address_valid);
}

bool capture_push(event_type_t type, interface_t iface, const uint8_t *data, uint8_t length, uint8_t flags, uint32_t timestamp_us) {
    uint16_t next = (uint16_t)((write_index + 1u) % CAPTURE_CAPACITY);
    if (next == read_index) { dropped_events++; return false; }
    capture_event_t *e = &ring[write_index];
    e->timestamp_us = timestamp_us;
    e->sequence = event_sequence++;
    e->type = (uint8_t)type;
    e->interface_id = (uint8_t)iface;
    e->length = (uint8_t)MIN_U32(length, EVENT_DATA_MAX);
    e->flags = flags;
    if (e->length && data) memcpy(e->data, data, e->length);
    if (e->length < EVENT_DATA_MAX) memset(&e->data[e->length], 0, EVENT_DATA_MAX - e->length);
    write_index = next;
    return true;
}

bool capture_pop(capture_event_t *event) {
    if (!event || read_index == write_index) return false;
    *event = ring[read_index];
    read_index = (uint16_t)((read_index + 1u) % CAPTURE_CAPACITY);
    return true;
}

uint16_t capture_count(void) {
    if (write_index >= read_index) return write_index - read_index;
    return (uint16_t)(CAPTURE_CAPACITY - read_index + write_index);
}
uint32_t capture_dropped(void) { return dropped_events; }

void capture_mark(const char *text) {
    uint8_t length = 0u;
    while (text && text[length] && length < EVENT_DATA_MAX) length++;
    capture_push(EVT_MARKER, IFACE_SYSTEM, (const uint8_t *)text, length, 0u, board_micros());
}

static void raise_alert(alert_code_t code, interface_t iface, const uint8_t *context, uint8_t length, uint32_t timestamp_us) {
    uint8_t data[EVENT_DATA_MAX];
    data[0] = (uint8_t)code;
    uint8_t copied = (uint8_t)MIN_U32(length, EVENT_DATA_MAX - 1u);
    if (copied && context) memcpy(&data[1], context, copied);
    alerts++;
    capture_push(EVT_POLICY, iface, data, copied + 1u, 1u, timestamp_us);
}

void policy_observe_wiegand(uint64_t raw, uint8_t bits, bool parity_ok, uint32_t timestamp_us) {
    uint64_t credential = wiegand_payload(raw, bits);
    uint8_t context[9];
    context[0] = bits;
    for (unsigned i = 0; i < 8u; ++i) context[1u+i] = (uint8_t)(credential >> (i * 8u));
    if (!parity_ok) raise_alert(ALERT_PARITY, IFACE_WIEGAND_A, context, sizeof context, timestamp_us);
    for (unsigned i = 0; i < ARRAY_SIZE(recent); ++i) {
        if (recent[i].bits == bits && recent[i].credential == credential) {
            uint32_t age = timestamp_us - recent[i].seen_us;
            /* Two presentations inside 700 ms often indicate relay/replay tooling,
               while normal anti-passback traffic tends to be farther apart. */
            if (age < 700000u) {
                context[0] = (uint8_t)(age / 1000u);
                context[1] = (uint8_t)(age / 256000u);
                raise_alert(ALERT_REPLAY_WINDOW, IFACE_WIEGAND_A, context, 2u, timestamp_us);
            }
        }
    }
    recent[recent_cursor].credential = credential;
    recent[recent_cursor].bits = bits;
    recent[recent_cursor].seen_us = timestamp_us;
    recent_cursor = (uint8_t)((recent_cursor + 1u) % ARRAY_SIZE(recent));
}

void policy_observe_osdp(const osdp_frame_t *frame, interface_t iface, uint32_t timestamp_us) {
    if (!frame) return;
    uint8_t context[4] = { frame->address, frame->command, frame->control, frame->valid_check };
    if (!frame->valid_check) raise_alert(ALERT_MALFORMED, iface, context, sizeof context, timestamp_us);
    /* POLL/ACK is allowed in clear during setup; credential-bearing RAW, FMT,
       and keypad replies should be protected once secure channel is expected. */
    bool sensitive = frame->command == 0x50u || frame->command == 0x51u || frame->command == 0x54u;
    if (sensitive && !frame->secure_block) raise_alert(ALERT_OSDP_PLAINTEXT, iface, context, sizeof context, timestamp_us);
    unsigned side = iface == IFACE_OSDP_PANEL ? 1u : 0u;
    if (!known_address_valid[side]) {
        known_osdp_address[side] = frame->address;
        known_address_valid[side] = true;
    } else if (known_osdp_address[side] != frame->address) {
        context[2] = known_osdp_address[side];
        raise_alert(ALERT_ADDRESS_CHANGE, iface, context, 3u, timestamp_us);
    }
}

void policy_tamper(bool active) {
    uint8_t state = active ? 1u : 0u;
    capture_push(EVT_TAMPER, IFACE_SYSTEM, &state, 1u, active ? 1u : 0u, board_micros());
    if (active) raise_alert(ALERT_TAMPER, IFACE_SYSTEM, &state, 1u, board_micros());
}

void policy_reset_session(void) {
    memset(recent, 0, sizeof recent);
    recent_cursor = 0u;
    alerts = 0u;
    memset(known_address_valid, 0, sizeof known_address_valid);
    capture_mark("policy-reset");
}
uint32_t policy_alert_count(void) { return alerts; }

const char *policy_alert_name(alert_code_t code) {
    switch (code) {
        case ALERT_PARITY: return "parity";
        case ALERT_REPLAY_WINDOW: return "replay-window";
        case ALERT_OSDP_PLAINTEXT: return "osdp-plaintext";
        case ALERT_ADDRESS_CHANGE: return "address-change";
        case ALERT_MALFORMED: return "malformed";
        case ALERT_LINE_STUCK: return "line-stuck";
        case ALERT_TAMPER: return "tamper";
        default: return "none";
    }
}
