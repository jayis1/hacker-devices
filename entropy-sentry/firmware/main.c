/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "health.h"
#include "wire.h"

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* No measurements are emitted if the recovery mechanism cannot start. */
        for (;;) { }
    }
    Health health;
    health_init(&health, board_millis());
    uint32_t last_sample = health.time;
    uint32_t last_status = health.time;
    uint32_t last_policy = health.time;
    uint32_t tx_fail = 0;
    bool boot_pending = true;
    bool policy_pending = true;
    char frame[SERIAL_BUFFER];
    for (;;) {
        uint32_t now = board_millis();
        if (now != last_sample) {
            health_tick(&health, now, board_levels());
            last_sample = now;
        }
        serial_poll();
        if (serial_take_timeout())
            tx_fail = health_add(tx_fail, 1);
        if (!serial_busy()) {
            size_t size = 0;
            if (boot_pending) {
                size = wire_boot(frame, sizeof frame, board_reset_flags());
                boot_pending = false;
            } else if (policy_pending || now - last_policy >= 1000u) {
                size = wire_policy(frame, sizeof frame);
                policy_pending = false;
                last_policy = now;
            } else if (now - last_status >= 100u) {
                size = wire_status(frame, sizeof frame, &health, tx_fail);
                last_status = now;
            } else {
                Event event;
                if (health_pop(&health, &event))
                    size = wire_event(frame, sizeof frame, &event);
            }
            if (size && !serial_submit(frame, size))
                tx_fail = health_add(tx_fail, 1);
        }
        board_watchdog_feed();
    }
}
