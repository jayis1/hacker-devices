/* Credential Canary firmware entry point
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-only
 * Authorized physical-access security assessment only.
 */
#include "board.h"
#include "protocol.h"
#include "capture.h"
#include "command.h"
#include "registers.h"

static system_status_t status;
static bool previous_tamper;
static uint32_t last_health_ms;
static uint32_t last_activity_us;

static void publish_boot(void) {
    uint8_t version[4] = { FW_VERSION_MAJOR, FW_VERSION_MINOR, 0u, 0u };
    capture_push(EVT_BOOT, IFACE_SYSTEM, version, sizeof version, 0u, board_micros());
}

static void update_indicators(void) {
    if (board_tamper_asserted()) board_set_led(20u, 0u, 0u);
    else if (status.capture_overrun) board_set_led(20u, 8u, 0u);
    else if (status.armed) board_set_led(0u, 0u, 20u);
    else board_set_led(0u, 12u, 0u);
}

static void health_poll(void) {
    uint32_t now = board_millis();
    if ((uint32_t)(now - last_health_ms) < 100u) return;
    last_health_ms = now;
    status.uptime_ms = now;
    status.capture_overrun = capture_dropped() != 0u;
    status.policy_alerts = policy_alert_count();
    bool tamper = board_tamper_asserted();
    if (tamper != previous_tamper) { policy_tamper(tamper); previous_tamper = tamper; }
    update_indicators();
    board_feed_watchdog();
}

/* IRQ hooks are intentionally tiny: protocol parsing runs from the cooperative
   loop, while real board revisions place bytes/edges into DMA queues here. */
void USART1_IRQHandler(void) {
    if (USART1->ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)USART1->RDR;
        osdp_rx_byte(IFACE_OSDP_READER, byte, board_micros());
        last_activity_us = board_micros();
    }
}
void USART2_IRQHandler(void) {
    if (USART2->ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)USART2->RDR;
        osdp_rx_byte(IFACE_OSDP_PANEL, byte, board_micros());
        last_activity_us = board_micros();
    }
}
void TIM2_IRQHandler(void) { TIM2->SR = 0u; }

int main(void) {
    board_init();
    capture_init();
    protocol_init();
    command_init();
    status.mode = MODE_SAFE_BYPASS;
    status.armed = false;
    previous_tamper = board_tamper_asserted();
    publish_boot();
    for (;;) {
        uint32_t now_us = board_micros();
        wiegand_poll(now_us);
        osdp_poll(now_us);
        command_poll();
        capture_event_t event;
        /* Bound draining so protocol timing always gets CPU between packets. */
        for (unsigned budget = 0; budget < 8u && capture_pop(&event); ++budget)
            command_stream_event(&event);
        health_poll();
        (void)last_activity_us;
    }
}
