/* DALI Sentinel application and hosted validation harness
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "board.h"
#include "drivers/dali_phy.h"
#include "drivers/analyzer.h"
#include "drivers/storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static board_inputs_t simulated_inputs;
static bool simulated_sink[2];
static bool simulated_bypass = true;
static uint8_t storage_image[32][STORAGE_BLOCK_BYTES];
static uint32_t stored_blocks;
static struct timespec boot_time;

void board_init(void)
{
    (void)timespec_get(&boot_time, TIME_UTC);
    memset(&simulated_inputs, 0, sizeof(simulated_inputs));
    simulated_inputs.bus_mv[0] = 16000u;
    simulated_inputs.bus_mv[1] = 16000u;
    simulated_inputs.temperature_c = 25;
    simulated_inputs.bypass_closed_feedback = true;
    simulated_inputs.isolated_power_good[0] = true;
    simulated_inputs.isolated_power_good[1] = true;
    simulated_bypass = true;
}

uint32_t board_millis(void)
{
    struct timespec now;
    (void)timespec_get(&now, TIME_UTC);
    uint64_t seconds = (uint64_t)(now.tv_sec - boot_time.tv_sec) * 1000u;
    int64_t nanos = now.tv_nsec - boot_time.tv_nsec;
    return (uint32_t)(seconds + nanos / 1000000);
}

uint32_t board_capture_ticks(void) { return board_millis() * (CAPTURE_TIMER_HZ / 1000u); }

board_inputs_t board_inputs(void)
{
    simulated_inputs.now_ms = board_millis();
    simulated_inputs.bypass_closed_feedback = simulated_bypass;
    simulated_inputs.sink_ma[0] = simulated_sink[0] ? 2u : 0u;
    simulated_inputs.sink_ma[1] = simulated_sink[1] ? 2u : 0u;
    return simulated_inputs;
}

void board_set_sink(bus_side_t side, bool enabled)
{
    if (side == SIDE_CONTROLLER || side == SIDE_GEAR) simulated_sink[(unsigned)side] = enabled;
}

void board_set_bypass(bool closed)
{
    simulated_bypass = closed;
    simulated_inputs.bypass_closed_feedback = closed;
}

void board_set_status(uint8_t red, uint8_t green, uint8_t blue)
{
    (void)red; (void)green; (void)blue;
}

void board_watchdog_kick(void) { }

void board_log(const char *message)
{
    if (message != NULL) fprintf(stderr, "board: %s\n", message);
}

static bool image_writer(uint32_t block, const uint8_t data[512])
{
    if (block >= 32u) return false;
    memcpy(storage_image[block], data, 512u);
    if (stored_blocks <= block) stored_blocks = block + 1u;
    return true;
}

static void inject_manchester(bus_side_t side, uint32_t raw, uint8_t bits, uint32_t start)
{
    bool level = true;
    uint32_t tick = start;
    dali_phy_capture_edge(side, false, tick, 2500u);
    level = false;
    tick += DALI_HALF_BIT_TICKS;
    dali_phy_capture_edge(side, true, tick, 16000u);
    level = true;
    for (uint8_t i = 0; i < bits; ++i) {
        bool bit = ((raw >> (bits - 1u - i)) & 1u) != 0u;
        bool first = !bit;
        bool second = bit;
        if (first != level) {
            dali_phy_capture_edge(side, first, tick, first ? 16000u : 2500u);
            level = first;
        }
        tick += DALI_HALF_BIT_TICKS;
        if (second != level) {
            dali_phy_capture_edge(side, second, tick, second ? 16000u : 2500u);
            level = second;
        }
        tick += DALI_HALF_BIT_TICKS;
    }
    if (!level) dali_phy_capture_edge(side, true, tick, 16000u);
    dali_phy_poll(tick + DALI_FULL_MAX_TICKS * 4u);
}

static bool integration_test(void)
{
    dali_phy_init();
    analyzer_init(60u, 120u);
    storage_init(image_writer);
    inject_manchester(SIDE_CONTROLLER, 0xFE00u, 16u, 10000u);
    dali_frame_t frame;
    unsigned frames_seen = 0u;
    while (dali_phy_next_frame(&frame) == PHY_OK) {
        frames_seen++;
        analyzer_consume(&frame, 1000u, 30u);
        if (!storage_append_frame(&frame)) return false;
    }
    finding_t finding;
    unsigned findings_seen = 0u;
    while (analyzer_next_finding(&finding)) {
        findings_seen++;
        if (!storage_append_finding(&finding)) return false;
    }
    if (!storage_flush()) return false;
    return frames_seen >= 1u && findings_seen >= 1u && stored_blocks >= 1u;
}

static int run_self_test(void)
{
    bool phy = dali_phy_run_self_test();
    bool analyzer = analyzer_run_self_test();
    bool storage = storage_run_self_test();
    bool integration = integration_test();
    printf("DALI Sentinel %s self-test\n", DALI_SENTINEL_VERSION);
    printf("  phy safety/queue: %s\n", phy ? "PASS" : "FAIL");
    printf("  analyzer rules:   %s\n", analyzer ? "PASS" : "FAIL");
    printf("  storage CRC/block:%s\n", storage ? "PASS" : "FAIL");
    printf("  integration path: %s\n", integration ? "PASS" : "FAIL");
    return phy && analyzer && storage && integration ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void print_frame(const dali_frame_t *frame)
{
    printf("frame side=%u bits=%u raw=0x%06X quality=%u collision=%u valid=%s\n",
           (unsigned)frame->side, frame->bit_count, (unsigned)frame->raw,
           frame->quality, frame->collision_score, frame->framing_ok ? "yes" : "no");
}

static int run_demo(void)
{
    dali_phy_init();
    analyzer_init(120u, 180u);
    storage_init(image_writer);
    const uint32_t samples[] = { 0xFF05u, 0x0190u, 0xA500u, 0xA700u, 0xB112u };
    uint32_t tick = 10000u;
    for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); ++i) {
        inject_manchester(SIDE_CONTROLLER, samples[i], 16u, tick);
        tick += 50000u;
    }
    dali_frame_t frame;
    while (dali_phy_next_frame(&frame) == PHY_OK) {
        print_frame(&frame);
        analyzer_consume(&frame, frame.start_tick / 2000u, 30u);
        (void)storage_append_frame(&frame);
    }
    finding_t finding;
    while (analyzer_next_finding(&finding)) {
        printf("finding severity=%u code=%s raw=0x%06X\n", (unsigned)finding.severity,
               analyzer_finding_name(finding.code), (unsigned)finding.evidence_raw);
        (void)storage_append_finding(&finding);
    }
    if (!storage_flush()) return EXIT_FAILURE;
    analyzer_stats_t a = analyzer_stats();
    storage_status_t s = storage_status();
    printf("summary frames=%u broadcasts=%u commissioning=%u evidence_blocks=%u\n",
           (unsigned)a.frames_total, (unsigned)a.broadcasts,
           (unsigned)a.commissioning_commands, (unsigned)s.blocks_written);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    board_init();
    if (argc == 2 && strcmp(argv[1], "--self-test") == 0) return run_self_test();
    if (argc == 2 && strcmp(argv[1], "--demo") == 0) return run_demo();
    fprintf(stderr, "Usage: %s --self-test | --demo\n", argv[0]);
    return EXIT_FAILURE;
}
