/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "budget.h"
#include "wire.h"

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* Failure to establish recovery suppresses all measurement output. */
        for (;;) { }
    }
    Budget budget;
    budget_init(&budget, board_millis());
    uint32_t last_sample = budget.time;
    uint32_t last_status = budget.time;
    uint32_t last_policy = budget.time;
    uint32_t tx_fail = 0;
    bool boot_pending = true;
    bool policy_pending = true;
    char frame[SERIAL_BUFFER];
    for (;;) {
        uint32_t now = board_millis();
        if (now != last_sample) {
            budget_tick(&budget, now, board_levels());
            last_sample = now;
        }
        serial_poll();
        if (serial_take_timeout())
            tx_fail = budget_add(tx_fail, 1);
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
                size = wire_status(frame, sizeof frame, &budget, tx_fail);
                last_status = now;
            } else {
                Event e;
                if (budget_pop(&budget, &e))
                    size = wire_event(frame, sizeof frame, &e);
            }
            if (size && !serial_submit(frame, size))
                tx_fail = budget_add(tx_fail, 1);
        }
        board_watchdog_feed();
    }
}
