/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "lease.h"
#include "wire.h"

static lease_t state;
static char line[WIRE_CAPACITY];

static void send(size_t length)
{
    if (!length || !serial_submit(line, length))
        lease_tx_failure(&state);
}

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* No apparently healthy output when our own watchdog is unavailable. */
        for (;;) { }
    }
    uint32_t now = board_millis();
    lease_init(&state, now, board_levels());
    send(wire_boot(line, sizeof line, board_reset_flags()));
    uint32_t reported = now;
    for (;;) {
        now = board_millis();
        lease_sample(&state, now, board_levels());
        serial_poll();
        if (serial_take_timeout())
            lease_tx_failure(&state);
        board_watchdog_feed();
        if (serial_busy())
            continue;
        /* Status cannot be starved by edge traffic or an alarm storm. */
        if (now - reported >= 100u) {
            reported = now;
            send(wire_status(line, sizeof line, &state, now));
        } else {
            lease_event_t event;
            if (lease_pop(&state, &event))
                send(wire_event(line, sizeof line, &event));
        }
    }
}
