/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "erase.h"
#include "wire.h"

static struct erase monitor;
static char packet[SERIAL_BUFFER];

static bool submit(size_t size)
{
    if (size && serial_submit(packet,size))
        return true;
    ew_tx_fail(&monitor);
    return false;
}

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* No plausible measurements if startup cannot establish its guard. */
        for (;;) { }
    }
    ew_init(&monitor,board_millis());
    uint32_t status_at=monitor.time;
    uint32_t policy_at=monitor.time;
    bool boot_pending=true;
    for (;;) {
        uint32_t now=board_millis();
        ew_sample(&monitor,now,board_levels());
        serial_poll();
        if (serial_take_timeout())
            ew_tx_fail(&monitor);
        if (!serial_busy()) {
            if (boot_pending) {
                boot_pending=!submit(ew_boot(packet,sizeof packet,board_reset_flags()));
            } else if (now-policy_at>=1000u) {
                /* Policy repeats without masquerading as a new boot. */
                policy_at=now;
                submit(ew_policy(packet,sizeof packet));
            } else if (now-status_at>=100u) {
                status_at=now;
                submit(ew_status(packet,sizeof packet,&monitor));
            } else {
                struct ew_event event;
                if (ew_pop(&monitor,&event))
                    submit(ew_event_frame(packet,sizeof packet,&event));
            }
        }
        board_watchdog_feed();
    }
}
