/*
 * SENT Sentinel firmware entry point, simulation, and deterministic tests
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "board.h"
#include "detector.h"
#include "sent.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEMO_FRAME_COUNT 30u
#define DEMO_PERIOD_US 1000u
#define DEMO_TICK_NS 3000u

static sent_frame_t make_frame(ss_channel_t channel,
                               uint32_t sequence,
                               uint64_t timestamp_us,
                               uint16_t value,
                               uint16_t analog_mv)
{
    sent_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.channel = channel;
    frame.sequence = sequence;
    frame.timestamp_us = timestamp_us;
    frame.tick_time_ns = DEMO_TICK_NS;
    frame.analog_mv = analog_mv;
    frame.status = 0u;
    frame.data_length = 6u;
    frame.data[0] = (uint8_t)((value >> 8u) & 0x0Fu);
    frame.data[1] = (uint8_t)((value >> 4u) & 0x0Fu);
    frame.data[2] = (uint8_t)(value & 0x0Fu);
    frame.data[3] = 0xAu;
    frame.data[4] = 0x5u;
    frame.data[5] = (uint8_t)(channel & 0x0Fu);
    frame.fast_value = value;
    return frame;
}

static bool decode_frame(sent_decoder_t *decoder,
                         const sent_frame_t *source,
                         sent_frame_t *decoded)
{
    uint32_t edges[SENT_ENCODED_EDGE_MAX];
    size_t edge_count = sent_encode_edges(source, edges, SENT_ENCODED_EDGE_MAX);
    if (edge_count == 0u) {
        return false;
    }
    return sent_decode_edges(decoder,
                             source->channel,
                             source->sequence,
                             source->timestamp_us,
                             source->analog_mv,
                             edges,
                             edge_count,
                             decoded) == SENT_DECODE_OK;
}

static bool test_crc_reference(void)
{
    const uint8_t nibbles[] = { 0u, 1u, 2u, 3u, 4u, 5u };
    uint8_t first = sent_crc4(nibbles, sizeof(nibbles));
    uint8_t second = sent_crc4(nibbles, sizeof(nibbles));
    return first <= 0x0Fu && first == second;
}

static bool test_round_trip(void)
{
    sent_decoder_t decoder;
    sent_frame_t source = make_frame(SS_CHANNEL_2, 17u, 44100u, 0xA35u, 3188u);
    sent_frame_t decoded;
    sent_decoder_init(&decoder);
    if (!decode_frame(&decoder, &source, &decoded)) {
        return false;
    }
    return decoded.channel == source.channel &&
           decoded.sequence == source.sequence &&
           decoded.timestamp_us == source.timestamp_us &&
           decoded.fast_value == source.fast_value &&
           decoded.analog_mv == source.analog_mv &&
           decoded.data_length == source.data_length;
}

static bool test_bad_sync(void)
{
    sent_decoder_t decoder;
    sent_frame_t source = make_frame(SS_CHANNEL_0, 1u, 1000u, 0x123u, 400u);
    sent_frame_t decoded;
    uint32_t edges[SENT_ENCODED_EDGE_MAX];
    size_t count = sent_encode_edges(&source, edges, SENT_ENCODED_EDGE_MAX);
    sent_decoder_init(&decoder);
    if (count == 0u) {
        return false;
    }
    edges[0] = 1u;
    return sent_decode_edges(&decoder,
                             source.channel,
                             source.sequence,
                             source.timestamp_us,
                             source.analog_mv,
                             edges,
                             count,
                             &decoded) == SENT_DECODE_BAD_SYNC;
}

static bool test_bad_crc(void)
{
    sent_decoder_t decoder;
    sent_frame_t source = make_frame(SS_CHANNEL_1, 2u, 2000u, 0x456u, 1200u);
    sent_frame_t decoded;
    uint32_t edges[SENT_ENCODED_EDGE_MAX];
    size_t count = sent_encode_edges(&source, edges, SENT_ENCODED_EDGE_MAX);
    sent_decoder_init(&decoder);
    if (count == 0u) {
        return false;
    }
    edges[count - 1u] += 3u * ((DEMO_TICK_NS * 170u + 500u) / 1000u);
    return sent_decode_edges(&decoder,
                             source.channel,
                             source.sequence,
                             source.timestamp_us,
                             source.analog_mv,
                             edges,
                             count,
                             &decoded) == SENT_DECODE_BAD_CRC;
}

static bool test_argument_validation(void)
{
    sent_decoder_t decoder;
    sent_frame_t frame;
    const uint32_t edges[] = { 100u, 50u, 60u, 70u };
    sent_decoder_init(&decoder);
    return sent_decode_edges(NULL, SS_CHANNEL_0, 0u, 0u, 0u, edges, 4u, &frame) == SENT_DECODE_BAD_ARGUMENT &&
           sent_decode_edges(&decoder, (ss_channel_t)9, 0u, 0u, 0u, edges, 4u, &frame) == SENT_DECODE_BAD_CHANNEL &&
           sent_decode_edges(&decoder, SS_CHANNEL_0, 0u, 0u, 0u, edges, 2u, &frame) == SENT_DECODE_BAD_LENGTH;
}

static bool learn_channel(detector_t *detector,
                          sent_decoder_t *decoder,
                          ss_channel_t channel,
                          uint16_t base_value,
                          uint16_t base_analog)
{
    unsigned sample;
    detector_event_t event;
    for (sample = 0u; sample < SS_MIN_LEARNING_FRAMES; ++sample) {
        sent_frame_t source = make_frame(channel,
                                         sample,
                                         (uint64_t)(sample + 1u) * DEMO_PERIOD_US,
                                         (uint16_t)(base_value + sample % 4u),
                                         (uint16_t)(base_analog + sample % 3u));
        sent_frame_t decoded;
        if (!decode_frame(decoder, &source, &decoded) ||
            !detector_process(detector, &decoded, &event)) {
            return false;
        }
    }
    return true;
}

static bool test_detector_learning(void)
{
    ss_config_t config = ss_default_config();
    detector_t detector;
    sent_decoder_t decoder;
    detector_event_t event;
    sent_frame_t source;
    sent_frame_t decoded;

    detector_init(&detector, &config);
    sent_decoder_init(&decoder);
    if (!learn_channel(&detector, &decoder, SS_CHANNEL_0, 1000u, 1220u)) {
        return false;
    }
    detector_freeze(&detector, true);
    source = make_frame(SS_CHANNEL_0,
                        SS_MIN_LEARNING_FRAMES,
                        (uint64_t)(SS_MIN_LEARNING_FRAMES + 1u) * DEMO_PERIOD_US,
                        1002u,
                        1221u);
    if (!decode_frame(&decoder, &source, &decoded) ||
        !detector_process(&detector, &decoded, &event)) {
        return false;
    }
    return event.kind == DETECTOR_EVENT_NONE;
}

static bool test_analog_mismatch(void)
{
    ss_config_t config = ss_default_config();
    detector_t detector;
    sent_decoder_t decoder;
    detector_event_t event;
    sent_frame_t source;
    sent_frame_t decoded;

    detector_init(&detector, &config);
    sent_decoder_init(&decoder);
    if (!learn_channel(&detector, &decoder, SS_CHANNEL_1, 1800u, 2200u)) {
        return false;
    }
    detector_freeze(&detector, true);
    source = make_frame(SS_CHANNEL_1,
                        SS_MIN_LEARNING_FRAMES,
                        (uint64_t)(SS_MIN_LEARNING_FRAMES + 1u) * DEMO_PERIOD_US,
                        1801u,
                        4100u);
    if (!decode_frame(&decoder, &source, &decoded) ||
        !detector_process(&detector, &decoded, &event)) {
        return false;
    }
    return event.kind == DETECTOR_EVENT_ANALOG_MISMATCH &&
           (event.flags & DETECTOR_FLAG_ANALOG) != 0u &&
           event.score >= config.score_threshold;
}

static bool test_counter_and_timing(void)
{
    ss_config_t config = ss_default_config();
    detector_t detector;
    sent_decoder_t decoder;
    detector_event_t event;
    sent_frame_t source;
    sent_frame_t decoded;

    detector_init(&detector, &config);
    sent_decoder_init(&decoder);
    if (!learn_channel(&detector, &decoder, SS_CHANNEL_3, 500u, 600u)) {
        return false;
    }
    detector_freeze(&detector, true);
    source = make_frame(SS_CHANNEL_3,
                        99u,
                        (uint64_t)(SS_MIN_LEARNING_FRAMES + 4u) * DEMO_PERIOD_US,
                        900u,
                        600u);
    source.tick_time_ns = 4200u;
    if (!decode_frame(&decoder, &source, &decoded) ||
        !detector_process(&detector, &decoded, &event)) {
        return false;
    }
    return (event.flags & DETECTOR_FLAG_TIMING) != 0u &&
           (event.flags & DETECTOR_FLAG_COUNTER) != 0u;
}

static bool test_baseline_export(void)
{
    detector_t detector;
    sent_decoder_t decoder;
    char output[1024];
    size_t length;

    detector_init(&detector, NULL);
    sent_decoder_init(&decoder);
    if (!learn_channel(&detector, &decoder, SS_CHANNEL_2, 2000u, 2440u)) {
        return false;
    }
    detector_freeze(&detector, true);
    length = detector_export_json(&detector, output, sizeof(output));
    return length > 0u && strstr(output, "\"author\":\"jayis1\"") != NULL &&
           strstr(output, "\"channel\":2") != NULL;
}

static int run_self_test(void)
{
    struct test_case {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "crc-reference", test_crc_reference },
        { "edge-round-trip", test_round_trip },
        { "bad-sync", test_bad_sync },
        { "bad-crc", test_bad_crc },
        { "argument-validation", test_argument_validation },
        { "detector-learning", test_detector_learning },
        { "analog-mismatch", test_analog_mismatch },
        { "counter-and-timing", test_counter_and_timing },
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
    printf("EVENT t=%" PRIu64 "us channel=%u kind=%s score=%u %s\n",
           event->timestamp_us,
           (unsigned)event->channel,
           detector_event_name(event->kind),
           event->score,
           event->detail);
}

static int run_demo(void)
{
    detector_t detector;
    sent_decoder_t decoder;
    detector_event_t event;
    unsigned sample;
    char baseline[2048];

    detector_init(&detector, NULL);
    sent_decoder_init(&decoder);
    puts("SENT Sentinel passive sensor trust demonstration");
    puts("Author: jayis1 | four receive-only SAE J2716 channels");
    for (sample = 0u; sample < DEMO_FRAME_COUNT; ++sample) {
        uint16_t value = (uint16_t)(1000u + sample % 8u);
        uint16_t analog = (uint16_t)(1220u + sample % 5u);
        sent_frame_t source;
        sent_frame_t decoded;
        if (sample == 25u) {
            detector_freeze(&detector, true);
        }
        if (sample == 27u) {
            analog = 4100u;
        }
        if (sample == 28u) {
            value = 3000u;
        }
        source = make_frame(SS_CHANNEL_0,
                            sample,
                            (uint64_t)(sample + 1u) * DEMO_PERIOD_US,
                            value,
                            analog);
        if (sample == 29u) {
            source.tick_time_ns = 3900u;
        }
        if (!decode_frame(&decoder, &source, &decoded)) {
            fputs("decoder rejected generated frame\n", stderr);
            return EXIT_FAILURE;
        }
        if (!detector_process(&detector, &decoded, &event)) {
            fputs("detector rejected decoded frame\n", stderr);
            return EXIT_FAILURE;
        }
        print_event(&event);
    }
    if (detector_export_json(&detector, baseline, sizeof(baseline)) != 0u) {
        printf("BASELINE %s\n", baseline);
    }
    printf("frames=%" PRIu64 " valid=%" PRIu64 " events=%u\n",
           decoder.stats.frames_seen,
           decoder.stats.valid_frames,
           detector.total_events);
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
        printf("SENT Sentinel %s by %s\n", SS_FIRMWARE_VERSION, SS_AUTHOR);
        return EXIT_SUCCESS;
    }
    print_usage(argv[0]);
    return EXIT_FAILURE;
}
