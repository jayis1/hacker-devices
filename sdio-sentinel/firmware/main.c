/*
 * SDIO Sentinel firmware entry point and host-side validation harness
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "board.h"
#include "drivers/frame.h"
#include "drivers/policy.h"
#include "drivers/transport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct runtime {
    struct ss_analyzer analyzer;
    struct ss_policy policy;
    struct ss_decoder decoder;
    uint32_t frames_seen;
    uint32_t anomalies;
    uint32_t now_ms;
};

static uint8_t command_crc_byte(const uint8_t raw[5])
{
    return (uint8_t)((ss_crc7(raw, 5u) << 1u) | 1u);
}

static bool make_command(uint8_t command, uint32_t argument, uint8_t raw[6])
{
    if (raw == NULL || command >= 64u) {
        return false;
    }
    raw[0] = (uint8_t)(0x40u | command);
    raw[1] = (uint8_t)(argument >> 24u);
    raw[2] = (uint8_t)(argument >> 16u);
    raw[3] = (uint8_t)(argument >> 8u);
    raw[4] = (uint8_t)argument;
    raw[5] = command_crc_byte(raw);
    return true;
}

static void runtime_init(struct runtime *runtime)
{
    memset(runtime, 0, sizeof(*runtime));
    ss_analyzer_init(&runtime->analyzer);
    ss_policy_init(&runtime->policy, 1u);
    ss_decoder_init(&runtime->decoder);
    runtime->now_ms = 1u;
}

static enum ss_policy_action process_raw(struct runtime *runtime,
                                         const uint8_t raw[6],
                                         char *json, size_t capacity)
{
    struct ss_frame frame;
    struct ss_analysis analysis;
    memset(&frame, 0, sizeof(frame));
    if (!ss_frame_decode_command(raw, runtime->now_ms, &frame)) {
        return SS_ACTION_BLOCK;
    }
    frame.sequence = (uint16_t)runtime->frames_seen;
    ss_analyze_frame(&runtime->analyzer, &frame, &analysis);
    enum ss_policy_action action = ss_policy_evaluate(&runtime->policy, &frame,
                                                       &analysis, runtime->now_ms);
    runtime->frames_seen++;
    if (analysis.severity >= SS_SEV_NOTICE) {
        runtime->anomalies++;
    }
    if (json != NULL) {
        (void)ss_frame_to_json(&frame, &analysis, json, capacity);
    }
    runtime->now_ms += 17u;
    return action;
}

static int expect(bool condition, const char *message)
{
    if (!condition) {
        (void)fprintf(stderr, "FAIL: %s\n", message);
        return 1;
    }
    return 0;
}

static int test_crc_vectors(void)
{
    int failures = 0;
    const uint8_t cmd0[5] = {0x40u, 0u, 0u, 0u, 0u};
    const uint8_t cmd8[5] = {0x48u, 0u, 0u, 1u, 0xAAu};
    failures += expect(ss_crc7(cmd0, sizeof(cmd0)) == 0x4Au,
                       "CMD0 CRC7 vector");
    failures += expect(ss_crc7(cmd8, sizeof(cmd8)) == 0x43u,
                       "CMD8 CRC7 vector");
    const uint8_t text[] = "123456789";
    failures += expect(ss_crc16(text, sizeof(text) - 1u) == 0x31C3u,
                       "CRC16 CCITT vector");
    failures += expect(ss_crc32(text, sizeof(text) - 1u) == 0xCBF43926u,
                       "CRC32 vector");
    return failures;
}

static int test_decode_and_analysis(void)
{
    int failures = 0;
    struct runtime runtime;
    runtime_init(&runtime);
    runtime.analyzer.state = CARD_STATE_TRANSFER;
    runtime.policy.mode = SS_MODE_PASSIVE_OBSERVE;
    uint8_t raw[6];
    char json[512];
    (void)make_command(17u, 8192u, raw);
    enum ss_policy_action action = process_raw(&runtime, raw, json, sizeof(json));
    failures += expect(action == SS_ACTION_ALLOW,
                       "normal read is allowed in observe mode");
    failures += expect(strstr(json, "READ_SINGLE_BLOCK") != NULL,
                       "JSON contains command name");

    runtime.analyzer.state = CARD_STATE_TRANSFER;
    (void)make_command(24u, 2u, raw);
    action = process_raw(&runtime, raw, json, sizeof(json));
    failures += expect(action == SS_ACTION_ALLOW_AND_LOG,
                       "observe mode logs suspicious boot write without interference");
    failures += expect(strstr(json, "protected boot-region") != NULL,
                       "boot-region finding emitted");
    return failures;
}

static int test_policy_authorization(void)
{
    int failures = 0;
    struct runtime runtime;
    runtime_init(&runtime);
    runtime.analyzer.state = CARD_STATE_TRANSFER;
    uint8_t raw[6];
    char json[512];

    (void)make_command(38u, 0u, raw);
    enum ss_policy_action action = process_raw(&runtime, raw, json, sizeof(json));
    failures += expect(action == SS_ACTION_ALLOW,
                       "safe bypass never interferes with the target");

    runtime.now_ms = 100u;
    ss_policy_note_arm_switch(&runtime.policy, true, runtime.now_ms);
    runtime.now_ms += SS_ARM_HOLD_MS + 1u;
    ss_policy_note_arm_switch(&runtime.policy, true, runtime.now_ms);
    failures += expect(ss_policy_confirm_operator(
                           &runtime.policy,
                           "I HAVE AUTHORIZATION FOR THIS SD/SDIO TARGET",
                           runtime.now_ms),
                       "exact scope statement accepted");
    failures += expect(ss_policy_set_mode(&runtime.policy, SS_MODE_ENFORCE_POLICY,
                                          runtime.now_ms),
                       "physical arm and scope confirmation enable policy mode");
    runtime.analyzer.state = CARD_STATE_TRANSFER;
    action = process_raw(&runtime, raw, json, sizeof(json));
    failures += expect(action == SS_ACTION_BLOCK,
                       "policy mode blocks destructive command by default");
    failures += expect(ss_policy_set_mode(&runtime.policy, SS_MODE_LAB_EMULATION,
                                          runtime.now_ms),
                       "physical arm and scope confirmation enable lab mode");

    runtime.analyzer.state = CARD_STATE_TRANSFER;
    action = process_raw(&runtime, raw, json, sizeof(json));
    failures += expect(action == SS_ACTION_ALLOW_AND_LOG,
                       "authorized lab erase is logged and passed");
    failures += expect(runtime.policy.audit_count >= 2u,
                       "both denied and authorized operations audited");

    ss_policy_tick(&runtime.policy, runtime.now_ms + SS_INACTIVITY_TIMEOUT_MS + 1u);
    failures += expect(runtime.policy.mode == SS_MODE_PASSIVE_OBSERVE,
                       "authorization expiry returns to passive mode");
    return failures;
}

static int test_transport(void)
{
    int failures = 0;
    struct ss_packet source;
    struct ss_packet decoded;
    struct ss_decoder decoder;
    struct ss_status_snapshot status = {
        .board_id = SS_BOARD_ID,
        .uptime_ms = 1234u,
        .frames_seen = 22u,
        .blocked = 3u,
        .anomalies = 4u,
        .clock_hz = 25000000u,
        .mode = SS_MODE_PASSIVE_OBSERVE,
        .card_state = CARD_STATE_TRANSFER,
        .armed = 0u,
        .card_present = 1u
    };
    uint8_t encoded[SS_PACKET_HEADER_SIZE + SS_MAX_HOST_PAYLOAD + 4u];
    failures += expect(ss_packet_make_status(&status, 77u, &source),
                       "status packet construction");
    size_t length = ss_packet_encode(&source, encoded, sizeof(encoded));
    failures += expect(length > 0u, "status packet encoding");
    ss_decoder_init(&decoder);
    bool complete = false;
    for (size_t i = 0u; i < length; ++i) {
        if (ss_decoder_push(&decoder, encoded[i], &decoded)) {
            complete = true;
        }
    }
    failures += expect(complete, "stream decoder completed packet");
    failures += expect(decoded.type == SS_PKT_STATUS && decoded.sequence == 77u,
                       "decoded header matches");
    uint32_t board_id = 0u;
    failures += expect(ss_packet_read_u32(&decoded, 0u, &board_id) &&
                       board_id == SS_BOARD_ID, "decoded board identifier");

    encoded[length - 1u] ^= 0x80u;
    ss_decoder_init(&decoder);
    complete = false;
    for (size_t i = 0u; i < length; ++i) {
        complete = ss_decoder_push(&decoder, encoded[i], &decoded) || complete;
    }
    failures += expect(!complete && decoder.discarded > 0u,
                       "corrupt packet rejected");
    return failures;
}

static int run_self_test(void)
{
    int failures = 0;
    failures += test_crc_vectors();
    failures += test_decode_and_analysis();
    failures += test_policy_authorization();
    failures += test_transport();
    if (failures == 0) {
        (void)puts("SDIO Sentinel self-test: PASS (author jayis1)");
        return EXIT_SUCCESS;
    }
    (void)fprintf(stderr, "SDIO Sentinel self-test: %d failure(s)\n", failures);
    return EXIT_FAILURE;
}

static int run_demo(void)
{
    struct runtime runtime;
    runtime_init(&runtime);
    runtime.analyzer.state = CARD_STATE_TRANSFER;
    runtime.policy.mode = SS_MODE_PASSIVE_OBSERVE;
    const struct { uint8_t command; uint32_t argument; } demo[] = {
        {17u, 0x00004000u}, {18u, 0x00008000u}, {24u, 0x00000002u},
        {52u, 0x00000000u}, {52u, 0x80002001u}, {42u, 0u},
        {38u, 0u}, {13u, 0u}
    };
    for (size_t i = 0u; i < sizeof(demo) / sizeof(demo[0]); ++i) {
        uint8_t raw[6];
        char json[512];
        runtime.analyzer.state = CARD_STATE_TRANSFER;
        (void)make_command(demo[i].command, demo[i].argument, raw);
        enum ss_policy_action action = process_raw(&runtime, raw, json, sizeof(json));
        (void)printf("%s action=%u\n", json, (unsigned)action);
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--self-test") == 0) {
        return run_self_test();
    }
    if (argc == 2 && strcmp(argv[1], "--demo") == 0) {
        return run_demo();
    }
    (void)printf("SDIO Sentinel %s by jayis1\n", SS_FIRMWARE_VERSION);
    (void)puts("usage: sdio-sentinel --self-test | --demo");
    return argc == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
