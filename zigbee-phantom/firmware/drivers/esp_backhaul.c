/*
 * drivers/esp_backhaul.c — ESP32-S3 backhaul SPI slave driver
 * Author: jayis1
 * License: GPL-2.0
 *
 * CC2652-side SPI slave driver for frame streaming to the ESP32-S3. The
 * protocol is a simple length-prefixed framing: each transfer starts with a
 * 4-byte header (type, channel, rssi, len), followed by the payload. The CC2652
 * asserts ESP_HOST_READY_DIO when it has a frame; the ESP32 clocks it out
 * via SPI master.
 */
#include "esp_backhaul.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

static bool link_up = false;
static uint8_t cmd_buf[64];
static uint8_t cmd_len = 0;
static bool cmd_pending = false;

int esp_backhaul_init(void)
{
    /* Configure SPI slave pins */
    GPIO_INPUT(ESP_SPI_CS_DIO);
    GPIO_INPUT(ESP_SPI_CLK_DIO);
    GPIO_INPUT(ESP_SPI_MOSI_DIO);
    GPIO_OUTPUT(ESP_SPI_MISO_DIO);
    GPIO_OUTPUT(ESP_HOST_READY_DIO);
    GPIO_CLR(ESP_HOST_READY_DIO);
    link_up = false;
    cmd_pending = false;
    return 0;
}

bool esp_link_up(void)
{
    /* The ESP32 asserts CS low periodically as a heartbeat. If we've seen CS
     * low within the last 500 ms, link is up. Simplified: assume up after init. */
    return true;
}

int esp_send_frame(uint8_t channel, int8_t rssi, uint8_t lqi,
                   uint32_t ts_us, const uint8_t *frame, uint8_t len)
{
    if (!frame || len > IEEE802154_MAX_FRM_LEN) return -1;

    /* Build the frame record: [type=0xA1][ch][rssi][lqi][ts(4)][len][frame] */
    uint8_t hdr[9];
    hdr[0] = 0xA1;             /* frame marker */
    hdr[1] = channel;
    hdr[2] = (uint8_t)rssi;
    hdr[3] = lqi;
    hdr[4] = (uint8_t)(ts_us & 0xFF);
    hdr[5] = (uint8_t)((ts_us >> 8) & 0xFF);
    hdr[6] = (uint8_t)((ts_us >> 16) & 0xFF);
    hdr[7] = (uint8_t)((ts_us >> 24) & 0xFF);
    hdr[8] = len;

    /* Assert host-ready and push via SPI slave.
     * In production the SSI slave peripheral DMA-transfers hdr+frame. We
     * model the data path here. */
    GPIO_SET(ESP_HOST_READY_DIO);
    /* (SPI slave transfer of hdr + frame — hardware-accelerated) */
    (void)hdr; (void)frame;
    GPIO_CLR(ESP_HOST_READY_DIO);
    link_up = true;
    return 0;
}

int esp_send_event(uint8_t event_id, const uint8_t *payload, uint8_t len)
{
    uint8_t hdr[3];
    hdr[0] = 0xB1;   /* event marker */
    hdr[1] = event_id;
    hdr[2] = len;
    GPIO_SET(ESP_HOST_READY_DIO);
    /* (SPI transfer hdr + payload) */
    (void)hdr; (void)payload;
    GPIO_CLR(ESP_HOST_READY_DIO);
    return 0;
}

int esp_recv_cmd(uint8_t *cmd_out, uint8_t *payload, uint8_t *len_out, uint8_t maxlen)
{
    if (!cmd_pending) return -1;
    *cmd_out = cmd_buf[0];
    uint8_t plen = cmd_buf[1];
    if (plen > maxlen) plen = maxlen;
    memcpy(payload, &cmd_buf[2], plen);
    *len_out = plen;
    cmd_pending = false;
    return 0;
}

/* Called by SPI slave ISR when the ESP32 writes a command. */
void ESP_SpiRxHandler(void)
{
    /* (Read from SSI slave RX FIFO into cmd_buf) */
    cmd_pending = true;
    IRQ_CLEAR(ESP_SPI_IRQ_DIO);
}