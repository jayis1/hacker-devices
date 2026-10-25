/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "consent.h"
#include "wire.h"

static struct consent monitor;
static char packet[SERIAL_BUFFER];

static bool submit(size_t size)
{
    if (size && serial_submit(packet,size))
        return true;
    cg_tx_fail(&monitor);
    return false;
}

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* No plausible measurements if startup cannot establish its guard. */
        for (;;) { }
    }
    cg_init(&monitor,board_millis());
    uint32_t status_at=monitor.time;
    uint32_t policy_at=monitor.time;
    bool boot_pending=true;
    for (;;) {
        uint32_t now=board_millis();
        cg_sample(&monitor,now,board_levels());
        serial_poll();
        if (serial_take_timeout())
            cg_tx_fail(&monitor);
        if (!serial_busy()) {
            if (boot_pending) {
                boot_pending=!submit(cg_boot(packet,sizeof packet,board_reset_flags()));
            } else if (now-policy_at>=1000u) {
                /* Policy repeats without masquerading as a new boot. */
                policy_at=now;
                submit(cg_policy(packet,sizeof packet));
            } else if (now-status_at>=100u) {
                status_at=now;
                submit(cg_status(packet,sizeof packet,&monitor));
            } else {
                struct cg_event event;
                if (cg_pop(&monitor,&event))
                    submit(cg_event_frame(packet,sizeof packet,&event));
            }
        }
        board_watchdog_feed();
    }
}
