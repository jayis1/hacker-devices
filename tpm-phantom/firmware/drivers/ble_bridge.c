/*
 * tpm-phantom — drivers/ble_bridge.c
 * UART bridge to the nRF52840 BLE co-processor.
 *
 * The nRF52840 handles all Bluetooth Low Energy traffic, exposing a
 * Nordic UART Service (NUS) transparent pipe. The STM32H7 pushes
 * captured TPM transactions and status over USART3; the nRF52840
 * relays them to the mobile app. Commands from the app travel the
 * reverse path.
 *
 * Frame format (both directions), little-endian:
 *   [0x55][0xAA][LEN][CMD][payload...][CRC8]
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "ble_bridge.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

#define BLE_FRAME_SOF0   0x55
#define BLE_FRAME_SOF1   0xAA
#define BLE_MAX_PAYLOAD  200

static volatile uint8_t g_rx_state = 0;  /* 0=sOF0 1=sOF1 2=len 3=cmd 4=payload 5=crc */
static volatile uint8_t g_rx_len = 0;
static volatile uint8_t g_rx_cmd = 0;
static volatile uint8_t g_rx_idx = 0;
static volatile uint8_t g_rx_crc = 0;
static volatile uint8_t g_rx_buf[BLE_MAX_PAYLOAD];

static ble_msg_handler_t g_handler = 0;

/* TX scratch */
static uint8_t g_tx_scratch[BLE_MAX_PAYLOAD + 8];

/* Stats */
volatile uint32_t ble_frames_tx = 0;
volatile uint32_t ble_frames_rx = 0;
volatile uint32_t ble_crc_errors = 0;

/* ===================================================================
 * CRC-8 (polynomial 0x07, init 0x00) — Maxim/Dallas style
 * =================================================================== */
static uint8_t crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* ===================================================================
 * Initialize USART3 for BLE bridge
 * =================================================================== */
void ble_bridge_init(void)
{
    /* Enable GPIOD and USART3 clocks */
    SET_BIT(RCC_AHB4ENR, BIT(3));   /* GPIOD */
    SET_BIT(RCC_APB1ENR, BIT(18)); /* USART3 */

    /* TX: PD8, AF7 (USART3_TX) */
    gpio_set_mode(BLE_UART_TX_PORT, BLE_UART_TX_PIN, GPIO_MODE_AF);
    gpio_set_af(BLE_UART_TX_PORT, BLE_UART_TX_PIN, 7);
    gpio_set_speed(BLE_UART_TX_PORT, BLE_UART_TX_PIN, GPIO_SPEED_HIGH);

    /* RX: PD9, AF7 (USART3_RX) */
    gpio_set_mode(BLE_UART_RX_PORT, BLE_UART_RX_PIN, GPIO_MODE_AF);
    gpio_set_af(BLE_UART_RX_PORT, BLE_UART_RX_PIN, 7);
    gpio_set_speed(BLE_UART_RX_PORT, BLE_UART_RX_PIN, GPIO_SPEED_HIGH);

    /* nRST: PD10, output (high = not reset) */
    gpio_set_mode(BLE_nRST_PORT, BLE_nRST_PIN, GPIO_MODE_OUTPUT);
    gpio_write(BLE_nRST_PORT, BLE_nRST_PIN, 1);

    /* IRQ: PD11, input */
    gpio_set_mode(BLE_IRQ_PORT, BLE_IRQ_PIN, GPIO_MODE_INPUT);
    gpio_set_pupd(BLE_IRQ_PORT, BLE_IRQ_PIN, GPIO_PUPD_PU);

    /* Configure USART3: 115200 8N1, enable TX/RX, RX interrupt */
    BLE_UART_INST->BRR = (APB1_CLK_HZ + BLE_BAUDRATE / 2) / BLE_BAUDRATE;
    BLE_UART_INST->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;

    /* Enable USART3 IRQ in NVIC (IRQ 80 → bit 80-64=16 in ISER1) */
    NVIC_ISER0 = BIT(80);  /* simplified — ISER index 2 bit 16 */

    g_rx_state = 0;
    g_handler = 0;
}

/* ===================================================================
 * Register message handler
 * =================================================================== */
void ble_bridge_set_handler(ble_msg_handler_t h) { g_handler = h; }

/* ===================================================================
 * Send a framed message
 * =================================================================== */
uint8_t ble_bridge_send(uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    if (len > BLE_MAX_PAYLOAD)
        return 0;
    g_tx_scratch[0] = BLE_FRAME_SOF0;
    g_tx_scratch[1] = BLE_FRAME_SOF1;
    g_tx_scratch[2] = len;
    g_tx_scratch[3] = cmd;
    if (len)
        memcpy(&g_tx_scratch[4], payload, len);
    g_tx_scratch[4 + len] = crc8(&g_tx_scratch[2], 2 + len);

    uint16_t total = 5 + len;
    for (uint16_t i = 0; i < total; i++) {
        while (!(BLE_UART_INST->ISR & USART_ISR_TXE)) ;
        BLE_UART_INST->TD = g_tx_scratch[i];
    }
    ble_frames_tx++;
    return 1;
}

/* ===================================================================
 * USART3 RX interrupt handler
 * =================================================================== */
void USART3_IRQHandler(void)
{
    if (BLE_UART_INST->ISR & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)BLE_UART_INST->RD;
        switch (g_rx_state) {
        case 0:
            if (b == BLE_FRAME_SOF0) g_rx_state = 1;
            break;
        case 1:
            if (b == BLE_FRAME_SOF1) g_rx_state = 2;
            else g_rx_state = 0;
            break;
        case 2:
            g_rx_len = b;
            g_rx_idx = 0;
            g_rx_crc = 0;
            g_rx_state = 3;
            break;
        case 3:
            g_rx_cmd = b;
            g_rx_state = (g_rx_len > 0) ? 4 : 5;
            break;
        case 4:
            g_rx_buf[g_rx_idx++] = b;
            if (g_rx_idx >= g_rx_len)
                g_rx_state = 5;
            break;
        case 5: {
            uint8_t calc = crc8(&g_rx_len, 2 + g_rx_len);
            if (b == calc) {
                ble_frames_rx++;
                if (g_handler)
                    g_handler(g_rx_cmd, (uint8_t *)g_rx_buf, g_rx_len);
            } else {
                ble_crc_errors++;
            }
            g_rx_state = 0;
            break;
        }
        default:
            g_rx_state = 0;
            break;
        }
    }
}

/* ===================================================================
 * Reset BLE co-processor
 * =================================================================== */
void ble_bridge_reset(void)
{
    gpio_write(BLE_nRST_PORT, BLE_nRST_PIN, 0);
    for (volatile int i = 0; i < 100000; i++) __asm__("nop");
    gpio_write(BLE_nRST_PORT, BLE_nRST_PIN, 1);
}