/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "grant.h"
#include "wire.h"

static struct grant ledger;
static char packet[SERIAL_BUFFER];

static bool submit(size_t size)
{
    if (size && serial_submit(packet, size))
        return true;
    grant_tx_fail(&ledger);
    return false;
}

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* No observations are published if startup health is uncertain. */
        for (;;) { }
    }
    grant_init(&ledger, board_millis());
    uint32_t status_at = ledger.time;
    bool boot_pending = true;
    for (;;) {
        uint32_t now = board_millis();
        grant_sample(&ledger, now, board_levels());
        serial_poll();
        if (serial_take_timeout())
            grant_tx_fail(&ledger);
        if (!serial_busy()) {
            if (boot_pending) {
                boot_pending = !submit(gl_boot(packet, sizeof packet, board_reset_flags()));
            } else if (now - status_at >= 100u) {
                /* Health snapshots have priority over the bounded event queue. */
                status_at = now;
                submit(gl_status(packet, sizeof packet, &ledger));
            } else {
                struct gl_event e;
                if (grant_pop(&ledger, &e))
                    submit(gl_event_frame(packet, sizeof packet, &e));
            }
        }
        board_watchdog_feed();
    }
}
