/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "epoch.h"
#include "wire.h"

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* Failure to establish recovery suppresses all measurement output. */
        for (;;) { }
    }
    Epoch epoch;
    epoch_init(&epoch, board_millis());
    uint32_t last_sample = epoch.time;
    uint32_t last_status = epoch.time;
    uint32_t last_policy = epoch.time;
    uint32_t tx_fail = 0;
    bool boot_pending = true;
    bool policy_pending = true;
    char frame[SERIAL_BUFFER];
    for (;;) {
        uint32_t now = board_millis();
        if (now != last_sample) {
            epoch_tick(&epoch, now, board_levels());
            last_sample = now;
        }
        serial_poll();
        if (serial_take_timeout())
            tx_fail = epoch_add(tx_fail, 1);
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
                size = wire_status(frame, sizeof frame, &epoch, tx_fail);
                last_status = now;
            } else {
                Event e;
                if (epoch_pop(&epoch, &e))
                    size = wire_event(frame, sizeof frame, &e);
            }
            if (size && !serial_submit(frame, size))
                tx_fail = epoch_add(tx_fail, 1);
        }
        board_watchdog_feed();
    }
}
