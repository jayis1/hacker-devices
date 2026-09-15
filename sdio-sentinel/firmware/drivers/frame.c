/*
 * SD/SDIO command decoding and behavior analysis
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "frame.h"

#include <stdio.h>
#include <string.h>

struct command_info {
    const char *name;
    uint32_t legal_states;
    uint8_t next_state;
    uint8_t score;
};

#define STATE_BIT(x) (1u << (x))
#define ANY_STATE 0x3FFu
#define KEEP_STATE 0xFFu

static const struct command_info command_table[64] = {
    [0]  = {"GO_IDLE_STATE", ANY_STATE, CARD_STATE_IDLE, 0},
    [1]  = {"SEND_OP_COND", STATE_BIT(CARD_STATE_IDLE), CARD_STATE_READY, 2},
    [2]  = {"ALL_SEND_CID", STATE_BIT(CARD_STATE_READY), CARD_STATE_IDENT, 0},
    [3]  = {"SEND_RELATIVE_ADDR", STATE_BIT(CARD_STATE_IDENT), CARD_STATE_STANDBY, 0},
    [4]  = {"SET_DSR", STATE_BIT(CARD_STATE_STANDBY), KEEP_STATE, 4},
    [5]  = {"IO_SEND_OP_COND", STATE_BIT(CARD_STATE_IDLE), CARD_STATE_READY, 3},
    [6]  = {"SWITCH_FUNC", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 5},
    [7]  = {"SELECT_CARD", STATE_BIT(CARD_STATE_STANDBY) | STATE_BIT(CARD_STATE_TRANSFER), CARD_STATE_TRANSFER, 0},
    [8]  = {"SEND_IF_COND", STATE_BIT(CARD_STATE_IDLE), KEEP_STATE, 0},
    [9]  = {"SEND_CSD", STATE_BIT(CARD_STATE_STANDBY), KEEP_STATE, 0},
    [10] = {"SEND_CID", STATE_BIT(CARD_STATE_STANDBY), KEEP_STATE, 0},
    [11] = {"VOLTAGE_SWITCH", STATE_BIT(CARD_STATE_READY), KEEP_STATE, 12},
    [12] = {"STOP_TRANSMISSION", STATE_BIT(CARD_STATE_DATA) | STATE_BIT(CARD_STATE_RECEIVE), CARD_STATE_TRANSFER, 0},
    [13] = {"SEND_STATUS", ANY_STATE, KEEP_STATE, 0},
    [15] = {"GO_INACTIVE_STATE", ANY_STATE, CARD_STATE_DISCONNECT, 1},
    [16] = {"SET_BLOCKLEN", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 2},
    [17] = {"READ_SINGLE_BLOCK", STATE_BIT(CARD_STATE_TRANSFER), CARD_STATE_DATA, 0},
    [18] = {"READ_MULTIPLE_BLOCK", STATE_BIT(CARD_STATE_TRANSFER), CARD_STATE_DATA, 1},
    [19] = {"SEND_TUNING_BLOCK", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 2},
    [20] = {"SPEED_CLASS_CONTROL", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 7},
    [23] = {"SET_BLOCK_COUNT", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 1},
    [24] = {"WRITE_BLOCK", STATE_BIT(CARD_STATE_TRANSFER), CARD_STATE_RECEIVE, 8},
    [25] = {"WRITE_MULTIPLE_BLOCK", STATE_BIT(CARD_STATE_TRANSFER), CARD_STATE_RECEIVE, 10},
    [27] = {"PROGRAM_CSD", STATE_BIT(CARD_STATE_TRANSFER), CARD_STATE_PROGRAMMING, 18},
    [28] = {"SET_WRITE_PROT", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 9},
    [29] = {"CLR_WRITE_PROT", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 14},
    [30] = {"SEND_WRITE_PROT", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 2},
    [32] = {"ERASE_WR_BLK_START", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 13},
    [33] = {"ERASE_WR_BLK_END", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 13},
    [38] = {"ERASE", STATE_BIT(CARD_STATE_TRANSFER), CARD_STATE_PROGRAMMING, 25},
    [42] = {"LOCK_UNLOCK", STATE_BIT(CARD_STATE_TRANSFER), CARD_STATE_DATA, 30},
    [52] = {"IO_RW_DIRECT", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 8},
    [53] = {"IO_RW_EXTENDED", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 10},
    [55] = {"APP_CMD", ANY_STATE, KEEP_STATE, 0},
    [56] = {"GEN_CMD", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 12},
    [58] = {"READ_OCR", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 0},
    [59] = {"CRC_ON_OFF", STATE_BIT(CARD_STATE_TRANSFER), KEEP_STATE, 5}
};

static void analysis_reset(struct ss_analysis *analysis)
{
    analysis->severity = SS_SEV_INFO;
    analysis->score = 0u;
    analysis->flags = 0u;
    (void)snprintf(analysis->summary, sizeof(analysis->summary), "normal transaction");
}

static void add_finding(struct ss_analysis *analysis, uint32_t flag,
                        uint16_t score, const char *summary)
{
    if ((analysis->flags & flag) == 0u) {
        uint32_t total = (uint32_t)analysis->score + score;
        analysis->score = (uint16_t)(total > 100u ? 100u : total);
        analysis->flags |= flag;
    }
    if (summary != NULL && analysis->severity < SS_SEV_CRITICAL) {
        (void)snprintf(analysis->summary, sizeof(analysis->summary), "%s", summary);
    }
}

static void update_severity(struct ss_analysis *analysis)
{
    if (analysis->score >= 70u) {
        analysis->severity = SS_SEV_CRITICAL;
    } else if (analysis->score >= 35u) {
        analysis->severity = SS_SEV_WARNING;
    } else if (analysis->score >= 10u) {
        analysis->severity = SS_SEV_NOTICE;
    } else {
        analysis->severity = SS_SEV_INFO;
    }
}

static bool command_is_write(uint8_t command)
{
    return command == 24u || command == 25u || command == 27u ||
           command == 28u || command == 29u || command == 32u ||
           command == 33u || command == 38u || command == 42u;
}

static bool targets_boot_region(const struct ss_analyzer *analyzer,
                                const struct ss_frame *frame)
{
    if (!command_is_write(frame->command)) {
        return false;
    }
    uint32_t block = analyzer->high_capacity ? frame->argument : frame->argument / 512u;
    return block < 8192u;
}

static void analyze_sequence(struct ss_analyzer *analyzer,
                             const struct ss_frame *frame,
                             struct ss_analysis *analysis)
{
    const struct command_info *info = &command_table[frame->command];
    uint32_t state_bit = STATE_BIT(analyzer->state);
    if (info->legal_states != 0u && (info->legal_states & state_bit) == 0u) {
        add_finding(analysis, SS_FLAG_STATE_VIOLATION, 24u,
                    "command is inconsistent with observed card state");
    }
    if (frame->sequence != analyzer->next_sequence) {
        add_finding(analysis, SS_FLAG_ILLEGAL_SEQUENCE, 8u,
                    "capture sequence gap or replay observed");
    }
    analyzer->next_sequence = (uint16_t)(frame->sequence + 1u);
    if (info->next_state != KEEP_STATE && info->legal_states != 0u) {
        analyzer->state = info->next_state;
    }
}

static void analyze_command_risk(struct ss_analyzer *analyzer,
                                 const struct ss_frame *frame,
                                 struct ss_analysis *analysis)
{
    const struct command_info *info = &command_table[frame->command];
    if (info->name == NULL) {
        add_finding(analysis, SS_FLAG_VENDOR_COMMAND, 15u,
                    "reserved or vendor-specific command observed");
    } else if (info->score > 0u) {
        add_finding(analysis, SS_FLAG_VENDOR_COMMAND, info->score, info->name);
    }

    if (frame->command == 0u) {
        if (analyzer->last_init_ms != 0u &&
            frame->timestamp_ms - analyzer->last_init_ms < 1000u) {
            add_finding(analysis, SS_FLAG_RAPID_REINIT, 20u,
                        "rapid card reinitialization may indicate instability");
        }
        analyzer->last_init_ms = frame->timestamp_ms;
    }
    if (frame->command == 5u || frame->command == 52u || frame->command == 53u) {
        analyzer->sdio_present = true;
        add_finding(analysis, SS_FLAG_SDIO_FUNCTION_IO, 8u,
                    "SDIO function access requires scope validation");
    }
    if (frame->command == 11u) {
        add_finding(analysis, SS_FLAG_VOLTAGE_SWITCH, 10u,
                    "1.8 V signaling transition observed");
    }
    if (frame->command == 42u) {
        add_finding(analysis, SS_FLAG_LOCK_UNLOCK, 30u,
                    "card lock or unlock transaction observed");
    }
    if (targets_boot_region(analyzer, frame)) {
        add_finding(analysis, SS_FLAG_WRITE_BOOT_REGION, 45u,
                    "write targets protected boot-region policy window");
    }
}

void ss_analyzer_init(struct ss_analyzer *analyzer)
{
    if (analyzer == NULL) {
        return;
    }
    memset(analyzer, 0, sizeof(*analyzer));
    analyzer->state = CARD_STATE_NO_MEDIA;
    analyzer->clock_hz = SS_DEFAULT_CLOCK_LIMIT_HZ;
}

uint8_t ss_crc7(const uint8_t *data, size_t length)
{
    uint8_t crc = 0u;
    if (data == NULL) {
        return 0u;
    }
    for (size_t i = 0u; i < length; ++i) {
        uint8_t value = data[i];
        for (uint8_t bit = 0u; bit < 8u; ++bit) {
            crc <<= 1u;
            if (((value ^ crc) & 0x80u) != 0u) {
                crc ^= 0x09u;
            }
            value <<= 1u;
        }
    }
    return (uint8_t)(crc & 0x7Fu);
}

uint16_t ss_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0u;
    if (data == NULL) {
        return 0u;
    }
    for (size_t i = 0u; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8u;
        for (uint8_t bit = 0u; bit < 8u; ++bit) {
            crc = (crc & 0x8000u) != 0u ? (uint16_t)((crc << 1u) ^ 0x1021u)
                                            : (uint16_t)(crc << 1u);
        }
    }
    return crc;
}

bool ss_frame_decode_command(const uint8_t raw[6], uint32_t timestamp_ms,
                             struct ss_frame *frame)
{
    if (raw == NULL || frame == NULL || (raw[0] & 0xC0u) != 0x40u ||
        (raw[5] & 0x01u) == 0u) {
        return false;
    }
    memset(frame, 0, sizeof(*frame));
    frame->timestamp_ms = timestamp_ms;
    frame->command = raw[0] & 0x3Fu;
    frame->argument = ((uint32_t)raw[1] << 24u) |
                      ((uint32_t)raw[2] << 16u) |
                      ((uint32_t)raw[3] << 8u) |
                      (uint32_t)raw[4];
    frame->direction = SS_DIR_HOST_TO_CARD;
    frame->crc_valid = ss_crc7(raw, 5u) == (uint8_t)(raw[5] >> 1u);
    return true;
}

void ss_analyze_frame(struct ss_analyzer *analyzer, const struct ss_frame *frame,
                      struct ss_analysis *analysis)
{
    if (analyzer == NULL || frame == NULL || analysis == NULL) {
        return;
    }
    analysis_reset(analysis);
    if (frame->command < 64u) {
        analyzer->command_count[frame->command]++;
    }
    if (!frame->crc_valid) {
        analyzer->bad_crc_count++;
        add_finding(analysis, SS_FLAG_BAD_CRC, 18u, "command CRC7 failed");
    }
    if (frame->timed_out) {
        analyzer->timeout_count++;
        add_finding(analysis, SS_FLAG_TIMEOUT, 15u, "transaction timed out");
    }
    if (frame->data_length > SS_MAX_FRAME_BYTES) {
        add_finding(analysis, SS_FLAG_OVERSIZED_FRAME, 30u,
                    "frame length exceeded capture boundary");
    }
    if (analyzer->clock_hz > SS_MAX_CLOCK_HZ) {
        add_finding(analysis, SS_FLAG_CLOCK_EXCESS, 20u,
                    "observed SD clock exceeds design limit");
    }
    if (command_is_write(frame->command)) {
        analyzer->write_count++;
    }
    if (frame->command == 17u || frame->command == 18u) {
        analyzer->read_count++;
    }
    analyze_sequence(analyzer, frame, analysis);
    analyze_command_risk(analyzer, frame, analysis);
    update_severity(analysis);
}

const char *ss_command_name(uint8_t command)
{
    if (command >= 64u || command_table[command].name == NULL) {
        return "RESERVED";
    }
    return command_table[command].name;
}

bool ss_frame_to_json(const struct ss_frame *frame, const struct ss_analysis *analysis,
                      char *output, size_t capacity)
{
    if (frame == NULL || analysis == NULL || output == NULL || capacity == 0u) {
        return false;
    }
    int written = snprintf(output, capacity,
        "{\"author\":\"jayis1\",\"seq\":%u,\"time_ms\":%u,\"direction\":\"%s\","
        "\"command\":%u,\"name\":\"%s\",\"argument\":%u,\"crc_ok\":%s,"
        "\"severity\":%u,\"score\":%u,\"flags\":%u,\"summary\":\"%s\"}",
        (unsigned)frame->sequence, (unsigned)frame->timestamp_ms,
        frame->direction == SS_DIR_HOST_TO_CARD ? "host-to-card" : "card-to-host",
        (unsigned)frame->command, ss_command_name(frame->command),
        (unsigned)frame->argument, frame->crc_valid ? "true" : "false",
        (unsigned)analysis->severity, (unsigned)analysis->score,
        (unsigned)analysis->flags, analysis->summary);
    return written >= 0 && (size_t)written < capacity;
}
