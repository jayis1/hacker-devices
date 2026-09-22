/* RFFE Sentinel host-compilable control-plane reference
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
#include "board.h"
#include "registers.h"
#include "drivers/policy.h"
#include "drivers/protocol.h"
#include "drivers/rffe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct rs_event {
    uint64_t timestamp_us;
    uint32_t sequence;
    uint32_t crc;
    uint16_t register_address;
    uint16_t electrical_flags;
    uint8_t usid;
    uint8_t command_class;
    uint8_t value;
    uint8_t action;
};

struct rs_journal {
    struct rs_event events[RS_EVENT_CAPACITY];
    size_t head;
    size_t count;
    uint32_t next_sequence;
};

struct rs_device {
    enum rs_mode mode;
    uint32_t faults;
    uint64_t lease_expires_us;
    uint64_t last_host_us;
    uint32_t last_nonce;
    struct rs_measurements measurements;
    struct rs_policy policy;
    struct rs_journal journal;
};

static uint64_t sim_time_us;
static uint64_t sim_last_arm_us = UINT64_MAX;
static struct rs_measurements sim_measurements = {
    1800u, 8u, 1785u, 1792u
};
static struct rs_mmio sim_mmio = {
    RS_FPGA_ID_EXPECTED, 0u, RS_FPGA_ST_PLL_LOCK | RS_FPGA_ST_VIO_VALID,
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u
};
static bool sim_active_path;

uint64_t board_time_us(void)
{
    return sim_time_us;
}

bool board_button_recent(uint64_t now_us)
{
    return (sim_last_arm_us != UINT64_MAX) &&
           (now_us >= sim_last_arm_us) &&
           ((now_us - sim_last_arm_us) <= RS_ARM_WINDOW_US);
}

void board_note_arm_press(uint64_t at_us)
{
    sim_last_arm_us = at_us;
}

void board_set_bypass(bool active_path)
{
    sim_active_path = active_path;
}

void board_set_led(enum rs_mode mode)
{
    (void)mode;
}

void board_read_measurements(struct rs_measurements *out)
{
    if (out != NULL) {
        *out = sim_measurements;
    }
}

void board_watchdog_kick(void)
{
    sim_mmio.watchdog = 0x51a7u;
}

void board_sim_set_measurements(uint16_t vio_mv, uint16_t target_ma)
{
    sim_measurements.vio_mv = vio_mv;
    sim_measurements.target_ma = target_ma;
}

void board_sim_advance(uint64_t delta_us)
{
    sim_time_us += delta_us;
}

uint32_t fpga_reg_read(uint32_t offset)
{
    const uint32_t *registers = (const uint32_t *)&sim_mmio;
    size_t index = offset / sizeof(uint32_t);

    if ((offset % sizeof(uint32_t)) != 0u ||
        index >= (sizeof(sim_mmio) / sizeof(uint32_t))) {
        return 0u;
    }
    return registers[index];
}

void fpga_reg_write(uint32_t offset, uint32_t value)
{
    uint32_t *registers = (uint32_t *)&sim_mmio;
    size_t index = offset / sizeof(uint32_t);

    if ((offset % sizeof(uint32_t)) == 0u &&
        index < (sizeof(sim_mmio) / sizeof(uint32_t))) {
        registers[index] = value;
    }
}

void fpga_sim_set_status(uint32_t value)
{
    sim_mmio.status = value;
}

static uint32_t event_crc(const struct rs_event *event)
{
    struct rs_event copy;

    copy = *event;
    copy.crc = 0u;
    return rscp_crc32c((const uint8_t *)&copy, sizeof(copy));
}

static void journal_init(struct rs_journal *journal)
{
    memset(journal, 0, sizeof(*journal));
    journal->next_sequence = 1u;
}

static void journal_append(struct rs_journal *journal,
                           const struct rffe_frame *frame,
                           const struct rs_decision *decision)
{
    struct rs_event *event;

    event = &journal->events[journal->head];
    memset(event, 0, sizeof(*event));
    event->timestamp_us = frame->timestamp_us;
    event->sequence = journal->next_sequence++;
    event->register_address = frame->register_address;
    event->electrical_flags = frame->flags;
    event->usid = frame->usid;
    event->command_class = (uint8_t)frame->command_class;
    event->value = frame->data[0];
    event->action = (uint8_t)decision->action;
    event->crc = event_crc(event);
    journal->head = (journal->head + 1u) % RS_EVENT_CAPACITY;
    if (journal->count < RS_EVENT_CAPACITY) {
        ++journal->count;
    }
}

static bool journal_validate(const struct rs_journal *journal)
{
    size_t index;

    for (index = 0u; index < journal->count; ++index) {
        if (journal->events[index].sequence != 0u &&
            journal->events[index].crc != event_crc(&journal->events[index])) {
            return false;
        }
    }
    return true;
}

static void device_force_bypass(struct rs_device *device, uint32_t fault)
{
    device->faults |= fault;
    device->mode = fault == RS_FAULT_NONE ?
                   RS_MODE_SAFE_BYPASS : RS_MODE_FAULT_LATCHED;
    device->lease_expires_us = 0u;
    fpga_reg_write(RS_FPGA_REG_CONTROL, 0u);
    board_set_bypass(false);
    board_set_led(device->mode);
}

static void device_init(struct rs_device *device)
{
    memset(device, 0, sizeof(*device));
    policy_init(&device->policy);
    journal_init(&device->journal);
    device->mode = RS_MODE_SAFE_BYPASS;
    device->last_host_us = board_time_us();
    board_set_bypass(false);
    board_set_led(device->mode);
}

static bool device_set_passive(struct rs_device *device)
{
    uint32_t status = fpga_reg_read(RS_FPGA_REG_STATUS);

    if (device->faults != RS_FAULT_NONE ||
        fpga_reg_read(RS_FPGA_REG_ID) != RS_FPGA_ID_EXPECTED ||
        (status & RS_FPGA_ST_PLL_LOCK) == 0u) {
        return false;
    }
    device->mode = RS_MODE_PASSIVE_CAPTURE;
    fpga_reg_write(RS_FPGA_REG_CONTROL, RS_FPGA_CTL_CAPTURE);
    board_set_bypass(false);
    board_set_led(device->mode);
    return true;
}

static bool device_arm(struct rs_device *device, uint32_t nonce,
                       uint64_t requested_lease_us)
{
    uint64_t now = board_time_us();

    if (device->mode != RS_MODE_PASSIVE_CAPTURE ||
        device->faults != RS_FAULT_NONE ||
        nonce <= device->last_nonce ||
        !board_button_recent(now) ||
        requested_lease_us == 0u ||
        requested_lease_us > RS_MAX_LEASE_US) {
        return false;
    }
    device->last_nonce = nonce;
    device->lease_expires_us = now + requested_lease_us;
    device->mode = RS_MODE_ARMED_FORWARD;
    fpga_reg_write(RS_FPGA_REG_CONTROL,
                   RS_FPGA_CTL_CAPTURE | RS_FPGA_CTL_FORWARD |
                   RS_FPGA_CTL_INTERVENE);
    board_set_bypass(true);
    board_set_led(device->mode);
    return true;
}

static bool device_clear_fault(struct rs_device *device)
{
    uint32_t status = fpga_reg_read(RS_FPGA_REG_STATUS);

    board_read_measurements(&device->measurements);
    if ((status & (RS_FPGA_ST_FIFO_OVERFLOW |
                   RS_FPGA_ST_CONTENTION |
                   RS_FPGA_ST_WATCHDOG)) != 0u) {
        return false;
    }
    if (device->measurements.vio_mv < RS_VIO_MIN_MV ||
        device->measurements.vio_mv > RS_VIO_MAX_MV ||
        device->measurements.target_ma > RS_OVERCURRENT_MA) {
        return false;
    }
    device->faults = RS_FAULT_NONE;
    device_force_bypass(device, RS_FAULT_NONE);
    return true;
}

static void device_poll(struct rs_device *device)
{
    uint64_t now = board_time_us();
    uint32_t status = fpga_reg_read(RS_FPGA_REG_STATUS);

    board_read_measurements(&device->measurements);
    if (device->measurements.vio_mv < RS_VIO_MIN_MV ||
        device->measurements.vio_mv > RS_VIO_MAX_MV) {
        device_force_bypass(device, RS_FAULT_VIO_RANGE);
        return;
    }
    if (device->measurements.target_ma > RS_OVERCURRENT_MA) {
        device_force_bypass(device, RS_FAULT_OVERCURRENT);
        return;
    }
    if ((status & RS_FPGA_ST_FIFO_OVERFLOW) != 0u) {
        device_force_bypass(device, RS_FAULT_FIFO_OVERFLOW);
        return;
    }
    if ((status & RS_FPGA_ST_CONTENTION) != 0u) {
        device_force_bypass(device, RS_FAULT_PROTOCOL);
        return;
    }
    if ((status & RS_FPGA_ST_WATCHDOG) != 0u) {
        device_force_bypass(device, RS_FAULT_FPGA_WATCHDOG);
        return;
    }
    if (device->mode == RS_MODE_ARMED_FORWARD &&
        now >= device->lease_expires_us) {
        device_force_bypass(device, RS_FAULT_NONE);
        return;
    }
    if (device->mode == RS_MODE_ARMED_FORWARD &&
        (now - device->last_host_us) > RS_HOST_TIMEOUT_US) {
        device_force_bypass(device, RS_FAULT_HOST_TIMEOUT);
        return;
    }
    board_watchdog_kick();
}

static struct rs_decision device_process_frame(struct rs_device *device,
                                               const struct rffe_frame *frame)
{
    bool authorized;
    struct rs_decision decision;

    authorized = device->mode == RS_MODE_ARMED_FORWARD &&
                 board_time_us() < device->lease_expires_us;
    decision = policy_evaluate(&device->policy, frame, authorized);
    journal_append(&device->journal, frame, &decision);
    return decision;
}

static uint64_t make_test_word(uint8_t usid, uint8_t command,
                               uint8_t reg, uint8_t value)
{
    uint64_t body = ((uint64_t)(usid & 0x0fu) << 20u) |
                    ((uint64_t)(command & 0x0fu) << 16u) |
                    ((uint64_t)reg << 8u) | value;
    return (body << 1u) | rffe_odd_parity(body, 24u);
}

static int expect_true(bool condition, const char *name)
{
    if (!condition) {
        (void)fprintf(stderr, "self-test failed: %s\n", name);
        return 1;
    }
    return 0;
}

static int test_protocol(void)
{
    struct rscp_message input;
    struct rscp_message output;
    uint8_t wire[RSCP_HEADER_SIZE + RSCP_MAX_PAYLOAD];
    size_t encoded;
    int failures = 0;

    memset(&input, 0, sizeof(input));
    input.version = RSCP_VERSION;
    input.type = RSCP_GET_STATUS;
    input.sequence = 77u;
    input.nonce = 101u;
    input.payload_length = 3u;
    input.payload[0] = 0xa5u;
    input.payload[1] = 0x5au;
    input.payload[2] = 0x11u;
    encoded = rscp_encode(&input, wire, sizeof(wire));
    failures += expect_true(encoded == RSCP_HEADER_SIZE + 3u,
                            "protocol encode length");
    failures += expect_true(rscp_decode(wire, encoded, &output),
                            "protocol decode");
    failures += expect_true(output.sequence == input.sequence,
                            "protocol sequence");
    failures += expect_true(output.payload[1] == 0x5au,
                            "protocol payload");
    wire[RSCP_HEADER_SIZE] ^= 1u;
    failures += expect_true(!rscp_decode(wire, encoded, &output),
                            "protocol corrupt rejection");
    failures += expect_true(rscp_is_mutating(RSCP_ARM_LEASE),
                            "mutating classification");
    failures += expect_true(!rscp_is_mutating(RSCP_GET_STATUS),
                            "query classification");
    return failures;
}

static int test_decoder_and_policy(void)
{
    struct rffe_frame frame;
    struct rs_policy policy;
    struct rs_rule rule;
    struct rs_decision decision;
    uint64_t raw;
    int failures = 0;

    raw = make_test_word(3u, 0u, 0x22u, 0xf3u);
    failures += expect_true(rffe_decode_word(raw, 25u, 100u, 1u, &frame),
                            "frame decode");
    failures += expect_true(frame.usid == 3u &&
                            frame.register_address == 0x22u,
                            "frame fields");
    failures += expect_true((frame.flags & RS_RFFE_FLAG_PARITY_OK) != 0u,
                            "frame parity");

    policy_init(&policy);
    memset(&rule, 0, sizeof(rule));
    rule.priority = 10u;
    rule.usid_mask = 0x0fu;
    rule.usid_value = 3u;
    rule.command_mask = (uint8_t)(1u << RFFE_CMD_REG_WRITE);
    rule.register_first = 0x20u;
    rule.register_last = 0x2fu;
    rule.value_mask = 0xf0u;
    rule.value_match = 0xf0u;
    rule.substitute_mask = 0x0fu;
    rule.substitute_value = 0x05u;
    rule.action = RS_ACTION_SUBSTITUTE;
    failures += expect_true(policy_add_rule(&policy, &rule), "policy add");
    decision = policy_evaluate(&policy, &frame, false);
    failures += expect_true(decision.action == RS_ACTION_ALERT,
                            "unauthorized downgrade");
    decision = policy_evaluate(&policy, &frame, true);
    failures += expect_true(decision.action == RS_ACTION_SUBSTITUTE,
                            "authorized substitute");
    failures += expect_true(decision.output_value == 0xf5u,
                            "substitute mask");
    rule.command_mask = (uint8_t)(1u << RFFE_CMD_REG_READ);
    failures += expect_true(!policy_validate_rule(&rule),
                            "active read rule rejected");
    return failures;
}

static int test_state_machine(void)
{
    struct rs_device device;
    struct rffe_frame frame;
    struct rs_rule rule;
    struct rs_decision decision;
    int failures = 0;

    sim_time_us = 100u;
    sim_last_arm_us = UINT64_MAX;
    sim_active_path = false;
    sim_mmio.id = RS_FPGA_ID_EXPECTED;
    sim_mmio.status = RS_FPGA_ST_PLL_LOCK | RS_FPGA_ST_VIO_VALID;
    board_sim_set_measurements(1800u, 8u);
    device_init(&device);
    failures += expect_true(device.mode == RS_MODE_SAFE_BYPASS,
                            "boot bypass");
    failures += expect_true(device_set_passive(&device), "passive entry");
    failures += expect_true(!sim_active_path, "passive bypass path");
    failures += expect_true(!device_arm(&device, 1u, 10000000u),
                            "arm needs button");
    board_note_arm_press(board_time_us());
    failures += expect_true(device_arm(&device, 1u, 10000000u),
                            "armed entry");
    failures += expect_true(sim_active_path, "active path selected");

    memset(&rule, 0, sizeof(rule));
    rule.priority = 1u;
    rule.usid_mask = 0x0fu;
    rule.usid_value = 2u;
    rule.command_mask = (uint8_t)(1u << RFFE_CMD_REG_WRITE);
    rule.register_first = 0x10u;
    rule.register_last = 0x10u;
    rule.action = RS_ACTION_DENY;
    failures += expect_true(policy_add_rule(&device.policy, &rule),
                            "deny rule add");
    failures += expect_true(rffe_decode_word(make_test_word(2u, 0u, 0x10u,
                                                             0xaau),
                                              25u, board_time_us(), 1u,
                                              &frame),
                            "state test frame");
    decision = device_process_frame(&device, &frame);
    failures += expect_true(decision.action == RS_ACTION_DENY,
                            "armed deny");
    failures += expect_true(device.journal.count == 1u &&
                            journal_validate(&device.journal),
                            "journal record");

    board_sim_advance(RS_HOST_TIMEOUT_US + 1u);
    device_poll(&device);
    failures += expect_true(device.mode == RS_MODE_FAULT_LATCHED &&
                            !sim_active_path,
                            "host timeout fail safe");
    failures += expect_true(device_clear_fault(&device), "fault clear");
    failures += expect_true(device.mode == RS_MODE_SAFE_BYPASS,
                            "clear returns bypass");

    failures += expect_true(device_set_passive(&device),
                            "passive after clear");
    fpga_sim_set_status(RS_FPGA_ST_PLL_LOCK | RS_FPGA_ST_VIO_VALID |
                        RS_FPGA_ST_CONTENTION);
    device_poll(&device);
    failures += expect_true(device.mode == RS_MODE_FAULT_LATCHED &&
                            (device.faults & RS_FAULT_PROTOCOL) != 0u,
                            "contention fails safe");
    fpga_sim_set_status(RS_FPGA_ST_PLL_LOCK | RS_FPGA_ST_VIO_VALID);
    failures += expect_true(device_clear_fault(&device),
                            "contention fault clear");
    failures += expect_true(device_set_passive(&device),
                            "passive before voltage test");
    board_sim_set_measurements(2100u, 8u);
    device_poll(&device);
    failures += expect_true((device.faults & RS_FAULT_VIO_RANGE) != 0u,
                            "vio fault");
    board_sim_set_measurements(1800u, 8u);
    return failures;
}

static int run_self_tests(void)
{
    int failures = 0;

    failures += test_protocol();
    failures += test_decoder_and_policy();
    failures += test_state_machine();
    if (failures == 0) {
        (void)puts("RFFE Sentinel self-test: PASS");
        return EXIT_SUCCESS;
    }
    (void)fprintf(stderr, "RFFE Sentinel self-test: FAIL (%d)\n", failures);
    return EXIT_FAILURE;
}

static int run_demo(void)
{
    struct rs_device device;
    struct rffe_frame frame;
    struct rs_decision decision;

    sim_time_us = 5000u;
    sim_mmio.id = RS_FPGA_ID_EXPECTED;
    sim_mmio.status = RS_FPGA_ST_PLL_LOCK | RS_FPGA_ST_VIO_VALID;
    board_sim_set_measurements(1800u, 7u);
    device_init(&device);
    if (!device_set_passive(&device)) {
        return EXIT_FAILURE;
    }
    if (!rffe_decode_word(make_test_word(1u, 0u, 0x2au, 0x44u),
                          25u, board_time_us(), 1u, &frame)) {
        return EXIT_FAILURE;
    }
    decision = device_process_frame(&device, &frame);
    (void)printf("device=%s mode=%u usid=%u class=%s register=0x%02x "
                 "value=0x%02x action=%s journal=%zu\n",
                 RS_BOARD_NAME, (unsigned)device.mode, (unsigned)frame.usid,
                 rffe_class_name(frame.command_class),
                 (unsigned)frame.register_address, (unsigned)frame.data[0],
                 policy_action_name(decision.action), device.journal.count);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        (void)fprintf(stderr, "usage: %s --self-test | --demo\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "--self-test") == 0) {
        return run_self_tests();
    }
    if (strcmp(argv[1], "--demo") == 0) {
        return run_demo();
    }
    (void)fprintf(stderr, "unknown option: %s\n", argv[1]);
    return EXIT_FAILURE;
}
