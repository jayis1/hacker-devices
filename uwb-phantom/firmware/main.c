/*
 * main.c — UWB-PHANTOM firmware entry point.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * UWB-PHANTOM is an ultra-wideband security research appliance.  See
 * README.md for the full design.  This file wires up the peripherals,
 * runs the mode FSM, and exposes the user-facing controls (buttons +
 * OLED + BLE GATT + USB-CDC console).
 *
 * Build with ESP-IDF v5.2:
 *     idf.py set-target esp32s3
 *     idf.py build flash monitor
 *
 * The Makefile in this directory wraps `idf.py` for convenience.
 *
 * Authorised-use enforcement
 * --------------------------
 * The firmware refuses to enter any transmit mode (TWR anchor, relay,
 * STS audit, tracker impersonation) until at least one authorised
 * target record has been loaded via the companion app.  Pure sniff and
 * tracker-scan (RX-only) modes do not require a target, but the OLED
 * still shows a "NO AUTH" warning so the operator knows TX is disabled.
 */

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#include "board.h"
#include "registers.h"
#include "dw3110.h"
#include "uwb_sniffer.h"
#include "relay.h"
#include "sts_audit.h"
#include "display.h"
#include "ble_comms.h"
#include "fuel_gauge.h"

static const char *TAG = "main";

/* ---- Mode FSM ------------------------------------------- */

typedef enum {
    MODE_IDLE = 0,
    MODE_TWR,
    MODE_SNIFF,
    MODE_STS_AUDIT,
    MODE_RELAY,
    MODE_TRACK,
    MODE_COUNT,
} app_mode_t;

static const char *MODE_NAMES[MODE_COUNT] = {
    "IDLE", "TWR", "SNIFF", "STS-AUDIT", "RELAY", "TRACK",
};

static app_mode_t   s_mode = MODE_IDLE;
static uwb_config_t  s_uwb_cfg;
static relay_state_t s_relay;

/* ---- Button events -------------------------------------- */

static QueueHandle_t s_btn_q;

static void IRAM_ATTR btn_isr(void *arg)
{
    uint8_t pin = (uint8_t)(uintptr_t)arg;
    BaseType_t hp = pdFALSE;
    xQueueSendFromISR(s_btn_q, &pin, &hp);
    if (hp) portYIELD_FROM_ISR();
}

static void buttons_init(void)
{
    s_btn_q = xQueueCreate(8, sizeof(uint8_t));
    static const int pins[3] = { BTN_MODE_GPIO, BTN_UP_GPIO, BTN_DOWN_GPIO };
    for (int i = 0; i < 3; i++) {
        gpio_config_t c = {
            .pin_bit_mask = 1ULL << pins[i],
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_NEGEDGE,
        };
        gpio_config(&c);
        gpio_isr_handler_add(pins[i], btn_isr, (void *)(uintptr_t)pins[i]);
    }
}

/* ---- UWB config defaults -------------------------------- */

static void uwb_config_defaults(uwb_config_t *c)
{
    memset(c, 0, sizeof(*c));
    c->channel       = UWB_DEFAULT_CHANNEL;
    c->prf           = UWB_DEFAULT_PRF;
    c->plen          = UWB_DEFAULT_PLEN;
    c->sfd           = UWB_SFD_4Z;
    c->sts_mode      = UWB_STS_DYNAMIC;
    c->sts_len       = UWB_DEFAULT_STS_LEN;
    c->pan_id        = 0xDECA;
    c->short_addr    = 0x0001;
    c->ant_delay_tx = DEFAULT_ANT_DLY_TX;
    c->ant_delay_rx = DEFAULT_ANT_DLY_RX;
    /* EUI-64 — in production this is read from OTP or the MAC fuse. */
    c->eui[0] = 0x01; c->eui[1] = 0x02; c->eui[2] = 0x03; c->eui[3] = 0x04;
    c->eui[4] = 0x05; c->eui[5] = 0x06; c->eui[6] = 0x07; c->eui[7] = 0x08;
}

/* ---- Sniffer callback for in-app frame list --------------- */

static void sniff_cb(const sniff_frame_t *f, void *user)
{
    (void)user;
    /* Forward the frame to the relay engine if armed. */
    if (s_mode == MODE_RELAY) {
        relay_on_frame(0, f->frame, f->frame_len, f->rx_dtu, f->sts_quality);
    }
    /* Stream PCAP chunks over BLE log characteristic in small chunks. */
    if (f->frame_len <= 64) {
        ble_comms_notify_log(f->frame, f->frame_len);
    }
}

/* ---- TWR ranging demo ----------------------------------- */
/*
 *  In TWR mode the device acts as an anchor: it listens for a poll,
 *  records the receive time, then sends a response that includes both
 *  the poll RX time and the response TX time.  The initiator computes
 *  the distance.  We also compute the distance from our side using
 *  single-sided TWR for display.
 */

static void twr_task(void *arg)
{
    (void)arg;
    uint8_t frame[127];
    uwb_ranging_t rng;
    size_t len = 0;
    double distance = -1.0;

    dw3110_rx_start(false, 0);
    while (s_mode == MODE_TWR) {
        uint32_t status;
        if (dw3110_read_sys_status(&status) == 0 && (status & SYS_STATUS_RXDFR)) {
            if (dw3110_rx_read(frame, sizeof(frame), &len, &rng) == 0 && len > 0) {
                /* Build a response: echo back with our TX time. */
                uint64_t now;
                dw3110_read_sys_time(&now);
                uint8_t resp[16];
                memcpy(resp, frame, MIN(len, 16));
                /* Insert our reply timestamp at offset 9. */
                for (int i = 0; i < 5; i++)
                    resp[9 + i] = (uint8_t)((now >> (i * 8)) & 0xFF);
                /* Reply after ~200 µs. */
                uint64_t tx_time = now + (uint64_t)200 * UWB_DTU_PER_US;
                dw3110_tx(resp, sizeof(resp), true, true, tx_time);
                distance = dw3110_dtu_to_meters_one_way(
                    (int64_t)rng.rx_stamp - (int64_t)now);
                if (distance < 0) distance = -1.0;
            }
            dw3110_rx_start(false, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

/* ---- Tracker scan -------------------------------------- */
/*
 *  Sweep channels 5 and 9 in RX, count unique emitters by their short
 *  address, and report each new one via BLE.  Pure passive mode: no
 *  transmission, so no authorised-target record is required.
 */

static void track_task(void *arg)
{
    (void)arg;
    uint8_t frame[127];
    uwb_ranging_t rng;
    size_t len = 0;
    uint8_t ch = UWB_CHANNEL_5;
    uint32_t seen[256] = {0};   /* bit-set of seen short addresses */
    uint32_t count = 0;

    while (s_mode == MODE_TRACK) {
        s_uwb_cfg.channel = ch;
        dw3110_configure_channel(&s_uwb_cfg);
        dw3110_rx_start(false, 0);

        int64_t deadline = esp_timer_get_time() + 5 * 1000000; /* 5 s */
        while (esp_timer_get_time() < deadline && s_mode == MODE_TRACK) {
            uint32_t status;
            if (dw3110_read_sys_status(&status) == 0 && (status & SYS_STATUS_RXDFR)) {
                if (dw3110_rx_read(frame, sizeof(frame), &len, &rng) == 0 && len > 0) {
                    uint8_t src = frame[7];  /* src short addr low byte */
                    if (!(seen[src / 32] & (1u << (src % 32)))) {
                        seen[src / 32] |= (1u << (src % 32));
                        count++;
                        char evt[64];
                        snprintf(evt, sizeof(evt),
                                 "{\"v\":\"track\",\"src\":%u,\"ch\":%u,\"sts\":%u}",
                                 src, ch, rng.sts_quality);
                        ble_comms_notify_event(evt);
                    }
                }
            }
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        ch = (ch == UWB_CHANNEL_5) ? UWB_CHANNEL_9 : UWB_CHANNEL_5;
    }
    char evt[64];
    snprintf(evt, sizeof(evt), "{\"v\":\"track_done\",\"count\":%lu}",
             (unsigned long)count);
    ble_comms_notify_event(evt);
    vTaskDelete(NULL);
}

/* ---- Mode enter / exit ---------------------------------- */

static void enter_mode(app_mode_t m)
{
    /* TX-requiring modes need an authorised target. */
    bool needs_target = (m == MODE_TWR || m == MODE_RELAY ||
                         m == MODE_STS_AUDIT);
    if (needs_target && !ble_comms_target_loaded()) {
        ble_comms_notify_event("{\"v\":\"err\",\"why\":\"no_target\"}");
        ESP_LOGW(TAG, "refusing TX mode %s: no authorised target", MODE_NAMES[m]);
        return;
    }

    s_mode = m;
    ESP_LOGI(TAG, "entering mode %s", MODE_NAMES[m]);

    switch (m) {
    case MODE_TWR:
        dw3110_configure_channel(&s_uwb_cfg);
        xTaskCreate(twr_task, "twr", 4096, NULL, 13, NULL);
        break;
    case MODE_SNIFF:
        sniff_start(s_uwb_cfg.channel, sniff_cb, NULL);
        break;
    case MODE_STS_AUDIT:
        /* STS audit is driven from the companion app; no task here. */
        break;
    case MODE_RELAY:
        relay_init(&s_relay);
        relay_arm(&s_relay);
        break;
    case MODE_TRACK:
        xTaskCreate(track_task, "track", 4096, NULL, 12, NULL);
        break;
    case MODE_IDLE:
    default:
        break;
    }
}

static void exit_mode(void)
{
    ESP_LOGI(TAG, "exiting mode %s", MODE_NAMES[s_mode]);
    switch (s_mode) {
    case MODE_SNIFF:    sniff_stop(); break;
    case MODE_RELAY:    relay_disarm(&s_relay); break;
    default: break;
    }
    s_mode = MODE_IDLE;
}

/* ---- UI task ------------------------------------------- */

static void ui_task(void *arg)
{
    (void)arg;
    uint8_t btn;
    int battery = 100;
    int rssi = -70;
    double dist = -1.0;

    while (true) {
        /* Button handling */
        if (xQueueReceive(s_btn_q, &btn, pdMS_TO_TICKS(100)) == pdPASS) {
            vTaskDelay(pdMS_TO_TICKS(BTN_DEBOUNCE_MS));
            if (btn == BTN_MODE_GPIO) {
                if (s_mode != MODE_IDLE) exit_mode();
                else                     enter_mode((app_mode_t)((s_mode + 1) % MODE_COUNT));
                /* advance on next press */
                static int next = 0;
                next = (next + 1) % MODE_COUNT;
                if (s_mode == MODE_IDLE) enter_mode((app_mode_t)next);
            } else if (btn == BTN_UP_GPIO) {
                s_uwb_cfg.channel =
                    (s_uwb_cfg.channel == UWB_CHANNEL_5) ? UWB_CHANNEL_9
                                                         : UWB_CHANNEL_5;
                if (s_mode != MODE_IDLE) dw3110_configure_channel(&s_uwb_cfg);
            } else if (btn == BTN_DOWN_GPIO) {
                /* Toggle STS on/off for quick testing. */
                if (s_uwb_cfg.sts_mode == UWB_STS_OFF)
                    s_uwb_cfg.sts_mode = UWB_STS_DYNAMIC;
                else
                    s_uwb_cfg.sts_mode = UWB_STS_OFF;
                dw3110_set_sts_mode((uwb_sts_mode_t)s_uwb_cfg.sts_mode);
            }
        }

        /* Periodic battery + display refresh */
        fuel_gauge_percent(&battery);
        display_status(MODE_NAMES[s_mode],
                       ble_comms_target_loaded() ? "AUTH OK" : "NO AUTH TARGET",
                       battery, rssi, dist);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

/* ---- Console task (USB-CDC) ---------------------------- */

static void console_task(void *arg)
{
    (void)arg;
    char line[128];
    while (true) {
        fgets(line, sizeof(line), stdin);
        /* Remove trailing newline. */
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == 0) continue;
        ESP_LOGI(TAG, "console: %s", line);

        if (strncmp(line, "mode ", 5) == 0) {
            const char *m = line + 5;
            exit_mode();
            for (int i = 0; i < MODE_COUNT; i++) {
                if (strcasecmp(m, MODE_NAMES[i]) == 0) {
                    enter_mode((app_mode_t)i);
                    break;
                }
            }
        } else if (strncmp(line, "channel ", 8) == 0) {
            s_uwb_cfg.channel = (uint8_t)atoi(line + 8);
            if (s_mode != MODE_IDLE) dw3110_configure_channel(&s_uwb_cfg);
        } else if (strncmp(line, "sts ", 4) == 0) {
            const char *s = line + 4;
            if (strncmp(s, "off", 3) == 0)        s_uwb_cfg.sts_mode = UWB_STS_OFF;
            else if (strncmp(s, "static", 6) == 0)  s_uwb_cfg.sts_mode = UWB_STS_STATIC;
            else if (strncmp(s, "dynamic", 7) == 0) s_uwb_cfg.sts_mode = UWB_STS_DYNAMIC;
            else if (strncmp(s, "advanced", 8) == 0) s_uwb_cfg.sts_mode = UWB_STS_ADVANCED;
            dw3110_set_sts_mode((uwb_sts_mode_t)s_uwb_cfg.sts_mode);
        } else if (strncmp(line, "relay arm", 9) == 0) {
            /* "relay arm 0.05" arms in SHRINK mode targeting 5 cm. */
            double cm = 0.0;
            if (strlen(line) > 10) cm = atof(line + 10);
            s_relay.mode = (cm > 0) ? RELAY_SHRINK : RELAY_TRANSPARENT;
            /* convert cm -> DTU one-way */
            s_relay.target_shrink_dtu =
                (uint32_t)((cm / 100.0) / UWB_M_PER_DTU_ONEWAY);
            if (s_mode != MODE_RELAY) {
                exit_mode();
                enter_mode(MODE_RELAY);
            } else {
                relay_disarm(&s_relay);
                relay_arm(&s_relay);
            }
        } else if (strncmp(line, "relay disarm", 12) == 0) {
            relay_disarm(&s_relay);
        } else if (strncmp(line, "track scan", 10) == 0) {
            exit_mode();
            enter_mode(MODE_TRACK);
        } else if (strncmp(line, "dump", 4) == 0) {
            sniff_pcap_close();
        } else if (strncmp(line, "help", 4) == 0) {
            printf("Commands: mode <name>, channel <5|9>, sts <off|static|dynamic|advanced>,\n");
            printf("          relay arm [cm], relay disarm, track scan, dump, help\n");
        }
    }
}

/* ---- app_main ----------------------------------------- */

void app_main(void)
{
    ESP_LOGI(TAG, "=== UWB-PHANTOM v%s (author: jayis1) ===",
             BOARD_FW_VERSION_STRING);
    ESP_LOGI(TAG, "LEGAL: authorised security research only.");

    /* NVS for BLE bonding */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    /* Install GPIO ISR service for buttons */
    gpio_install_isr_service(0);

    /* Peripherals */
    display_init();
    fuel_gauge_init();
    ble_comms_init();
    ble_comms_advertise(true);

    /* UWB */
    uwb_config_defaults(&s_uwb_cfg);
    int rc = dw3110_init(&s_uwb_cfg);
    if (rc != 0) {
        ESP_LOGE(TAG, "DW3110 init failed rc=%d — UWB features disabled", rc);
        display_status("UWB FAIL", "DW3110 not found", 100, 0, -1);
    }

    /* UI + console */
    buttons_init();
    xTaskCreate(ui_task,      "ui",      4096, NULL, 10, NULL);
    xTaskCreate(console_task, "console", 4096, NULL,  9, NULL);

    ESP_LOGI(TAG, "ready.  Mode = %s.  Use buttons or console.",
             MODE_NAMES[s_mode]);
}