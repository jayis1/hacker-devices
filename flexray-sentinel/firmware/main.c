/*
 * FlexRay Sentinel firmware entry point and deterministic self-test
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "board.h"
#include "detector.h"
#include "flexray.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEMO_FRAME_COUNT 12u
#define CYCLE_TICKS 400000u

static fr_frame_t make_frame(uint16_t slot,
                             uint8_t cycle,
                             uint64_t timestamp,
                             fs_channel_t channel,
                             uint16_t payload_length,
                             uint8_t seed)
{
    fr_frame_t frame;
    uint16_t index;
    memset(&frame, 0, sizeof(frame));
    frame.timestamp_ticks = timestamp;
    frame.slot_id = slot;
    frame.payload_length = payload_length;
    frame.cycle = cycle;
    frame.channel = channel;
    frame.flags = slot == 1u ? (FS_FRAME_FLAG_STARTUP | FS_FRAME_FLAG_SYNC) : 0u;
    for (index = 0u; index < payload_length; ++index) {
        frame.payload[index] = (uint8_t)(seed + index * 3u);
    }
    return frame;
}

static bool test_crc_reference(void)
{
    const uint8_t bytes[] = { 0x10u, 0x20u, 0x30u, 0x40u, 0x50u };
    const uint32_t first = fr_frame_crc24(bytes, sizeof(bytes));
    const uint32_t second = fr_frame_crc24(bytes, sizeof(bytes));
    return first != 0u && first == second;
}

static bool test_round_trip(void)
{
    fr_decoder_t decoder;
    fr_frame_t source = make_frame(44u, 7u, 9000123u, FS_CHANNEL_A, 16u, 0x31u);
    fr_frame_t decoded;
    uint8_t encoded[FR_ENCODED_RECORD_MAX];
    const size_t length = fr_encode_record(&source, encoded, sizeof(encoded));

    fr_decoder_init(&decoder);
    if (length == 0u) {
        return false;
    }
    {
        const fr_decode_result_t result = fr_decode_record(&decoder, encoded, length, &decoded);
        if (result != FR_DECODE_OK) {
            printf("round-trip decode result: %s\n", fr_decode_result_name(result));
            return false;
        }
    }
    return decoded.slot_id == source.slot_id &&
           decoded.cycle == source.cycle &&
           decoded.channel == source.channel &&
           decoded.payload_length == source.payload_length &&
           decoded.timestamp_ticks == source.timestamp_ticks &&
           memcmp(decoded.payload, source.payload, source.payload_length) == 0;
}

static bool test_crc_corruption(void)
{
    fr_decoder_t decoder;
    fr_frame_t source = make_frame(71u, 2u, 123456u, FS_CHANNEL_B, 8u, 0xA0u);
    fr_frame_t decoded;
    uint8_t encoded[FR_ENCODED_RECORD_MAX];
    const size_t length = fr_encode_record(&source, encoded, sizeof(encoded));

    if (length == 0u) {
        return false;
    }
    encoded[15u] ^= 0x01u;
    fr_decoder_init(&decoder);
    return fr_decode_record(&decoder, encoded, length, &decoded) == FR_DECODE_BAD_FRAME_CRC;
}

static bool test_bad_input(void)
{
    fr_decoder_t decoder;
    fr_frame_t frame;
    const uint8_t short_record[4] = { 0u, 0u, 0u, 0u };
    fr_decoder_init(&decoder);
    return fr_decode_record(&decoder, short_record, sizeof(short_record), &frame) == FR_DECODE_NEED_MORE &&
           fr_decode_record(NULL, short_record, sizeof(short_record), &frame) == FR_DECODE_BAD_ARGUMENT;
}

static bool test_detector_learning(void)
{
    fs_config_t config = fs_default_config();
    detector_t detector;
    detector_event_t event;
    fr_frame_t frame;
    unsigned observation;

    detector_init(&detector, &config);
    for (observation = 0u; observation < config.minimum_observations; ++observation) {
        frame = make_frame(105u,
                           (uint8_t)observation,
                           (uint64_t)observation * CYCLE_TICKS + 1200u,
                           FS_CHANNEL_A,
                           8u,
                           0x10u);
        if (!detector_process(&detector, &frame, &event)) {
            return false;
        }
    }
    detector_freeze_baseline(&detector, true);
    frame = make_frame(105u, 9u, 9u * CYCLE_TICKS + 1200u, FS_CHANNEL_A, 18u, 0x22u);
    if (!detector_process(&detector, &frame, &event)) {
        return false;
    }
    return event.kind == DETECTOR_EVENT_LENGTH_CHANGE && event.score >= config.anomaly_threshold;
}

static bool test_baseline_export(void)
{
    fs_config_t config = fs_default_config();
    detector_t detector;
    detector_event_t event;
    fr_frame_t frame = make_frame(12u, 1u, 1000u, FS_CHANNEL_A, 4u, 3u);
    char output[512];
    size_t length;

    detector_init(&detector, &config);
    (void)detector_process(&detector, &frame, &event);
    length = detector_export_json(&detector, output, sizeof(output));
    return length > 0u && strstr(output, "\"slot\":12") != NULL && strstr(output, "jayis1") != NULL;
}

static int run_self_test(void)
{
    struct test_case {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "crc-reference", test_crc_reference },
        { "record-round-trip", test_round_trip },
        { "crc-corruption", test_crc_corruption },
        { "bad-input", test_bad_input },
        { "detector-learning", test_detector_learning },
        { "baseline-export", test_baseline_export }
    };
    size_t index;
    unsigned failures = 0u;

    for (index = 0u; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        const bool passed = tests[index].function();
        printf("%-24s %s\n", tests[index].name, passed ? "PASS" : "FAIL");
        if (!passed) {
            failures++;
        }
    }
    printf("self-test: %zu tests, %u failures\n", sizeof(tests) / sizeof(tests[0]), failures);
    return failures == 0u ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void print_event(const detector_event_t *event)
{
    if (event->kind == DETECTOR_EVENT_NONE) {
        return;
    }
    printf("EVENT t=%" PRIu64 " slot=%u cycle=%u channel=%c kind=%s score=%u detail=%s\n",
           event->timestamp_ticks,
           event->slot_id,
           event->cycle,
           event->channel == FS_CHANNEL_A ? 'A' : 'B',
           detector_event_name(event->kind),
           event->score,
           event->detail);
}

static int run_demo(void)
{
    fs_config_t config = fs_default_config();
    detector_t detector;
    detector_event_t event;
    fr_decoder_t decoder;
    uint8_t record[FR_ENCODED_RECORD_MAX];
    char baseline[4096];
    unsigned cycle;

    detector_init(&detector, &config);
    fr_decoder_init(&decoder);
    puts("FlexRay Sentinel deterministic capture demonstration");
    puts("Author: jayis1 | mode: passive schedule learning");

    for (cycle = 0u; cycle < DEMO_FRAME_COUNT; ++cycle) {
        fr_frame_t source = make_frame(44u,
                                       (uint8_t)cycle,
                                       (uint64_t)cycle * CYCLE_TICKS + 3000u,
                                       FS_CHANNEL_A,
                                       16u,
                                       0x20u);
        fr_frame_t decoded;
        size_t record_length;
        if (cycle == 10u) {
            source.timestamp_ticks += 350u;
        }
        if (cycle == 11u) {
            source.payload_length = 24u;
        }
        record_length = fr_encode_record(&source, record, sizeof(record));
        if (record_length == 0u || fr_decode_record(&decoder, record, record_length, &decoded) != FR_DECODE_OK) {
            fputs("capture decoder failure\n", stderr);
            return EXIT_FAILURE;
        }
        if (cycle == 8u) {
            detector_freeze_baseline(&detector, true);
        }
        (void)detector_process(&detector, &decoded, &event);
        print_event(&event);
    }
    if (detector_export_json(&detector, baseline, sizeof(baseline)) != 0u) {
        printf("BASELINE %s\n", baseline);
    }
    printf("frames=%" PRIu64 " valid=%" PRIu64 " events=%u\n",
           decoder.stats.frames_seen, decoder.stats.valid_frames, detector.total_events);
    return EXIT_SUCCESS;
}

static void print_usage(const char *program)
{
    printf("Usage: %s --self-test | --demo | --version\n", program);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "--self-test") == 0) {
        return run_self_test();
    }
    if (strcmp(argv[1], "--demo") == 0) {
        return run_demo();
    }
    if (strcmp(argv[1], "--version") == 0) {
        printf("FlexRay Sentinel %s by jayis1\n", FS_FIRMWARE_VERSION);
        return EXIT_SUCCESS;
    }
    print_usage(argv[0]);
    return EXIT_FAILURE;
}
