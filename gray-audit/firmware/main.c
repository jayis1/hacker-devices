/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "audit.h"
#include "board.h"
#include "wire.h"

static audit_t audit;
static char line[WIRE_CAPACITY];

static void send(size_t n)
{
    if (!n || !serial_submit(line, n))
        audit_tx_failure(&audit);
}

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* No health claims without a running watchdog; inputs remain inputs. */
        for (;;) { }
    }
    uint32_t now = board_millis();
    audit_init(&audit, now, board_levels());
    send(wire_boot(line, sizeof line, board_reset_flags()));
    uint32_t report_at = now;
    uint8_t slot = 0;
    for (;;) {
        now = board_millis();
        audit_sample(&audit, now, board_levels());
        serial_poll();
        if (serial_take_timeout())
            audit_tx_failure(&audit);
        board_watchdog_feed();
        if (serial_busy())
            continue;
        /* Reserve bandwidth for five periodic records regardless of alarms. */
        if (now - report_at >= 100u) {
            report_at = now;
            if (slot < 2)
                send(wire_axis(line, sizeof line, &audit, slot, now));
            else if (slot < 4)
                send(wire_histogram(line, sizeof line, &audit, slot - 2u, now));
            else
                send(wire_health(line, sizeof line, &audit, now));
            slot = (slot + 1u) % 5u;
        } else {
            audit_event_t e;
            if (audit_pop(&audit, &e))
                send(wire_event(line, sizeof line, &e));
        }
    }
}
