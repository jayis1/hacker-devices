/*
 * SILENT-SYMPHONY — Ultrasonic Covert Channel Communicator
 * nRF52840 BLE Bridge Driver — Implementation
 *
 * Manages UART protocol with nRF52840 BLE module.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "ble_bridge.h"
#include <string.h>

/* Global BLE bridge state */
ble_bridge_t g_ble = {0};

/* =========================================================================
 * CRC-16 for BLE packets (reuse from ultrasonic_driver)
 * ========================================================================= */
extern uint16_t crc16_ibm(const uint8_t *data, uint16_t len);

/* =========================================================================
 * USART5 Initialization (115200 8N1, FIFO enabled)
 * ========================================================================= */

static void usart5_gpio_init(void)
{
    /* Enable GPIOB clock */
    mmio_set_bits(RCC_AHB4ENR, RCC_AHB4ENR_GPIOB);

    uint32_t moder  = mmio_read32(GPIOB_BASE + GPIO_MODER);
    uint32_t afrl   = mmio_read32(GPIOB_BASE + GPIO_AFRL);
    uint32_t afrh   = mmio_read32(GPIOB_BASE + GPIO_AFRH);

    /* PB3 = RTS, PB4 = CTS, PB5 = RX, PB6 = TX.
     * All AF7. Let's set all four. */

    /* PB3 (RTS) */
    moder &= ~(3U << (3 * 2));
    moder |= (GPIO_MODE_AF << (3 * 2));
    afrl  &= ~(0xFUL << (3 * 4));
    afrl  |= (AF_USART5 << (3 * 4));

    /* PB4 (CTS) */
    moder &= ~(3U << (4 * 2));
    moder |= (GPIO_MODE_AF << (4 * 2));
    afrl  &= ~(0xFUL << (4 * 4));
    afrl  |= (AF_USART5 << (4 * 4));

    /* PB5 (RX) */
    moder &= ~(3U << (5 * 2));
    moder |= (GPIO_MODE_AF << (5 * 2));
    afrl  &= ~(0xFUL << (5 * 4));
    afrl  |= (AF_USART5 << (5 * 4));

    /* PB6 (TX) */
    moder &= ~(3U << (6 * 2));
    moder |= (GPIO_MODE_AF << (6 * 2));
    afrl  &= ~(0xFUL << (6 * 4));
    afrl  |= (AF_USART5 << (6 * 4));

    mmio_write32(GPIOB_BASE + GPIO_MODER, moder);
    mmio_write32(GPIOB_BASE + GPIO_AFRL, afrl);

    /* Set all to high speed */
    mmio_write32(GPIOB_BASE + GPIO_OSPEEDR, 0xFFFFFFFF);

    /* Pull-up on RX and CTS */
    uint32_t pupdr = mmio_read32(GPIOB_BASE + GPIO_PUPDR);
    pupdr |= (GPIO_PULL_UP << (5 * 2));  /* RX pull-up */
    pupdr |= (GPIO_PULL_UP << (4 * 2));  /* CTS pull-up */
    mmio_write32(GPIOB_BASE + GPIO_PUPDR, pupdr);
}

static void usart5_init(void)
{
    /* Enable USART5 clock (APB1L) */
    mmio_set_bits(RCC_APB1LENR, RCC_APB1LENR_USART5);

    /* Configure GPIO */
    usart5_gpio_init();

    /* Reset USART */
    mmio_write32(USART5_BASE + USART_CR1, 0);

    /* Configure: 115200 baud, 8N1, FIFO enabled */
    /* BRR = fCK / (16 * baud) = 120MHz / (16 * 115200) = 65.1, round to 65 */
    mmio_write32(USART5_BASE + USART_BRR, 65);

    /* CR1: UE=0 first, set params, then UE=1 */
    uint32_t cr1 = USART_CR1_FIFOEN | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    mmio_write32(USART5_BASE + USART_CR1, cr1);

    /* CR2: default (1 stop bit) */
    mmio_write32(USART5_BASE + USART_CR2, 0);

    /* CR3: enable RTS/CTS flow control */
    mmio_write32(USART5_BASE + USART_CR3, (1U << 9) | (1U << 8)); /* RTSE=1, CTSE=1 */

    /* Enable USART */
    mmio_set_bits(USART5_BASE + USART_CR1, USART_CR1_UE);
}

/* =========================================================================
 * BLE Bridge Implementation
 * ========================================================================= */

void ble_bridge_init(void)
{
    usart5_init();

    g_ble.initialized        = 1;
    g_ble.parse_state        = BLE_PARSE_START;
    g_ble.rx_index           = 0;
    g_ble.cmd_pending        = 0;
    g_ble.tx_len             = 0;
    g_ble.tx_busy            = 0;

    /* Default status */
    g_ble.status.battery_pct     = 100;
    g_ble.status.battery_mv      = 4200;
    g_ble.status.mode            = MODE_IDLE;
    g_ble.status.signal_quality  = 0;
    g_ble.status.storage_used    = 0;
    g_ble.status.storage_total   = QSPI_FLASH_SIZE;
    g_ble.status.tx_power        = TX_POWER_MED;
    g_ble.status.rx_gain         = RX_GAIN_MED;
    g_ble.status.uptime_s        = 0;
    g_ble.status.fw_version_major = 1;
    g_ble.status.fw_version_minor = 0;
    g_ble.status.fw_version_patch = 0;
}

void ble_bridge_process_rx(void)
{
    if (!g_ble.initialized) return;

    uint8_t byte;

    while (ble_uart_read(&byte)) {
        switch (g_ble.parse_state) {
        case BLE_PARSE_START:
            if (byte == BLE_START_BYTE) {
                g_ble.rx_buf[0] = byte;
                g_ble.rx_index = 1;
                g_ble.parse_state = BLE_PARSE_LENGTH;
            }
            break;

        case BLE_PARSE_LENGTH:
            g_ble.rx_buf[g_ble.rx_index++] = byte;
            if (g_ble.rx_index >= 3) { /* START + 2 bytes length */
                /* Parse length (LE) */
                g_ble.expected_length = (uint16_t)g_ble.rx_buf[1] | ((uint16_t)g_ble.rx_buf[2] << 8);
                if (g_ble.expected_length > BLE_MAX_PACKET_SIZE) {
                    /* Invalid length — reset */
                    g_ble.parse_state = BLE_PARSE_START;
                    g_ble.rx_index = 0;
                } else {
                    g_ble.parse_state = BLE_PARSE_CMD;
                }
            }
            break;

        case BLE_PARSE_CMD:
            g_ble.rx_buf[g_ble.rx_index++] = byte;
            if (g_ble.rx_index >= 5) { /* START + LEN(2) + CMD(2) */
                g_ble.expected_cmd = (uint16_t)g_ble.rx_buf[3] | ((uint16_t)g_ble.rx_buf[4] << 8);
                g_ble.expected_payload_len = g_ble.expected_length - 4; /* minus cmd(2) + crc(2) */
                if (g_ble.expected_payload_len > BLE_MAX_PAYLOAD) {
                    g_ble.parse_state = BLE_PARSE_START;
                    g_ble.rx_index = 0;
                } else if (g_ble.expected_payload_len == 0) {
                    /* No payload, go straight to CRC */
                    g_ble.parse_state = BLE_PARSE_CRC;
                } else {
                    g_ble.parse_state = BLE_PARSE_PAYLOAD;
                }
            }
            break;

        case BLE_PARSE_PAYLOAD:
            g_ble.rx_buf[g_ble.rx_index++] = byte;
            if (g_ble.rx_index >= (uint16_t)(5 + g_ble.expected_payload_len)) {
                g_ble.parse_state = BLE_PARSE_CRC;
            }
            break;

        case BLE_PARSE_CRC:
            g_ble.rx_buf[g_ble.rx_index++] = byte;
            if (g_ble.rx_index >= (uint16_t)(5 + g_ble.expected_payload_len + BLE_CRC_SIZE)) {
                /* Complete packet received — verify CRC */
                uint16_t received_crc = (uint16_t)g_ble.rx_buf[g_ble.rx_index - 2]
                                      | ((uint16_t)g_ble.rx_buf[g_ble.rx_index - 1] << 8);
                uint16_t computed_crc = crc16_ibm(g_ble.rx_buf, g_ble.rx_index - 2);

                if (computed_crc == received_crc) {
                    /* Valid packet — copy to pending */
                    g_ble.pending_cmd = g_ble.expected_cmd;
                    g_ble.pending_payload_len = g_ble.expected_payload_len;
                    if (g_ble.pending_payload_len > 0) {
                        memcpy(g_ble.pending_payload,
                               &g_ble.rx_buf[5], g_ble.pending_payload_len);
                    }
                    g_ble.cmd_pending = 1;
                }

                /* Reset parser */
                g_ble.parse_state = BLE_PARSE_START;
                g_ble.rx_index = 0;
            }
            break;
        }
    }
}

int ble_bridge_send(uint16_t cmd, const uint8_t *payload, uint16_t len)
{
    if (!g_ble.initialized) return ERR_NOT_INIT;
    if (len > BLE_MAX_PAYLOAD) return ERR_INVALID_PARAM;

    uint8_t buf[BLE_MAX_PACKET_SIZE];
    uint16_t idx = 0;

    /* START byte */
    buf[idx++] = BLE_START_BYTE;

    /* Length (LE): cmd(2) + payload + crc(2) */
    uint16_t pkt_len = 2 + len + 2;
    buf[idx++] = pkt_len & 0xFF;
    buf[idx++] = (pkt_len >> 8) & 0xFF;

    /* Command ID (LE) */
    buf[idx++] = cmd & 0xFF;
    buf[idx++] = (cmd >> 8) & 0xFF;

    /* Payload */
    if (len > 0) {
        memcpy(&buf[idx], payload, len);
        idx += len;
    }

    /* CRC-16 (LE) */
    uint16_t crc = crc16_ibm(buf, idx);
    buf[idx++] = crc & 0xFF;
    buf[idx++] = (crc >> 8) & 0xFF;

    /* Send */
    ble_uart_write(buf, idx);

    return ERR_OK;
}

int ble_bridge_respond(uint16_t cmd, const uint8_t *payload, uint16_t len)
{
    return ble_bridge_send(cmd, payload, len);
}

int ble_bridge_cmd_pending(void)
{
    return g_ble.cmd_pending ? 1 : 0;
}

int ble_bridge_get_cmd(uint16_t *out_cmd, uint8_t *out_payload, uint16_t *out_len)
{
    if (!g_ble.cmd_pending) return ERR_NO_DATA;
    if (!out_cmd || !out_payload || !out_len) return ERR_INVALID_PARAM;

    *out_cmd = g_ble.pending_cmd;
    *out_len = g_ble.pending_payload_len;
    memcpy(out_payload, g_ble.pending_payload, g_ble.pending_payload_len);

    return ERR_OK;
}

void ble_bridge_cmd_consumed(void)
{
    g_ble.cmd_pending = 0;
    g_ble.pending_cmd = 0;
    g_ble.pending_payload_len = 0;
}

void ble_bridge_update_status(void)
{
    /* This is called periodically from main loop.
     * In production, battery voltage would be read from ADC,
     * storage used from QSPI filesystem, etc. */
    
    /* Placeholder: battery ADC would be on STM32H7 internal ADC or
     * TP4056 STAT pin monitoring via GPIO */
    g_ble.status.uptime_s++; /* Incremented every second from SysTick */
}

int ble_bridge_send_status(void)
{
    uint8_t payload[20];
    uint16_t idx = 0;

    payload[idx++] = g_ble.status.battery_pct;
    payload[idx++] = g_ble.status.battery_mv & 0xFF;
    payload[idx++] = (g_ble.status.battery_mv >> 8) & 0xFF;
    payload[idx++] = g_ble.status.mode;
    payload[idx++] = g_ble.status.signal_quality;
    payload[idx++] = g_ble.status.storage_used & 0xFF;
    payload[idx++] = (g_ble.status.storage_used >> 8) & 0xFF;
    payload[idx++] = (g_ble.status.storage_used >> 16) & 0xFF;
    payload[idx++] = (g_ble.status.storage_used >> 24) & 0xFF;
    payload[idx++] = g_ble.status.storage_total & 0xFF;
    payload[idx++] = (g_ble.status.storage_total >> 8) & 0xFF;
    payload[idx++] = (g_ble.status.storage_total >> 16) & 0xFF;
    payload[idx++] = (g_ble.status.storage_total >> 24) & 0xFF;
    payload[idx++] = g_ble.status.tx_power;
    payload[idx++] = g_ble.status.rx_gain;
    payload[idx++] = g_ble.status.uptime_s & 0xFF;
    payload[idx++] = (g_ble.status.uptime_s >> 8) & 0xFF;
    payload[idx++] = (g_ble.status.uptime_s >> 16) & 0xFF;
    payload[idx++] = (g_ble.status.uptime_s >> 24) & 0xFF;
    payload[idx++] = g_ble.status.fw_version_major;
    payload[idx++] = g_ble.status.fw_version_minor;
    payload[idx++] = g_ble.status.fw_version_patch;

    return ble_bridge_send(BLE_CMD_STATUS_RESP, payload, idx);
}