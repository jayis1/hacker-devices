/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "quorum.h"
#include "wire.h"

static struct quorum monitor;
static char packet[SERIAL_BUFFER];

static bool submit(size_t size)
{
    if (size && serial_submit(packet,size))
        return true;
    qw_tx_fail(&monitor);
    return false;
}

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* Startup uncertainty must not produce plausible evidence. */
        for (;;) { }
    }
    qw_init(&monitor,board_millis());
    uint32_t status_at=monitor.time;
    uint32_t policy_at=monitor.time;
    bool boot_pending=true;
    for (;;) {
        uint32_t now=board_millis();
        qw_sample(&monitor,now,board_levels());
        serial_poll();
        if (serial_take_timeout())
            qw_tx_fail(&monitor);
        if (!serial_busy()) {
            if (boot_pending) {
                boot_pending=!submit(qw_boot(packet,sizeof packet,board_reset_flags()));
            } else if (now-policy_at>=1000u) {
                /* Recover from late attachment or loss of the one-shot boot frame. */
                policy_at=now;
                submit(qw_policy(packet,sizeof packet));
            } else if (now-status_at>=100u) {
                status_at=now;
                submit(qw_status(packet,sizeof packet,&monitor));
            } else {
                struct qw_event e;
                if (qw_pop(&monitor,&e))
                    submit(qw_event_frame(packet,sizeof packet,&e));
            }
        }
        board_watchdog_feed();
    }
}
