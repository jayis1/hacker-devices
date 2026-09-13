/* DALI Sentinel protocol-aware security analyzer
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "analyzer.h"
#include <string.h>

typedef struct { finding_t item[FINDING_CAPACITY]; uint8_t write, read; } finding_ring_t;
static finding_ring_t findings;
static analyzer_stats_t stats;
static uint32_t low_started[2];
static bool low_active[2];
static uint32_t last_reply_raw;
static uint32_t last_reply_ms;
static uint8_t comparison_count;

enum {
    SPECIAL_TERMINATE = 0xA1,
    SPECIAL_DTR0 = 0xA3,
    SPECIAL_INITIALISE = 0xA5,
    SPECIAL_RANDOMISE = 0xA7,
    SPECIAL_COMPARE = 0xA9,
    SPECIAL_WITHDRAW = 0xAB,
    SPECIAL_SEARCHADDRH = 0xB1,
    SPECIAL_SEARCHADDRM = 0xB3,
    SPECIAL_SEARCHADDRL = 0xB5,
    SPECIAL_PROGRAM_SHORT = 0xB7,
    CMD_OFF = 0x00,
    CMD_RECALL_MAX = 0x05
};

static uint8_t ring_next(uint8_t value) { return (uint8_t)((value + 1u) % FINDING_CAPACITY); }

static void emit(finding_code_t code, finding_severity_t severity, uint32_t now_ms,
                 uint32_t raw, uint16_t context, bus_side_t side)
{
    uint8_t next = ring_next(findings.write);
    if (next == findings.read) findings.read = ring_next(findings.read);
    finding_t *out = &findings.item[findings.write];
    out->timestamp_ms = now_ms;
    out->code = code;
    out->severity = severity;
    out->evidence_raw = raw;
    out->context = context;
    out->side = side;
    findings.write = next;
}

static bool in_maintenance_window(uint32_t minute)
{
    uint32_t start = stats.maintenance_start_minute;
    uint32_t end = stats.maintenance_end_minute;
    if (start == end) return true;
    if (start < end) return minute >= start && minute < end;
    return minute >= start || minute < end;
}

static bool is_special(uint8_t address)
{
    switch (address) {
    case SPECIAL_TERMINATE: case SPECIAL_DTR0: case SPECIAL_INITIALISE:
    case SPECIAL_RANDOMISE: case SPECIAL_COMPARE: case SPECIAL_WITHDRAW:
    case SPECIAL_SEARCHADDRH: case SPECIAL_SEARCHADDRM: case SPECIAL_SEARCHADDRL:
    case SPECIAL_PROGRAM_SHORT: return true;
    default: return false;
    }
}

static bool is_commissioning(uint8_t address)
{
    return address == SPECIAL_INITIALISE || address == SPECIAL_RANDOMISE ||
           address == SPECIAL_COMPARE || address == SPECIAL_WITHDRAW ||
           address == SPECIAL_SEARCHADDRH || address == SPECIAL_SEARCHADDRM ||
           address == SPECIAL_SEARCHADDRL || address == SPECIAL_PROGRAM_SHORT ||
           address == SPECIAL_TERMINATE;
}

static void analyze_commissioning(uint8_t special, uint8_t data, const dali_frame_t *frame,
                                  uint32_t now_ms, uint32_t minute)
{
    (void)data;
    stats.commissioning_commands++;
    if (!in_maintenance_window(minute)) {
        emit(FIND_COMMISSIONING_OUTSIDE_WINDOW, SEV_HIGH, now_ms, frame->raw,
             special, frame->side);
    }
    switch (special) {
    case SPECIAL_INITIALISE:
        stats.commissioning_state = 1u;
        comparison_count = 0u;
        emit(FIND_COMMISSIONING_START, SEV_MEDIUM, now_ms, frame->raw, 0u, frame->side);
        break;
    case SPECIAL_RANDOMISE:
        if (stats.commissioning_state != 1u) {
            emit(FIND_COMMISSIONING_SEQUENCE, SEV_HIGH, now_ms, frame->raw, 1u, frame->side);
        }
        stats.commissioning_state = 2u;
        break;
    case SPECIAL_SEARCHADDRH:
    case SPECIAL_SEARCHADDRM:
    case SPECIAL_SEARCHADDRL:
        if (stats.commissioning_state < 2u) {
            emit(FIND_COMMISSIONING_SEQUENCE, SEV_MEDIUM, now_ms, frame->raw, 2u, frame->side);
        }
        stats.commissioning_state = 3u;
        break;
    case SPECIAL_COMPARE:
        comparison_count++;
        if (stats.commissioning_state < 3u) {
            emit(FIND_COMMISSIONING_SEQUENCE, SEV_MEDIUM, now_ms, frame->raw, 3u, frame->side);
        }
        if (comparison_count > 96u) {
            emit(FIND_RATE_HIGH, SEV_MEDIUM, now_ms, frame->raw, comparison_count, frame->side);
        }
        break;
    case SPECIAL_PROGRAM_SHORT:
        if (stats.commissioning_state < 3u || comparison_count == 0u) {
            emit(FIND_COMMISSIONING_SEQUENCE, SEV_HIGH, now_ms, frame->raw, 4u, frame->side);
        }
        emit(FIND_ADDRESS_PROGRAMMED, SEV_HIGH, now_ms, frame->raw, data, frame->side);
        stats.commissioning_state = 4u;
        break;
    case SPECIAL_WITHDRAW:
        if (stats.commissioning_state < 4u) {
            emit(FIND_COMMISSIONING_SEQUENCE, SEV_LOW, now_ms, frame->raw, 5u, frame->side);
        }
        stats.commissioning_state = 2u;
        break;
    case SPECIAL_TERMINATE:
        stats.commissioning_state = 0u;
        comparison_count = 0u;
        break;
    default:
        break;
    }
}

static void analyze_forward(const dali_frame_t *frame, uint32_t now_ms, uint32_t minute)
{
    uint8_t address = (uint8_t)(frame->raw >> 8u);
    uint8_t data = (uint8_t)frame->raw;
    stats.last_forward_ms = now_ms;
    if (is_special(address)) {
        if (is_commissioning(address)) analyze_commissioning(address, data, frame, now_ms, minute);
        return;
    }
    bool command = (address & 1u) != 0u;
    bool broadcast = (address & 0xFEu) == 0xFEu;
    if (broadcast) {
        stats.broadcasts++;
        if (!command || data == CMD_OFF) {
            emit(FIND_BROADCAST_OFF, SEV_HIGH, now_ms, frame->raw, data, frame->side);
        } else if (data == CMD_RECALL_MAX) {
            emit(FIND_BROADCAST_MAX, SEV_MEDIUM, now_ms, frame->raw, data, frame->side);
        }
    }
    if (command && (data & 0xF0u) == 0x90u) {
        stats.last_query_address = (uint8_t)((address >> 1u) & 0x3Fu);
    }
}

static void analyze_backward(const dali_frame_t *frame, uint32_t now_ms)
{
    stats.last_backward_ms = now_ms;
    if (last_reply_ms != 0u && now_ms - last_reply_ms < 4u && frame->raw != last_reply_raw) {
        emit(FIND_DUPLICATE_REPLY, SEV_MEDIUM, now_ms, frame->raw,
             (uint16_t)last_reply_raw, frame->side);
    }
    last_reply_ms = now_ms;
    last_reply_raw = frame->raw;
}

void analyzer_init(uint32_t maintenance_start_minute, uint32_t maintenance_end_minute)
{
    memset(&findings, 0, sizeof(findings));
    memset(&stats, 0, sizeof(stats));
    memset(low_started, 0, sizeof(low_started));
    memset(low_active, 0, sizeof(low_active));
    stats.maintenance_start_minute = maintenance_start_minute % 1440u;
    stats.maintenance_end_minute = maintenance_end_minute % 1440u;
    last_reply_raw = 0u;
    last_reply_ms = 0u;
    comparison_count = 0u;
}

void analyzer_consume(const dali_frame_t *frame, uint32_t now_ms, uint32_t minute_of_day)
{
    if (frame == NULL) return;
    stats.frames_total++;
    if (stats.window_started_ms == 0u) stats.window_started_ms = now_ms;
    if (now_ms - stats.window_started_ms >= 1000u) {
        if (stats.frames_in_window > 80u) {
            emit(FIND_RATE_HIGH, SEV_MEDIUM, now_ms, frame->raw,
                 (uint16_t)stats.frames_in_window, frame->side);
        }
        stats.window_started_ms = now_ms;
        stats.frames_in_window = 0u;
    }
    stats.frames_in_window++;
    if (!frame->framing_ok) {
        stats.frames_bad++;
        emit(FIND_BAD_FRAMING, SEV_LOW, now_ms, frame->raw, frame->quality, frame->side);
    }
    if (frame->collision_score >= 3u) {
        stats.collisions++;
        emit(FIND_COLLISION, SEV_MEDIUM, now_ms, frame->raw,
             frame->collision_score, frame->side);
    }
    if (!frame->framing_ok) return;
    if (frame->bit_count == 16u) analyze_forward(frame, now_ms, minute_of_day);
    else if (frame->bit_count == 8u) analyze_backward(frame, now_ms);
}

void analyzer_note_bus(bool low, uint32_t now_ms, bus_side_t side)
{
    if (side != SIDE_CONTROLLER && side != SIDE_GEAR) return;
    unsigned index = (unsigned)side;
    if (low && !low_active[index]) {
        low_active[index] = true;
        low_started[index] = now_ms;
    } else if (!low && low_active[index]) {
        uint32_t duration = now_ms - low_started[index];
        if (duration >= 500u) {
            emit(FIND_STUCK_LOW, duration >= 5000u ? SEV_CRITICAL : SEV_HIGH,
                 now_ms, 0u, duration > UINT16_MAX ? UINT16_MAX : (uint16_t)duration, side);
        }
        low_active[index] = false;
    } else if (low && now_ms - low_started[index] == 5000u) {
        emit(FIND_STUCK_LOW, SEV_CRITICAL, now_ms, 0u, 5000u, side);
    }
}

void analyzer_note_capture_loss(uint32_t count, uint32_t now_ms)
{
    if (count) emit(FIND_CAPTURE_LOSS, SEV_HIGH, now_ms, count,
                    count > UINT16_MAX ? UINT16_MAX : (uint16_t)count, SIDE_UNKNOWN);
}

bool analyzer_next_finding(finding_t *finding)
{
    if (finding == NULL || findings.read == findings.write) return false;
    *finding = findings.item[findings.read];
    findings.read = ring_next(findings.read);
    return true;
}

analyzer_stats_t analyzer_stats(void) { return stats; }

const char *analyzer_finding_name(finding_code_t code)
{
    switch (code) {
    case FIND_BAD_FRAMING: return "BAD_FRAMING";
    case FIND_COLLISION: return "COLLISION";
    case FIND_BROADCAST_OFF: return "BROADCAST_OFF";
    case FIND_BROADCAST_MAX: return "BROADCAST_MAX";
    case FIND_COMMISSIONING_START: return "COMMISSIONING_START";
    case FIND_COMMISSIONING_SEQUENCE: return "COMMISSIONING_SEQUENCE";
    case FIND_COMMISSIONING_OUTSIDE_WINDOW: return "COMMISSIONING_OUTSIDE_WINDOW";
    case FIND_ADDRESS_PROGRAMMED: return "ADDRESS_PROGRAMMED";
    case FIND_RATE_HIGH: return "RATE_HIGH";
    case FIND_STUCK_LOW: return "STUCK_LOW";
    case FIND_DUPLICATE_REPLY: return "DUPLICATE_REPLY";
    case FIND_CAPTURE_LOSS: return "CAPTURE_LOSS";
    case FIND_SAFETY_FAULT: return "SAFETY_FAULT";
    default: return "NONE";
    }
}

bool analyzer_run_self_test(void)
{
    analyzer_init(60u, 120u);
    dali_frame_t frame = {0};
    frame.framing_ok = true;
    frame.bit_count = 16u;
    frame.raw = 0xFE00u;
    frame.side = SIDE_CONTROLLER;
    frame.quality = 95u;
    analyzer_consume(&frame, 1000u, 30u);
    frame.raw = 0xA500u;
    analyzer_consume(&frame, 1010u, 30u);
    finding_t finding;
    bool saw_off = false, saw_window = false;
    while (analyzer_next_finding(&finding)) {
        if (finding.code == FIND_BROADCAST_OFF) saw_off = true;
        if (finding.code == FIND_COMMISSIONING_OUTSIDE_WINDOW) saw_window = true;
    }
    analyzer_note_bus(true, 2000u, SIDE_GEAR);
    analyzer_note_bus(false, 8000u, SIDE_GEAR);
    if (!analyzer_next_finding(&finding) || finding.code != FIND_STUCK_LOW) return false;
    return saw_off && saw_window && stats.frames_total == 2u;
}
