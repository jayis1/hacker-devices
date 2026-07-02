/*
 * ble_link.c — BLE 5.0 rendezvous protocol
 * Device: Aurora-Phantom   Author: jayis1   License: GPL-2.0
 *
 * BLE is only enabled in MODE_RENDEZVOUS to preserve RF silence during
 * capture. The protocol is a simple GATT service with two characteristics:
 *   CMD  (write) : app -> device commands (mission JSON, mode, settings)
 *   DATA (notify): device -> app frame stream + header/trailer markers
 *
 * Frame packets: a 3-byte header [0xAA][seq_lo][seq_hi] followed by up to
 * 509 bytes of frame magnitude data (MTU 512). Header/trailer use sentinel
 * byte 0xA5.
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../board.h"
#include "ble_link.h"

__attribute__((weak)) int ble_stack_start(const char *name) { (void)name; return 0; }
__attribute__((weak)) void ble_stack_stop(void) {}
__attribute__((weak)) bool ble_stack_connected(void) { return false; }
__attribute__((weak)) int ble_notify(const uint8_t *data, uint32_t len) { (void)data; (void)len; return (int)len; }

static ble_cmd_cb_t g_cmd_cb = NULL;
static uint16_t g_seq = 0;

int ble_link_start(void)
{
    return ble_stack_start(BLE_DEVICE_NAME);
}

void ble_link_stop(void)
{
    ble_stack_stop();
}

bool ble_link_is_connected(void)
{
    return ble_stack_connected();
}

void ble_link_set_cmd_callback(ble_cmd_cb_t cb) { g_cmd_cb = cb; }

static void send_packet(uint8_t kind, const uint8_t *payload, uint32_t len)
{
    uint8_t pkt[BLE_MTU];
    if (len > BLE_MTU - 4) len = BLE_MTU - 4;
    pkt[0] = kind;                 /* 0xAA data, 0xA5 marker */
    pkt[1] = (uint8_t)(g_seq & 0xFF);
    pkt[2] = (uint8_t)(g_seq >> 8);
    if (len && payload) memcpy(pkt + 3, payload, len);
    ble_notify(pkt, len + 3);
    g_seq++;
}

void ble_link_send_header(uint16_t total_frames)
{
    uint8_t p[2] = { (uint8_t)(total_frames & 0xFF), (uint8_t)(total_frames >> 8) };
    send_packet(0xA5, p, 2);       /* header marker */
    g_seq = 0;
}

void ble_link_send_frame(const uint16_t *words, uint32_t n)
{
    /* Pack 16-bit mags into bytes (drop low byte for preview bandwidth). */
    uint8_t buf[BLE_MTU - 4];
    uint32_t out = 0;
    for (uint32_t i = 0; i < n && out < sizeof(buf); i++)
        buf[out++] = (uint8_t)(words[i] >> 8);
    send_packet(0xAA, buf, out);
}

void ble_link_send_trailer(void)
{
    send_packet(0xA5, NULL, 0);
}