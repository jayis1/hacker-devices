/*
 * ble_comms.c — BLE GATT service and authorised-target store.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Implements the custom GATT service documented in the README:
 *
 *   Service  : 0000FEA5-...  ("UWB-Phantom")
 *     cmd     (write)   — JSON command
 *     event   (notify)  — JSON event stream
 *     log     (notify)  — raw PCAP chunks
 *     target  (r/w)     — authorised-target blob
 *
 * Commands are tiny JSON objects.  The handler is intentionally small:
 * it parses the verb, dispatches to a mode-specific action, and emits
 * an event in response.  All long-running work happens in dedicated
 * FreeRTOS tasks; the GATT handler never blocks.
 *
 * The authorised-target store is held in RAM only.  When a target
 * record arrives we verify the HMAC-SHA256 against a device-unique
 * secret burned into eFuse at manufacture.  Records are lost on power
 * down by design.
 */

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_hmac.h"
#include "nvs_flash.h"

#include "board.h"
#include "ble_comms.h"

static const char *TAG = "ble";

/* ---- GATT UUIDs ----------------------------------------- */

/* 16-bit service: 0xFEA5 ("UWB-Phantom") */
#define SRVC_UUID       0xFEA5
#define CHAR_CMD_UUID   0xFEA6
#define CHAR_EVT_UUID   0xFEA7
#define CHAR_LOG_UUID   0xFEA8
#define CHAR_TGT_UUID   0xFEA9

/* ---- Module state -------------------------------------- */

static target_record_t s_targets[TARGET_RECORD_MAX];
static size_t           s_target_n;
static SemaphoreHandle_t s_targets_lock;
static bool              s_adv_on;

/* HMAC key — in production this comes from eFuse BLOCK_KEY0.  Here we
 * use a compile-time placeholder; the companion app shares it. */
static const uint8_t s_hmac_key[32] = {
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
    0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
};

/* ---- Target store --------------------------------------- */

static bool verify_hmac(const target_record_t *t)
{
    uint8_t msg[8 + 24 + 8];
    memcpy(msg, t->eui, 8);
    memcpy(msg + 8, t->label, 24);
    memcpy(msg + 32, &t->nonce, 8);

    uint8_t mac[32];
    esp_hmac_sha256(s_hmac_key, msg, sizeof(msg), mac);
    return memcmp(mac, t->hmac, 32) == 0;
}

int ble_comms_add_target(const target_record_t *t)
{
    if (!verify_hmac(t)) {
        ESP_LOGW(TAG, "target HMAC mismatch — rejected");
        return -EPERM;
    }
    xSemaphoreTake(s_targets_lock, portMAX_DELAY);
    if (s_target_n >= TARGET_RECORD_MAX) {
        xSemaphoreGive(s_targets_lock);
        return -ENOSPC;
    }
    /* anti-replay: reject nonces we have already seen */
    for (size_t i = 0; i < s_target_n; i++) {
        if (s_targets[i].nonce == t->nonce) {
            xSemaphoreGive(s_targets_lock);
            return -EEXIST;
        }
    }
    s_targets[s_target_n++] = *t;
    xSemaphoreGive(s_targets_lock);
    ESP_LOGI(TAG, "target added: %s (EUI %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x)",
             t->label, t->eui[0],t->eui[1],t->eui[2],t->eui[3],
             t->eui[4],t->eui[5],t->eui[6],t->eui[7]);
    return 0;
}

bool ble_comms_target_loaded(void)
{
    return s_target_n > 0;
}

bool ble_comms_check_target(const uint8_t eui[8])
{
    xSemaphoreTake(s_targets_lock, portMAX_DELAY);
    bool ok = false;
    for (size_t i = 0; i < s_target_n; i++) {
        if (memcmp(s_targets[i].eui, eui, 8) == 0) { ok = true; break; }
    }
    xSemaphoreGive(s_targets_lock);
    return ok;
}

int ble_comms_list_targets(target_record_t *out, size_t cap, size_t *n)
{
    xSemaphoreTake(s_targets_lock, portMAX_DELAY);
    size_t m = (s_target_n < cap) ? s_target_n : cap;
    memcpy(out, s_targets, m * sizeof(target_record_t));
    if (n) *n = m;
    xSemaphoreGive(s_targets_lock);
    return 0;
}

void ble_comms_clear_targets(void)
{
    xSemaphoreTake(s_targets_lock, portMAX_DELAY);
    memset(s_targets, 0, sizeof(s_targets));
    s_target_n = 0;
    xSemaphoreGive(s_targets_lock);
}

/* ---- GATT event sink (simplified) ---------------------- */
/*
 *  A complete ESP-IDF GATT server is ~300 lines of plumbing.  In this
 *  reference build we sketch the profile table and dispatch, omitting
 *  the verbose esp_gatts_cb boilerplate.  The companion app talks to
 *  the four characteristics exactly as documented in the README.
 */

static void gatts_cmd_write(const uint8_t *data, size_t len)
{
    /* Tiny JSON command parser.  Real firmware links a JSON lib; here
     * we parse a single verb. */
    char buf[64];
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, data, len);
    buf[len] = 0;
    ESP_LOGI(TAG, "cmd: %s", buf);

    /* Dispatch verbs.  Real implementation calls into mode handlers. */
    if (strncmp(buf, "{\"v\":\"idle\"", 11) == 0) {
        ble_comms_notify_event("{\"v\":\"idle_ok\"}");
    } else if (strncmp(buf, "{\"v\":\"sniff\"", 11) == 0) {
        if (!ble_comms_target_loaded()) {
            ble_comms_notify_event("{\"v\":\"err\",\"why\":\"no_target\"}");
            return;
        }
        ble_comms_notify_event("{\"v\":\"sniff_ok\"}");
    } else if (strncmp(buf, "{\"v\":\"relay\"", 11) == 0) {
        if (!ble_comms_target_loaded()) {
            ble_comms_notify_event("{\"v\":\"err\",\"why\":\"no_target\"}");
            return;
        }
        ble_comms_notify_event("{\"v\":\"relay_ok\"}");
    } else {
        ble_comms_notify_event("{\"v\":\"err\",\"why\":\"bad_cmd\"}");
    }
}

/* ---- Advertising ---------------------------------------- */

void ble_comms_advertise(bool on)
{
    s_adv_on = on;
    if (on) {
        esp_ble_adv_data_t ad = {0};
        ad.set_scan_rsp = false;
        ad.include_name = true;
        ad.min_interval = 0x20;
        ad.max_interval = 0x40;
        esp_ble_gap_config_adv_data(&ad);
        esp_ble_gap_start_advertising(&ad);
    } else {
        esp_ble_gap_stop_advertising();
    }
}

/* ---- Event/log notify (stubs that write to console) ---- */

void ble_comms_notify_event(const char *json)
{
    ESP_LOGI(TAG, "evt: %s", json);
}

void ble_comms_notify_log(const uint8_t *chunk, size_t n)
{
    /* In the real build this writes to the LOG characteristic's value
     * + sends a notification.  Here we just log the size. */
    ESP_LOGI(TAG, "log chunk %u bytes", (unsigned)n);
}

/* ---- Init ----------------------------------------------- */

int ble_comms_init(void)
{
    s_targets_lock = xSemaphoreCreateMutex();
    s_target_n = 0;
    s_adv_on  = false;

    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    /* Real build would call esp_ble_gatts_app_register() here. */
    ESP_LOGI(TAG, "BLE GATT service initialised");
    return 0;
}