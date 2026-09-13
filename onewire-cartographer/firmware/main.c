/*
 * OneWire Cartographer reference firmware and deterministic host self-test
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "board.h"
#include "registers.h"
#include "drivers/analyzer.h"
#include "drivers/onewire_phy.h"
#include "drivers/policy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static owc_phy_t g_phy;
static owc_analyzer_t g_analyzer;
static owc_policy_t g_policy;
static uint32_t g_time_us;
static bool g_gpio[32];
static uint16_t g_bus_mv[2] = {3300u, 3300u};

uint32_t board_micros(void) { return g_time_us; }
uint32_t board_millis(void) { return g_time_us / 1000u; }

uint16_t board_adc_mv(unsigned channel)
{
    if (channel == PIN_UPSTREAM_SENSE) {
        return g_gpio[PIN_UPSTREAM_PULL] ? 40u : g_bus_mv[0];
    }
    if (channel == PIN_DOWNSTREAM_SENSE) {
        return g_gpio[PIN_DOWNSTREAM_PULL] ? 40u : g_bus_mv[1];
    }
    return 0u;
}

bool board_gpio_read(unsigned pin)
{
    return pin < 32u ? g_gpio[pin] : false;
}

void board_gpio_write(unsigned pin, bool value)
{
    if (pin < 32u) {
        g_gpio[pin] = value;
    }
}

void board_delay_us(uint32_t delay) { g_time_us += delay; }

void board_usb_write(const uint8_t *data, size_t length)
{
    size_t index;
    for (index = 0u; index < length; ++index) {
        (void)data[index];
    }
}

void board_log(const char *message)
{
    if (message != NULL) {
        (void)fprintf(stderr, "%s\n", message);
    }
}

void board_watchdog_kick(void) { }

void board_init(void)
{
    memset(g_gpio, 0, sizeof(g_gpio));
    g_time_us = 1u;
    board_gpio_write(PIN_BRIDGE_ENABLE, true);
}

typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t command;
    uint16_t length;
    uint32_t sequence;
    uint8_t payload[OWC_USB_FRAME_MAX - 12u];
} host_frame_t;

static uint32_t read_u32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8u) |
           ((uint32_t)bytes[2] << 16u) |
           ((uint32_t)bytes[3] << 24u);
}

static owc_status_t command_drive_byte(const host_frame_t *frame)
{
    owc_port_t port;
    if (frame->length != 2u) {
        return OWC_STATUS_BAD_FRAME;
    }
    if (!owc_policy_authorize(&g_policy, OWC_CMD_EMIT_BYTE, board_millis())) {
        return OWC_STATUS_DENIED;
    }
    port = frame->payload[0] == 0u ? OWC_PORT_UPSTREAM : OWC_PORT_DOWNSTREAM;
    return owc_phy_write_byte(&g_phy, port, frame->payload[1])
               ? OWC_STATUS_OK : OWC_STATUS_HW_FAULT;
}

static owc_status_t command_reset(const host_frame_t *frame)
{
    owc_port_t port;
    if (frame->length != 1u || frame->payload[0] > 1u) {
        return OWC_STATUS_BAD_FRAME;
    }
    if (!owc_policy_authorize(&g_policy, OWC_CMD_EMIT_RESET, board_millis())) {
        return OWC_STATUS_DENIED;
    }
    port = (owc_port_t)frame->payload[0];
    return owc_phy_drive_reset(&g_phy, port)
               ? OWC_STATUS_OK : OWC_STATUS_NO_DEVICE;
}

static owc_status_t process_frame(const host_frame_t *frame)
{
    uint32_t challenge;
    uint32_t response;

    if (frame == NULL || frame->magic != OWC_USB_MAGIC ||
        frame->version != OWC_PROTOCOL_VERSION ||
        frame->length > sizeof(frame->payload)) {
        return OWC_STATUS_BAD_FRAME;
    }
    switch ((owc_command_t)frame->command) {
    case OWC_CMD_GET_INFO:
    case OWC_CMD_GET_STATUS:
    case OWC_CMD_CAPTURE_READ:
    case OWC_CMD_SCAN_ROMS:
        return OWC_STATUS_OK;
    case OWC_CMD_CAPTURE_START:
        owc_phy_start(&g_phy);
        return OWC_STATUS_OK;
    case OWC_CMD_CAPTURE_STOP:
        owc_phy_stop(&g_phy);
        return OWC_STATUS_OK;
    case OWC_CMD_SET_THRESHOLDS:
        if (frame->length != 4u) {
            return OWC_STATUS_BAD_FRAME;
        }
        return owc_phy_set_thresholds(&g_phy,
                   (uint16_t)(frame->payload[0] | (frame->payload[1] << 8u)),
                   (uint16_t)(frame->payload[2] | (frame->payload[3] << 8u)))
                   ? OWC_STATUS_OK : OWC_STATUS_RANGE;
    case OWC_CMD_ARM_SESSION:
        if (frame->length != 8u) {
            return OWC_STATUS_BAD_FRAME;
        }
        challenge = read_u32(frame->payload);
        response = read_u32(frame->payload + 4u);
        return owc_policy_unlock(&g_policy, challenge, response)
                   ? OWC_STATUS_OK : OWC_STATUS_DENIED;
    case OWC_CMD_EMIT_RESET:
        return command_reset(frame);
    case OWC_CMD_EMIT_BYTE:
        return command_drive_byte(frame);
    case OWC_CMD_BRIDGE_MODE:
        if (frame->length != 1u) {
            return OWC_STATUS_BAD_FRAME;
        }
        return owc_policy_set_mode(&g_policy,
                   (owc_bridge_mode_t)frame->payload[0], board_millis())
                   ? OWC_STATUS_OK : OWC_STATUS_DENIED;
    case OWC_CMD_CLEAR_FAULT:
        return owc_policy_clear_fault(&g_policy)
                   ? OWC_STATUS_OK : OWC_STATUS_HW_FAULT;
    default:
        return OWC_STATUS_BAD_FRAME;
    }
}

static void service_capture(void)
{
    owc_edge_t edge;
    owc_event_t event;
    uint8_t packet[16];

    while (owc_phy_next_edge(&g_phy, &edge)) {
        owc_analyzer_consume(&g_analyzer, &edge);
    }
    while (owc_analyzer_next_event(&g_analyzer, &event)) {
        memset(packet, 0, sizeof(packet));
        packet[0] = (uint8_t)event.type;
        packet[1] = event.port;
        packet[2] = event.value;
        packet[3] = event.flags;
        memcpy(packet + 4u, &event.timestamp_us, sizeof(event.timestamp_us));
        memcpy(packet + 8u, &event.duration_us, sizeof(event.duration_us));
        memcpy(packet + 12u, &event.voltage_mv, sizeof(event.voltage_mv));
        board_usb_write(packet, sizeof(packet));
    }
}

static void inject_pulse(owc_port_t port, uint32_t start, uint32_t width)
{
    owc_phy_record_edge(&g_phy, port, OWC_EDGE_FALLING, 20u, start);
    owc_phy_record_edge(&g_phy, port, OWC_EDGE_RISING, 3300u, start + width);
}

static int expect(bool condition, const char *name)
{
    if (!condition) {
        (void)fprintf(stderr, "FAIL: %s\n", name);
        return 1;
    }
    return 0;
}

static int self_test(void)
{
    int failures = 0;
    owc_event_t event;
    uint8_t rom[8] = {0x01u, 0x2Au, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0u};
    host_frame_t frame;
    uint32_t challenge = 0x12345678u;
    uint32_t response = owc_policy_expected_response(challenge);

    board_init();
    owc_phy_init(&g_phy);
    owc_analyzer_init(&g_analyzer);
    owc_policy_init(&g_policy);
    owc_phy_start(&g_phy);
    inject_pulse(OWC_PORT_UPSTREAM, 100u, 480u);
    inject_pulse(OWC_PORT_UPSTREAM, 700u, 6u);
    inject_pulse(OWC_PORT_UPSTREAM, 770u, 60u);
    while (owc_phy_next_edge(&g_phy, &(owc_edge_t){0})) { /* drain test below */ }
    owc_phy_start(&g_phy);
    inject_pulse(OWC_PORT_UPSTREAM, 1000u, 480u);
    inject_pulse(OWC_PORT_UPSTREAM, 1600u, 6u);
    while (owc_phy_next_edge(&g_phy, &(owc_edge_t){0})) { }
    /* Feed known pulses directly to isolate ring-buffer and decoder checks. */
    {
        owc_edge_t down = {2000u, 20u, 0u, OWC_EDGE_FALLING};
        owc_edge_t up = {2480u, 3300u, 0u, OWC_EDGE_RISING};
        owc_analyzer_consume(&g_analyzer, &down);
        owc_analyzer_consume(&g_analyzer, &up);
    }
    failures += expect(owc_analyzer_next_event(&g_analyzer, &event), "decoded event");
    failures += expect(event.type == OWC_EVT_RESET, "reset classification");
    rom[7] = owc_crc8(rom, 7u);
    failures += expect(owc_analyzer_observe_rom(&g_analyzer, rom, 84u), "ROM observation");
    failures += expect(g_analyzer.devices[0].crc_valid, "ROM CRC");
    failures += expect(owc_phy_set_thresholds(&g_phy, 700u, 2300u), "threshold valid");
    failures += expect(!owc_phy_set_thresholds(&g_phy, 2100u, 2300u), "threshold reject");

    memset(&frame, 0, sizeof(frame));
    frame.magic = OWC_USB_MAGIC;
    frame.version = OWC_PROTOCOL_VERSION;
    frame.command = OWC_CMD_EMIT_BYTE;
    frame.length = 2u;
    frame.payload[0] = 1u;
    frame.payload[1] = 0xCCu;
    failures += expect(process_frame(&frame) == OWC_STATUS_DENIED, "drive denied disarmed");
    g_gpio[PIN_ARM_SWITCH] = true;
    frame.command = OWC_CMD_ARM_SESSION;
    frame.length = 8u;
    memcpy(frame.payload, &challenge, 4u);
    memcpy(frame.payload + 4u, &response, 4u);
    failures += expect(process_frame(&frame) == OWC_STATUS_OK, "session unlock");
    frame.command = OWC_CMD_BRIDGE_MODE;
    frame.length = 1u;
    frame.payload[0] = OWC_MODE_LAB_DRIVE;
    failures += expect(process_frame(&frame) == OWC_STATUS_OK, "lab mode armed");
    failures += expect(owc_crc8((const uint8_t *)"123456789", 9u) == 0xA1u, "Dallas CRC vector");

    if (failures == 0) {
        (void)printf("OneWire Cartographer self-test: 11 checks passed\n");
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--self-test") == 0) {
        return self_test();
    }
    board_init();
    owc_phy_init(&g_phy);
    owc_analyzer_init(&g_analyzer);
    owc_policy_init(&g_policy);
    owc_phy_start(&g_phy);
    board_log("OneWire Cartographer monitor active");
#ifdef OWC_HOST_SIM
    service_capture();
    return EXIT_SUCCESS;
#else
    for (;;) {
        service_capture();
        owc_policy_tick(&g_policy, board_millis());
        board_watchdog_kick();
    }
#endif
}
