/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "drain.h"
#include "wire.h"

static Drain drain;
static char frame[SERIAL_BUFFER];
static bool watchdog_ok;
static bool boot_pending;
static bool policy_pending;
static uint32_t status_at;
static uint32_t policy_at;

static void send_frame(size_t length)
{
    if (!length || !serial_submit(frame, length))
        drain_tx_failure(&drain);
}

static void initialize(void)
{
    board_init();
    drain_init(&drain);
    watchdog_ok = board_watchdog_start();
    if (!watchdog_ok)
        drain_halt(&drain, RD_WATCHDOG);
    drain_sample(&drain, board_millis(), board_levels());
    boot_pending = true;
    policy_pending = true;
    status_at = board_millis();
    policy_at = status_at;
}

static void telemetry(uint32_t now)
{
    if (serial_busy())
        return;
    if (boot_pending) {
        send_frame(wire_boot(frame, sizeof frame, board_reset_flags()));
        boot_pending = false;
        return;
    }
    /* Repeated policy is not a reset: late attachment is supported. */
    if (policy_pending || now - policy_at >= 1000u) {
        send_frame(wire_policy(frame, sizeof frame));
        policy_pending = false;
        policy_at = now;
        return;
    }
    if (now - status_at >= 100u) {
        send_frame(wire_status(frame, sizeof frame, &drain));
        status_at = now;
    }
}

static void iteration(void)
{
    uint32_t now = board_millis();
    drain_sample(&drain, now, board_levels());
    serial_poll();
    if (serial_take_timeout())
        drain_tx_failure(&drain);
    telemetry(now);
    if (watchdog_ok)
        board_watchdog_feed();
}

int main(void)
{
    initialize();
    for (;;)
        iteration();
}
