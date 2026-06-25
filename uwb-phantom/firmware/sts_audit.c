/*
 * sts_audit.c — STS enforcement auditor.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Each audit suite acts as a (possibly malformed) prover against the
 * target verifier and records whether the verifier accepted the frame.
 * "Accepted" is detected by observing an unlock/ranging-complete frame
 * from the verifier within the timeout window.
 *
 * The auditor is purely a research tool: it proves whether a particular
 * implementation falls back to insecure behaviour.  Operators must have
 * authorisation for the target verifier before running any suite.
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "board.h"
#include "registers.h"
#include "dw3110.h"
#include "ble_comms.h"
#include "sts_audit.h"

static const char *TAG = "sts_audit";

/* A minimal 802.15.4z ranging poll frame.  The STS field is appended
 * by the DW3110 hardware when STS is enabled; the MAC bytes below are
 * the plaintext portion. */
static const uint8_t POLL_FRAME[] = {
    0x41, 0x88,        /* FCF: data frame, 802.15.4z PAN compression */
    0x00,               /* sequence number (filled at runtime) */
    0xFF, 0xFF,         /* PAN ID broadcast */
    0xFF, 0xFF,         /* dst short addr broadcast */
    0x00, 0x00,         /* src short addr (filled at runtime) */
    0x02, 0x00,         /* ranging function code: poll */
};

static int send_poll(const uint8_t *eui, bool with_sts, uint8_t seq)
{
    uint8_t frame[sizeof(POLL_FRAME)];
    memcpy(frame, POLL_FRAME, sizeof(frame));
    frame[2] = seq;
    if (eui) {
        frame[6] = eui[6]; frame[7] = eui[7];
    }
    return dw3110_tx(frame, sizeof(frame), with_sts, false, 0);
}

static bool wait_for_accept(uint32_t timeout_ms, uint8_t *out_quality)
{
    uint64_t deadline = esp_timer_get_time() + (uint64_t)timeout_ms * 1000;
    while (esp_timer_get_time() < deadline) {
        uint32_t status;
        if (dw3110_read_sys_status(&status) == 0 && (status & SYS_STATUS_RXDFR)) {
            uint8_t buf[127];
            uwb_ranging_t rng;
            size_t len = 0;
            if (dw3110_rx_read(buf, sizeof(buf), &len, &rng) == 0 && len > 0) {
                /* Heuristic: a verifier "accept" frame is typically a
                 * 802.15.4 data frame with function code 0x06 (range
                 * report) or 0x10 (unlock). */
                if (len >= 10 && (buf[9] == 0x06 || buf[9] == 0x10)) {
                    if (out_quality) *out_quality = rng.sts_quality;
                    return true;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return false;
}

int sts_audit_run(audit_suite_t suite, const uint8_t target_eui[8],
                  audit_result_t *out, uint32_t timeout_ms)
{
    if (!out) return -EINVAL;
    memset(out, 0, sizeof(*out));

    if (!ble_comms_check_target(target_eui)) {
        ESP_LOGW(TAG, "target EUI not in authorised list — refusing");
        return -EPERM;
    }

    ESP_LOGI(TAG, "audit suite %d starting (timeout %lu ms)",
             suite, (unsigned long)timeout_ms);

    /* Open RX before each poll so we can catch the verifier's reply. */
    dw3110_rx_start(false, 0);

    switch (suite) {
    case AUDIT_NO_STS: {
        dw3110_set_sts_mode(UWB_STS_OFF);
        send_poll(target_eui, false, 1);
        out->accepted_no_sts = wait_for_accept(timeout_ms,
                                                &out->captured_sts_quality);
        out->frames_examined = 1;
        break;
    }
    case AUDIT_STATIC_STS: {
        /* Use a zero STS key/IV — many buggy implementations ship
         * with this and will accept it. */
        uint8_t zero_key[16] = {0};
        uint8_t zero_iv[16]  = {0};
        dw3110_load_sts(zero_key, zero_iv);
        dw3110_set_sts_mode(UWB_STS_STATIC);
        send_poll(target_eui, true, 2);
        out->accepted_static_sts = wait_for_accept(timeout_ms,
                                                   &out->captured_sts_quality);
        out->frames_examined = 1;
        break;
    }
    case AUDIT_DOWNGRADE: {
        /* First send a 4a (legacy) poll; if that fails, retry as 4z. */
        dw3110_set_sts_mode(UWB_STS_OFF);
        send_poll(target_eui, false, 3);
        bool got = wait_for_accept(timeout_ms / 2, NULL);
        if (!got) {
            dw3110_set_sts_mode(UWB_STS_DYNAMIC);
            send_poll(target_eui, true, 4);
            got = wait_for_accept(timeout_ms / 2, NULL);
        }
        out->accepted_downgrade = got;
        out->frames_examined = 2;
        break;
    }
    case AUDIT_COUNTER_REPLAY: {
        /* Send the same sequence number twice; if the verifier
         * accepts the second one, anti-replay is missing. */
        dw3110_set_sts_mode(UWB_STS_DYNAMIC);
        send_poll(target_eui, true, 5);
        wait_for_accept(timeout_ms / 2, NULL);
        /* Replay */
        send_poll(target_eui, true, 5);
        out->accepted_replay = wait_for_accept(timeout_ms / 2,
                                               &out->captured_sts_quality);
        out->frames_examined = 2;
        break;
    }
    }

    ESP_LOGI(TAG, "audit suite %d done: no_sts=%d static=%d downgrade=%d replay=%d",
             suite, out->accepted_no_sts, out->accepted_static_sts,
             out->accepted_downgrade, out->accepted_replay);
    return 0;
}