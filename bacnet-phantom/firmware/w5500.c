/*
 * w5500.c — Wiznet W5500 driver for BACnet/IP transport.
 *
 * Author: jayis1
 * License: GPLv3
 *
 * The W5500 is a hard-wired TCP/IP stack with 8 sockets. We use three:
 *   socket 0 — raw promiscuous mode for passive sniffing (BACnet/IP UDP/47808)
 *   socket 1 — active TX socket (Who-Is, ReadPropertyMultiple, WriteProperty)
 *   socket 2 — BBMD foreign-device registration socket
 *
 * The driver is deliberately minimal: it hides the SPI framing, exposes
 * board_w5500_send_raw / recv_raw to the rest of the firmware, and never
 * emits on its own — emission is gated by bp_state_t.mode in board.c.
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "board.h"
#include "registers.h"

static const char *TAG = "w5500";

/* W5500 common register offsets (selected) */
#define W5500_REG_MR            0x0000  /* mode */
#define W5500_REG_GAR0          0x0001  /* gateway */
#define W5500_REG_SUBR0         0x0005  /* subnet */
#define W5500_REG_SHAR0         0x0009  /* MAC */
#define W5500_REG_SIPR0         0x000F  /* IP */
#define W5500_REG_PHYCFGR       0x002E  /* PHY config */
#define W5500_REG_VERSIONR      0x0039  /* silicon rev (expect 0x04) */

/* Per-socket registers (block 0x0n for socket n) */
#define W5500_SREG_MR           0x0000  /* socket mode */
#define W5500_SREG_CR           0x0001  /* control */
#define W5500_SREG_IR           0x0002  /* interrupt */
#define W5500_SREG_SR           0x0003  /* status */
#define W5500_SREG_PORT0        0x0004  /* source port */
#define W5500_SREG_DIPR0        0x000C  /* destination IP */
#define W5500_SREG_DPORT0       0x0010  /* destination port */
#define W5500_SREG_TX_FSR0      0x0020  /* TX free size */
#define W5500_SREG_TX_RD0       0x0024  /* TX read pointer */
#define W5500_SREG_TX_WR0       0x0026  /* TX write pointer */
#define W5500_SREG_RX_RSR0      0x0026  /* RX received size */
#define W5500_SREG_RX_RD0       0x0028  /* RX read pointer */

/* Socket mode constants */
#define W5500_MR_MACRAW         0x04
#define W5500_MR_UDP            0x02
#define W5500_MR_IPRAW          0x03
#define W5500_CR_OPEN           0x01
#define W5500_CR_CLOSE         0x10
#define W5500_CR_SEND          0x20
#define W5500_CR_SEND_MAC      0x20
#define W5500_CR_RECV          0x40

/* BSSR block select bits */
#define W5500_CTRL_BLOCK_REG   0x00
#define W5500_CTRL_BLOCK_TX    0x10
#define W5500_CTRL_BLOCK_RX    0x18

static spi_device_handle_t s_spi;

/* SPI frame: 3-byte header (offset[16] + control[8]) then data */
static void w5500_read(uint16_t addr, uint8_t ctrl, uint8_t *dst, size_t n)
{
    uint8_t hdr[3] = { (addr >> 8) & 0xFF, addr & 0xFF, ctrl };
    spi_transaction_t t = { 0 };
    t.length = 24;
    t.tx_buffer = hdr;
    t.rx_buffer = NULL;
    spi_device_polling_transmit(s_spi, &t);

    t.length = n * 8;
    t.tx_buffer = NULL;
    t.rx_buffer = dst;
    spi_device_polling_transmit(s_spi, &t);
}

static void w5500_write(uint16_t addr, uint8_t ctrl, const uint8_t *src, size_t n)
{
    uint8_t hdr[3] = { (addr >> 8) & 0xFF, addr & 0xFF, ctrl | 0x04 };
    spi_transaction_t t = { 0 };
    t.length = 24;
    t.tx_buffer = hdr;
    t.rx_buffer = NULL;
    spi_device_polling_transmit(s_spi, &t);

    t.length = n * 8;
    t.tx_buffer = src;
    t.rx_buffer = NULL;
    spi_device_polling_transmit(s_spi, &t);
}

static uint8_t w5500_r8(uint16_t addr, uint8_t ctrl)
{
    uint8_t v;
    w5500_read(addr, ctrl, &v, 1);
    return v;
}

static void w5500_w8(uint16_t addr, uint8_t ctrl, uint8_t v)
{
    w5500_write(addr, ctrl, &v, 1);
}

static uint16_t w5500_r16(uint16_t addr, uint8_t ctrl)
{
    uint8_t b[2];
    w5500_read(addr, ctrl, b, 2);
    return ((uint16_t)b[0] << 8) | b[1];
}

static void w5500_w16(uint16_t addr, uint8_t ctrl, uint16_t v)
{
    uint8_t b[2] = { (v >> 8) & 0xFF, v & 0xFF };
    w5500_write(addr, ctrl, b, 2);
}

static void w5500_sreg_w8(uint8_t sock, uint16_t reg, uint8_t v)
{
    uint16_t addr = (sock << 8) | reg;     /* block offset */
    /* For socket registers the block select is 0x01 (control) | (sock<<5) */
    uint8_t ctrl = 0x01 | (sock << 5);
    w5500_w8(addr, ctrl, v);
}

static uint8_t w5500_sreg_r8(uint8_t sock, uint16_t reg)
{
    uint16_t addr = (sock << 8) | reg;
    uint8_t ctrl = 0x00 | (sock << 5);
    return w5500_r8(addr, ctrl);
}

static void w5500_sreg_w16(uint8_t sock, uint16_t reg, uint16_t v)
{
    uint16_t addr = (sock << 8) | reg;
    uint8_t ctrl = 0x01 | (sock << 5);
    w5500_w16(addr, ctrl, v);
}

static uint16_t w5500_sreg_r16(uint8_t sock, uint16_t reg)
{
    uint16_t addr = (sock << 8) | reg;
    uint8_t ctrl = 0x00 | (sock << 5);
    return w5500_r16(addr, ctrl);
}

esp_err_t board_w5500_init(void)
{
    /* Configure RST and INT pins */
    gpio_config_t io = { .pin_bit_mask = (1ULL << BP_W5500_PIN_RST) |
                                    (1ULL << BP_W5500_PIN_CS),
                         .mode = GPIO_MODE_OUTPUT };
    gpio_config(&io);
    gpio_set_level(BP_W5500_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(BP_W5500_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    spi_bus_config_t bus = { .mosi_io_num = BP_W5500_PIN_MOSI,
                             .miso_io_num = BP_W5500_PIN_MISO,
                             .sclk_io_num = BP_W5500_PIN_SCLK,
                             .quadhd_io_num = -1, .quadwp_io_num = -1,
                             .max_transfer_sz = BP_NPDU_MAX + 16 };
    spi_bus_initialize(BP_W5500_SPI_HOST, &bus, SPI_DMA_CH_AUTO);
    spi_device_interface_config_t dev = { .clock_speed_hz = BP_W5500_SPI_HZ,
                                          .mode = 0,
                                          .spics_io_num = BP_W5500_PIN_CS,
                                          .queue_size = 4 };
    spi_bus_add_device(BP_W5500_SPI_HOST, &dev, &s_spi);

    uint8_t ver = w5500_r8(W5500_REG_VERSIONR, W5500_CTRL_BLOCK_REG);
    if (ver != 0x04) {
        ESP_LOGE(TAG, "unexpected silicon rev 0x%02x (want 0x04)", ver);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "W5500 silicon rev 0x%02x — init ok", ver);

    /* Randomise MAC from PSRAM seed (board.c seeds RNG). */
    extern uint8_t bp_mac[6];
    w5500_write(W5500_REG_SHAR0, W5500_CTRL_BLOCK_REG, bp_mac, 6);

    /* Configure socket 0 in MACRAW mode for passive sniffing.
     * Socket 1 (active TX) is opened lazily by board_w5500_send_raw. */
    w5500_sreg_w8(0, W5500_SREG_MR, W5500_MR_MACRAW);
    w5500_sreg_w8(0, W5500_SREG_CR, W5500_CR_OPEN);
    return ESP_OK;
}

/* Send a raw UDP/0xBAC0 frame on socket 1 (active TX). */
esp_err_t board_w5500_send_raw(const uint8_t *frame, size_t len)
{
    if (len == 0 || len > BP_NPDU_MAX) return ESP_ERR_INVALID_ARG;

    /* Open socket 1 as UDP on port 0xBAC0 if not already open. */
    if (w5500_sreg_r8(1, W5500_SREG_SR) != 0x22) {  /* 0x22 = UDP */
        w5500_sreg_w8(1, W5500_SREG_MR, W5500_MR_UDP);
        w5500_sreg_w16(1, W5500_SREG_PORT0, 0xBAC0);
        w5500_sreg_w8(1, W5500_SREG_CR, W5500_CR_OPEN);
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    /* Broadcast destination 255.255.255.255 : 0xBAC0 */
    uint8_t dip[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    w5500_write((1 << 8) | W5500_SREG_DIPR0, 0x01 | (1 << 5), dip, 4);
    w5500_sreg_w16(1, W5500_SREG_DPORT0, 0xBAC0);

    /* Write to socket 1 TX buffer (block 0x10 + socket<<5) */
    uint16_t wr = w5500_sreg_r16(1, W5500_SREG_TX_WR0);
    uint8_t ctrl = 0x10 | (1 << 5);
    w5500_write(wr, ctrl, frame, len);
    w5500_sreg_w16(1, W5500_SREG_TX_WR0, wr + (uint16_t)len);
    w5500_sreg_w8(1, W5500_SREG_CR, W5500_CR_SEND);

    /* Wait for SEND_OK (IR bit 0x10) */
    int wait = 0;
    while ((w5500_sreg_r8(1, W5500_SREG_IR) & 0x10) == 0 && wait < 50) {
        vTaskDelay(pdMS_TO_TICKS(1));
        wait++;
    }
    w5500_sreg_w8(1, W5500_SREG_IR, 0x10);
    return (wait < 50) ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t board_w5500_recv_raw(uint8_t *buf, size_t cap, size_t *out_len,
                                int timeout_ms)
{
    /* Read from socket 0 in MACRAW mode: first two bytes are length. */
    uint16_t rsr = w5500_sreg_r16(0, W5500_SREG_RX_RSR0);
    if (rsr < 2) {
        vTaskDelay(pdMS_TO_TICKS(timeout_ms > 0 ? timeout_ms : 1));
        *out_len = 0;
        return ESP_OK;
    }
    uint16_t rd = w5500_sreg_r16(0, W5500_SREG_RX_RD0);
    uint8_t ctrl = 0x18 | (0 << 5);   /* RX block, socket 0 */
    uint8_t lenb[2];
    w5500_read(rd, ctrl, lenb, 2);
    uint16_t flen = ((uint16_t)lenb[0] << 8) | lenb[1];
    if (flen > cap) flen = (uint16_t)cap;
    w5500_read(rd + 2, ctrl, buf, flen);
    w5500_sreg_w16(0, W5500_SREG_RX_RD0, rd + flen + 2);
    w5500_sreg_w8(0, W5500_SREG_CR, W5500_CR_RECV);
    *out_len = flen;
    return ESP_OK;
}