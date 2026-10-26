/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "engine.h"
#include "wire.h"
static Witness sensor;
static char frame[SERIAL_BUFFER];
static bool watchdog_ok;
static bool boot_pending;
static bool policy_pending;
static uint32_t status_at;
static uint32_t policy_at;
static void send_frame(size_t length)
{
    if (!length || !serial_submit(frame, length))
        witness_tx_failure(&sensor);
}
static void initialize(void)
{
    board_init();
    witness_init(&sensor);
    watchdog_ok = board_watchdog_start();
    if (!watchdog_ok)
        witness_halt(&sensor, DW_WATCHDOG);
    witness_sample(&sensor, board_millis(), board_levels());
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
    /* Recurring policy is not a reset: late attachment must work. */
    if (policy_pending || now - policy_at >= 1000u) {
        send_frame(wire_policy(frame, sizeof frame));
        policy_pending = false;
        policy_at = now;
        return;
    }
    if (now - status_at >= 100u) {
        send_frame(wire_status(frame, sizeof frame, &sensor));
        status_at = now;
    }
}
static void iteration(void)
{
    uint32_t now = board_millis();
    witness_sample(&sensor, now, board_levels());
    serial_poll();
    if (serial_take_timeout())
        witness_tx_failure(&sensor);
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
