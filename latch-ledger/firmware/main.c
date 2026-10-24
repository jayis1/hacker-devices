/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "monitor.h"
#include "wire.h"

static monitor_t monitor;
static char output[WIRE_CAPACITY];

static void transmit(size_t size)
{
    if (!size || !serial_submit(output, size))
        monitor_tx_failure(&monitor);
}

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* No trustworthy watchdog means no measurement claim; stay input-only. */
        for (;;) { }
    }
    uint32_t now = board_millis();
    monitor_init(&monitor, now, board_contacts());
    transmit(wire_boot(output, sizeof output, board_reset_flags()));
    uint32_t report_at = now;
    uint8_t report_slot = 0;
    for (;;) {
        now = board_millis();
        monitor_sample(&monitor, now, board_contacts());
        serial_poll();
        if (serial_take_timeout())
            monitor_tx_failure(&monitor);
        board_watchdog_feed();
        if (serial_busy())
            continue;
        /* Guaranteed report bandwidth, even during continuous contact chatter. */
        if (now - report_at >= 100u) {
            report_at = now;
            if (report_slot < CHANNELS)
                transmit(wire_status(output, sizeof output, &monitor, report_slot, now));
            else if (report_slot < CHANNELS + PAIRS)
                transmit(wire_pair(output, sizeof output, &monitor,
                                   report_slot - CHANNELS, now));
            else
                transmit(wire_health(output, sizeof output, &monitor, now));
            report_slot = (uint8_t)((report_slot + 1) % (CHANNELS + PAIRS + 1));
        } else {
            event_t event;
            if (monitor_pop(&monitor, &event))
                transmit(wire_event(output, sizeof output, &event));
        }
    }
}
