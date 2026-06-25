/*
 * uwb_sniffer.c — UWB passive sniffer with PCAP export.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * The sniffer never transmits: it puts the DW3110 into permanent RX
 * with auto-re-enable and forwards each frame to a callback.  When a
 * PCAP file is open, frames are also written to SD with a link type
 * compatible with Wireshark's 802.15.4 dissector plus a small TLV that
 * records UWB-PHY metadata not present in vanilla 802.15.4 PCAPs.
 */

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/sdspi.h"
#include "esp_vfs_fat.h"

#include "board.h"
#include "registers.h"
#include "dw3110.h"
#include "uwb_sniffer.h"

static const char *TAG = "sniffer";

static TaskHandle_t      s_task;
static QueueHandle_t     s_q;
static sniff_cb_t        s_cb;
static void             *s_cb_user;
static uint8_t           s_channel;
static bool               s_running;
static FILE             *s_pcap;
static uint32_t          s_cap_count;

/* PCAP header — IEEE 802.15.4 with TAP / per-frame metadata TLV.
 * We use link type 195 (LINKTYPE_IEEE802_15_4) which Wireshark
 * understands, and prepend a vendor TLV block in the frame payload
 * that carries UWB-specific info. */
#pragma pack(push, 1)
typedef struct {
    uint32_t magic;          /* 0xA1B2C3D4 */
    uint16_t ver_major;      /* 2 */
    uint16_t ver_minor;      /* 4 */
    int32_t  thiszone;       /* 0 */
    uint32_t sigfigs;        /* 0 */
    uint32_t snaplen;        /* 65535 */
    uint32_t linktype;       /* 195 */
} pcap_hdr_t;

typedef struct {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
} pcap_rec_t;

/* Vendor TLV prepended to each frame (length is 8 + 12 = 20 bytes). */
typedef struct {
    uint16_t tlv_type;        /* 0x00FB = vendor UWB metadata */
    uint16_t tlv_len;        /* 12 */
    uint32_t channel;
    int8_t   rssi_dbm;
    uint8_t  sts_quality;
    uint8_t  _pad[2];
    uint32_t rx_dtu_lo;
} uwb_tlv_t;
#pragma pack(pop)

static void sniffer_task(void *arg)
{
    dw3110_rx_start(false, 0);
    while (s_running) {
        uint32_t status;
        if (dw3110_read_sys_status(&status) == 0 && (status & SYS_STATUS_RXDFR)) {
            sniff_frame_t sf;
            uwb_ranging_t rng;
            size_t len = 0;
            if (dw3110_rx_read(sf.frame, SNIFF_MAX_FRAME, &len, &rng) == 0 && len > 0) {
                sf.frame_len   = (uint16_t)len;
                sf.rx_dtu      = rng.rx_stamp;
                sf.channel     = s_channel;
                sf.rssi_dbm    = -70; /* placeholder: DW3000 RSSI calc */
                sf.sts_quality = rng.sts_quality;
                if (s_cb) s_cb(&sf, s_cb_user);
                if (s_pcap) sniff_pcap_write(&sf);
                s_cap_count++;
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    vTaskDelete(NULL);
}

int sniff_start(uint8_t channel, sniff_cb_t cb, void *user)
{
    if (s_running) return -EALREADY;
    s_channel = channel;
    s_cb      = cb;
    s_cb_user = user;
    s_running = true;
    s_cap_count = 0;
    s_q = xQueueCreate(8, sizeof(sniff_frame_t));
    xTaskCreate(sniffer_task, "sniff", 4096, NULL, 12, &s_task);
    ESP_LOGI(TAG, "sniffer started on channel %u", channel);
    return 0;
}

void sniff_stop(void)
{
    s_running = false;
    vTaskDelay(pdMS_TO_TICKS(50));
    if (s_q) vQueueDelete(s_q);
    s_q = NULL;
    ESP_LOGI(TAG, "sniffer stopped, %lu frames captured",
             (unsigned long)s_cap_count);
}

int sniff_pcap_open(const char *path)
{
    if (s_pcap) return -EALREADY;
    s_pcap = fopen(path, "wb");
    if (!s_pcap) { ESP_LOGE(TAG, "pcap open failed: %s", path); return -errno; }
    pcap_hdr_t h = {
        .magic = 0xA1B2C3D4, .ver_major = 2, .ver_minor = 4,
        .thiszone = 0, .sigfigs = 0, .snaplen = 65535, .linktype = 195,
    };
    fwrite(&h, sizeof(h), 1, s_pcap);
    ESP_LOGI(TAG, "pcap open: %s", path);
    return 0;
}

int sniff_pcap_write(const sniff_frame_t *f)
{
    if (!s_pcap) return -EINVAL;
    int64_t us = esp_timer_get_time();
    pcap_rec_t r;
    r.ts_sec   = (uint32_t)(us / 1000000);
    r.ts_usec  = (uint32_t)(us % 1000000);
    r.orig_len = f->frame_len + sizeof(uwb_tlv_t);
    r.incl_len = r.orig_len;

    uwb_tlv_t tlv = {
        .tlv_type = 0x00FB, .tlv_len = 12,
        .channel = f->channel,
        .rssi_dbm = f->rssi_dbm,
        .sts_quality = f->sts_quality,
        .rx_dtu_lo = (uint32_t)(f->rx_dtu & 0xFFFFFFFFu),
    };

    fwrite(&r,   sizeof(r),   1, s_pcap);
    fwrite(&tlv, sizeof(tlv), 1, s_pcap);
    fwrite(f->frame, f->frame_len, 1, s_pcap);
    fflush(s_pcap);
    return 0;
}

int sniff_pcap_close(void)
{
    if (!s_pcap) return -EINVAL;
    fclose(s_pcap);
    s_pcap = NULL;
    ESP_LOGI(TAG, "pcap closed");
    return 0;
}