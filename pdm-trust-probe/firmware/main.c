/* PDM Trust Probe reference firmware and host self-test
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "board.h"
#include "registers.h"
#include "drivers/analyzer.h"
#include "drivers/diagnostics.h"
#include "drivers/pdm_capture.h"
#include "drivers/protocol.h"
#include <stdio.h>
#include <string.h>

static uint32_t now_ms;
static uint64_t ticks;
static bool gpio[16];
static uint32_t fpga_registers[16];

uint32_t board_millis(void)
{
    return now_ms;
}

uint64_t board_ticks(void)
{
    return ticks;
}

bool board_gpio_read(unsigned pin)
{
    return pin < 16u ? gpio[pin] : false;
}

void board_gpio_write(unsigned pin, bool value)
{
    if (pin < 16u) {
        gpio[pin] = value;
    }
}

void board_usb_write(const uint8_t *data, size_t length)
{
    size_t index;

    for (index = 0u; index < length; ++index) {
        (void)data[index];
    }
}

bool board_run_fixed_pattern(uint8_t pattern,
                             uint8_t channel_mask,
                             uint32_t duration_ms)
{
    if (pattern > 3u || channel_mask == 0u ||
        duration_ms == 0u || duration_ms > PTP_MAX_INJECT_MS ||
        !board_gpio_read(PIN_ARM)) {
        return false;
    }
    fpga_registers[REG_CHANNEL_MASK / 4u] = channel_mask;
    fpga_registers[REG_INJECT_PATTERN / 4u] = pattern;
    fpga_registers[REG_GATE_LIMIT / 4u] = duration_ms;
    fpga_registers[REG_CONTROL / 4u] = CTRL_ACTIVE_PATH |
                                       CTRL_INJECT_ENABLE;
    board_gpio_write(PIN_RELAY, true);
    now_ms += duration_ms;
    board_gpio_write(PIN_RELAY, false);
    fpga_registers[REG_CONTROL / 4u] = CTRL_CAPTURE_ENABLE;
    return true;
}

void board_watchdog_kick(void)
{
}

void board_init(void)
{
    memset(gpio, 0, sizeof(gpio));
    memset(fpga_registers, 0, sizeof(fpga_registers));
    board_gpio_write(PIN_BYPASS, true);
    board_gpio_write(PIN_GREEN_LED, true);
}

static int expect(bool condition, const char *label)
{
    if (!condition) {
        (void)fprintf(stderr, "FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static void put_u32(uint8_t *pointer, uint32_t value)
{
    pointer[0] = (uint8_t)value;
    pointer[1] = (uint8_t)(value >> 8u);
    pointer[2] = (uint8_t)(value >> 16u);
    pointer[3] = (uint8_t)(value >> 24u);
}

static void make_samples(uint8_t *samples,
                         size_t length,
                         unsigned phase,
                         bool stuck_fourth)
{
    size_t index;

    for (index = 0u; index < length; ++index) {
        uint8_t value = 0u;
        value |= (uint8_t)(((index + phase) & 1u) << 0u);
        value |= (uint8_t)((((index / 3u) + phase) & 1u) << 1u);
        value |= (uint8_t)((((index / 5u) + phase) & 1u) << 2u);
        if (!stuck_fourth) {
            value |= (uint8_t)((((index / 7u) + phase) & 1u) << 3u);
        }
        samples[index] = value;
    }
}

static int test_capture_and_analyzer(void)
{
    ptp_capture_t capture;
    ptp_analyzer_t analyzer;
    ptp_window_t window;
    ptp_event_t event;
    uint8_t samples[256];
    bool found_stuck = false;
    int failures = 0;

    capture_init(&capture);
    analyzer_init(&analyzer);
    failures += expect(capture_configure(&capture, 3072000u),
                       "valid clock");
    failures += expect(!capture_configure(&capture, 100u),
                       "reject clock");
    capture_start(&capture);
    make_samples(samples, sizeof(samples), 0u, true);
    failures += expect(capture_push_bits(&capture,
                                         samples,
                                         sizeof(samples),
                                         0x0Fu,
                                         100u),
                       "capture window");
    failures += expect(capture_next(&capture, &window),
                       "read window");
    failures += expect(window.ones[3] == 0u,
                       "stuck channel measured");
    analyzer_consume(&analyzer, &window, capture.clock_hz);
    while (analyzer_next_event(&analyzer, &event)) {
        if (event.type == EVT_STUCK && event.channel == 3u) {
            found_stuck = true;
        }
    }
    failures += expect(found_stuck, "stuck event");
    return failures;
}

static int test_calibration(void)
{
    ptp_calibration_t calibration;
    ptp_capture_t capture;
    ptp_window_t window;
    ptp_policy_t policy;
    ptp_health_report_t health;
    uint8_t samples[256];
    unsigned pass;
    int failures = 0;

    capture_init(&capture);
    capture_start(&capture);
    diagnostics_init(&calibration, 0x0Fu);
    for (pass = 0u; pass < 12u; ++pass) {
        make_samples(samples, sizeof(samples), pass, false);
        failures += expect(capture_push_bits(&capture,
                                             samples,
                                             sizeof(samples),
                                             0x0Fu,
                                             pass * 1000u),
                           "calibration capture");
        failures += expect(capture_next(&capture, &window),
                           "calibration dequeue");
        failures += expect(diagnostics_observe(&calibration,
                                               &window,
                                               3072000u),
                           "calibration observe");
    }
    failures += expect(diagnostics_policy(&calibration, &policy),
                       "calibration policy");
    failures += expect(policy.clock_min_hz < 3072000u,
                       "clock lower margin");
    failures += expect(policy.clock_max_hz > 3072000u,
                       "clock upper margin");
    health = diagnostics_health(&calibration, 2u);
    failures += expect(health.level == HEALTH_GOOD,
                       "healthy channel");
    failures += expect(diagnostics_fingerprint(&calibration) != 0u,
                       "nonzero fingerprint");
    return failures;
}

static int test_protocol(void)
{
    ptp_capture_t capture;
    ptp_analyzer_t analyzer;
    ptp_session_t session;
    ptp_frame_t frame;
    int failures = 0;

    capture_init(&capture);
    analyzer_init(&analyzer);
    protocol_init(&session);
    memset(&frame, 0, sizeof(frame));
    frame.magic = PTP_USB_MAGIC;
    frame.version = PTP_PROTOCOL_VERSION;
    frame.sequence = 1u;
    frame.command = CMD_PATTERN_RUN;
    frame.length = 6u;
    frame.payload[0] = 0u;
    frame.payload[1] = 1u;
    put_u32(frame.payload + 2u, 10u);
    failures += expect(protocol_process(&session,
                                        &capture,
                                        &analyzer,
                                        &frame,
                                        now_ms) == ST_DENIED,
                       "pattern denied");

    gpio[PIN_ARM] = true;
    frame.sequence = 2u;
    frame.command = CMD_ARM;
    frame.length = 8u;
    put_u32(frame.payload, session.nonce);
    put_u32(frame.payload + 4u, protocol_response(session.nonce));
    failures += expect(protocol_process(&session,
                                        &capture,
                                        &analyzer,
                                        &frame,
                                        now_ms) == ST_OK,
                       "arm accepted");

    frame.sequence = 3u;
    frame.command = CMD_PATTERN_RUN;
    frame.length = 6u;
    frame.payload[0] = 1u;
    frame.payload[1] = 1u;
    put_u32(frame.payload + 2u, PTP_MAX_INJECT_MS + 1u);
    failures += expect(protocol_process(&session,
                                        &capture,
                                        &analyzer,
                                        &frame,
                                        now_ms) == ST_RANGE,
                       "duration bound");

    frame.sequence = 4u;
    put_u32(frame.payload + 2u, 25u);
    failures += expect(protocol_process(&session,
                                        &capture,
                                        &analyzer,
                                        &frame,
                                        now_ms) == ST_OK,
                       "bounded pattern");
    failures += expect(!gpio[PIN_RELAY], "pattern returns to bypass");
    failures += expect(protocol_process(&session,
                                        &capture,
                                        &analyzer,
                                        &frame,
                                        now_ms) == ST_BAD_FRAME,
                       "replay rejected");
    now_ms = PTP_ARM_LEASE_MS + 1u;
    frame.sequence = 5u;
    failures += expect(protocol_process(&session,
                                        &capture,
                                        &analyzer,
                                        &frame,
                                        now_ms) == ST_DENIED,
                       "expired lease");
    session.last_sequence = UINT32_MAX;
    frame.sequence = 0u;
    failures += expect(protocol_process(&session,
                                        &capture,
                                        &analyzer,
                                        &frame,
                                        now_ms) == ST_BAD_FRAME,
                       "sequence wrap rejected");
    return failures;
}

static int self_test(void)
{
    int failures = 0;

    board_init();
    failures += test_capture_and_analyzer();
    failures += test_calibration();
    failures += test_protocol();
    (void)printf("PDM Trust Probe self-test: %s (%d failures)\n",
                 failures == 0 ? "PASS" : "FAIL",
                 failures);
    return failures == 0 ? 0 : 1;
}

static void service(ptp_capture_t *capture,
                    ptp_analyzer_t *analyzer)
{
    ptp_window_t window;
    ptp_event_t event;
    uint8_t packet[16];

    while (capture_next(capture, &window)) {
        analyzer_consume(analyzer, &window, capture->clock_hz);
    }
    while (analyzer_next_event(analyzer, &event)) {
        memset(packet, 0, sizeof(packet));
        packet[0] = (uint8_t)event.timestamp;
        packet[1] = (uint8_t)(event.timestamp >> 8u);
        packet[2] = (uint8_t)(event.timestamp >> 16u);
        packet[3] = (uint8_t)(event.timestamp >> 24u);
        packet[4] = (uint8_t)(event.timestamp >> 32u);
        packet[5] = (uint8_t)(event.timestamp >> 40u);
        packet[6] = (uint8_t)(event.timestamp >> 48u);
        packet[7] = (uint8_t)(event.timestamp >> 56u);
        packet[8] = (uint8_t)event.value;
        packet[9] = (uint8_t)(event.value >> 8u);
        packet[10] = (uint8_t)(event.value >> 16u);
        packet[11] = (uint8_t)(event.value >> 24u);
        packet[12] = (uint8_t)event.detail;
        packet[13] = (uint8_t)(event.detail >> 8u);
        packet[14] = event.type;
        packet[15] = event.channel;
        board_usb_write(packet, sizeof(packet));
    }
    board_watchdog_kick();
}

int main(int argc, char **argv)
{
    ptp_capture_t capture;
    ptp_analyzer_t analyzer;
    ptp_session_t session;

    if (argc == 2 && strcmp(argv[1], "--self-test") == 0) {
        return self_test();
    }
    board_init();
    capture_init(&capture);
    analyzer_init(&analyzer);
    protocol_init(&session);
    capture_start(&capture);
    for (now_ms = 0u; now_ms < 100u; ++now_ms) {
        ticks += 3072u;
        service(&capture, &analyzer);
    }
    (void)printf("PDM Trust Probe host simulation complete; author jayis1\n");
    return 0;
}
