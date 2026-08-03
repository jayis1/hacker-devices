/*
 * covert_channel.c — CC-line VDM covert channel codec
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Implements a bidirectional covert channel over the USB-C PD VDM
 * (Vendor Defined Message) channel on the CC line. Data is encoded
 * into Unstructured VDMs using a custom VDM command (VDM_CMD_COVERT_DATA).
 *
 * Frame format:
 *   [VDM Header (16-bit)] [Seq (8-bit)] [Len (8-bit)] [Data (up to 24 bytes)]
 *
 * Each VDM data object carries 4 bytes of payload, so a single PD message
 * with 7 data objects can carry 1 VDM header + 6 data objects = 24 bytes
 * of covert data per message.
 *
 * At 600 kbps BMC with 4-byte VDMs, the raw covert channel bandwidth is
 * approximately 3 KB/s. With protocol overhead and GoodCRC replies, the
 * effective throughput is approximately 1.5 KB/s.
 *
 * The covert channel works on power-only connections (no USB data needed)
 * and is invisible to USB monitoring tools, EDR solutions, and network DLP.
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

/* ---- External state ---- */
extern device_state_t g_state;

/* ---- Covert channel state (internal) ---- */
static covert_channel_t *s_covert = &g_state.covert;

/* ---- Queue helpers ---- */
static int queue_push(uint8_t *queue, uint16_t *head, uint16_t *tail,
                      uint8_t byte) {
    uint16_t next = (*head + 1) % COVERT_QUEUE_SIZE;
    if (next == *tail) return -1; /* Full */
    queue[*head] = byte;
    *head = next;
    return 0;
}

static int queue_pop(uint8_t *queue, uint16_t *head, uint16_t *tail,
                     uint8_t *byte) {
    if (*tail == *head) return -1; /* Empty */
    *byte = queue[*tail];
    *tail = (*tail + 1) % COVERT_QUEUE_SIZE;
    return 0;
}

static uint16_t queue_count(uint16_t head, uint16_t tail) {
    if (head >= tail) return head - tail;
    return COVERT_QUEUE_SIZE - tail + head;
}

/* ---- Init ---- */
void covert_channel_init(void) {
    memset(s_covert, 0, sizeof(*s_covert));
    s_covert->active = 0;
    s_covert->seq_num = 0;
}

/* ---- TX: queue data for transmission over the covert channel ---- */
int covert_channel_tx(const uint8_t *data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        if (queue_push(s_covert->tx_queue, &s_covert->tx_head,
                       &s_covert->tx_tail, data[i]) != 0) {
            return -1; /* Queue full */
        }
    }
    return 0;
}

/* ---- RX: retrieve received data from the covert channel ---- */
int covert_channel_rx(uint8_t *data, uint16_t maxlen, uint32_t timeout_ms) {
    uint16_t count = 0;
    uint32_t deadline = millis() + timeout_ms;

    while (count < maxlen) {
        uint8_t byte;
        if (queue_pop(s_covert->rx_queue, &s_covert->rx_head,
                      &s_covert->rx_tail, &byte) != 0) {
            if (millis() > deadline) break;
            continue;
        }
        data[count++] = byte;
    }
    return (int)count;
}

/* ---- Handle incoming VDM from the PD controller ---- */
void covert_channel_handle_vdm(const uint32_t *data, uint8_t count) {
    /* Each 32-bit data object contains 4 bytes of covert data */
    for (uint8_t i = 0; i < count; i++) {
        uint32_t obj = data[i];
        uint8_t bytes[4];
        bytes[0] = (uint8_t)(obj & 0xFFu);
        bytes[1] = (uint8_t)((obj >> 8) & 0xFFu);
        bytes[2] = (uint8_t)((obj >> 16) & 0xFFu);
        bytes[3] = (uint8_t)((obj >> 24) & 0xFFu);

        for (int b = 0; b < 4; b++) {
            queue_push(s_covert->rx_queue, &s_covert->rx_head,
                       &s_covert->rx_tail, bytes[b]);
        }
    }

    /* If RX monitoring is active, notify the app via BLE */
    if (s_covert->active) {
        /* Send a notification with the received data count */
        char notify[32];
        snprintf(notify, sizeof(notify), "COVERT_RX %d bytes", count * 4);
        ble_uart_send((const uint8_t *)notify, strlen(notify));
        ble_uart_send((const uint8_t *)"\r\n", 2);
    }
}

/* ---- TX pump: send queued data as VDMs ---- */
static void covert_tx_pump(void) {
    uint16_t pending = queue_count(s_covert->tx_head, s_covert->tx_tail);
    if (pending == 0) return;

    /* Build a VDM with up to 6 data objects (24 bytes of payload) */
    uint32_t vdm_data[6];
    uint8_t vdm_count = 0;

    for (uint8_t i = 0; i < 6 && pending > 0; i++) {
        uint32_t obj = 0;
        for (int b = 0; b < 4; b++) {
            uint8_t byte = 0;
            if (queue_pop(s_covert->tx_queue, &s_covert->tx_head,
                          &s_covert->tx_tail, &byte) == 0) {
                obj |= (uint32_t)byte << (b * 8);
            }
        }
        vdm_data[i] = obj;
        vdm_count++;
        pending = queue_count(s_covert->tx_head, s_covert->tx_tail);
    }

    /* Build VDM header: Unstructured VDM with custom command */
    uint16_t vdm_header = VDM_UNSTRUCT | VDM_CMD_COVERT_DATA;
    vdm_header |= (uint16_t)(s_covert->seq_num++ & 0x0Fu) << 0;

    /* Send the VDM on the sink port (toward the target) */
    pd_send_vdm(PD_PORT_SINK, vdm_header, vdm_data, vdm_count);
}

/* ---- Covert channel poll (called from main loop) ---- */
void covert_channel_poll(void) {
    /* Pump TX queue at most once per 10ms to avoid flooding the CC line */
    static uint32_t last_tx = 0;
    if ((millis() - last_tx) >= 10) {
        last_tx = millis();
        covert_tx_pump();
    }
}

/* ---- Get pending TX bytes ---- */
uint16_t covert_channel_tx_pending(void) {
    return queue_count(s_covert->tx_head, s_covert->tx_tail);
}

/* ---- Get pending RX bytes ---- */
uint16_t covert_channel_rx_pending(void) {
    return queue_count(s_covert->rx_head, s_covert->rx_tail);
}

/* ---- snprintf stub (if not available in minimal libc) ---- */
#ifndef HAVE_SNPRINTF
#include <stdio.h>
#endif