/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "quarantine.h"
#include "wire.h"

static struct quarantine monitor;
static char packet[SERIAL_BUFFER];

static bool submit(size_t size)
{
    if (size && serial_submit(packet,size))
        return true;
    rq_tx_fail(&monitor);
    return false;
}

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* No plausible measurements if startup cannot establish its guard. */
        for (;;) { }
    }
    rq_init(&monitor,board_millis());
    uint32_t status_at=monitor.time;
    uint32_t policy_at=monitor.time;
    bool boot_pending=true;
    for (;;) {
        uint32_t now=board_millis();
        rq_sample(&monitor,now,board_levels());
        serial_poll();
        if (serial_take_timeout())
            rq_tx_fail(&monitor);
        if (!serial_busy()) {
            if (boot_pending) {
                boot_pending=!submit(rq_boot(packet,sizeof packet,board_reset_flags()));
            } else if (now-policy_at>=1000u) {
                /* Policy repeats without masquerading as a new boot. */
                policy_at=now;
                submit(rq_policy(packet,sizeof packet));
            } else if (now-status_at>=100u) {
                status_at=now;
                submit(rq_status(packet,sizeof packet,&monitor));
            } else {
                struct rq_event event;
                if (rq_pop(&monitor,&event))
                    submit(rq_event_frame(packet,sizeof packet,&event));
            }
        }
        board_watchdog_feed();
    }
}
