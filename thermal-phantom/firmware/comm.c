/*
 * comm.c - USB CDC + BLE UART communication with binary protocol
 *
 * Packet format: [SYNC:0xAA][CMD:1B][LEN:2B][DATA:N][CRC:2B]
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPLv3
 */

#include "comm.h"
#include "board.h"
#include "registers.h"
#include <string.h>

#define RX_BUF_SIZE   512
#define TX_BUF_SIZE   512

static uint8_t rx_buffer[RX_BUF_SIZE];
static uint16_t rx_index = 0;
static uint8_t tx_buffer[TX_BUF_SIZE];

static command_handler_t cmd_handler = NULL;
static uint32_t rx_packet_count = 0;
static uint32_t tx_packet_count = 0;
static bool usb_connected = false;
static bool ble_connected = false;

/* UART ring buffers for BLE */
static uint8_t ble_rx_buf[256];
static volatile uint16_t ble_rx_head = 0;
static volatile uint16_t ble_rx_tail = 0;

extern volatile uint32_t systick_ms;

/* ============================================================
 * UART configuration
 * ============================================================ */

static void uart_init(uint32_t uart_base, uint32_t baud)
{
    uint32_t apb_freq;
    
    if (uart_base == USART3_BASE)
        apb_freq = BOARD_APB1_FREQ_HZ;
    else if (uart_base == UART4_BASE)
        apb_freq = BOARD_APB1_FREQ_HZ;
    else
        return;
    
    /* Configure TX/RX pins */
    if (uart_base == USART3_BASE) {
        /* PB10=TX, PB11=RX, AF7 */
        uint32_t pb_base = GPIOB_BASE;
        GPIO_MODER(pb_base) &= ~(3U << (10 * 2));
        GPIO_MODER(pb_base) |= (GPIO_MODE_AF << (10 * 2));
        GPIO_MODER(pb_base) &= ~(3U << (11 * 2));
        GPIO_MODER(pb_base) |= (GPIO_MODE_AF << (11 * 2));
        REG32(pb_base + GPIO_AFRL_OFFSET) &= ~(0xFU << (10 * 4));
        REG32(pb_base + GPIO_AFRL_OFFSET) |= (7U << (10 * 4));
        REG32(pb_base + GPIO_AFRL_OFFSET) &= ~(0xFU << (11 * 4));
        REG32(pb_base + GPIO_AFRL_OFFSET) |= (7U << (11 * 4));
        usb_connected = true;
    } else if (uart_base == UART4_BASE) {
        /* PA0=TX, PA1=RX, AF6 -- note: conflicts with TEC_IPROPI
         * In production, remap TEC_IPROPI to another pin or use
         * a different UART. For this design, BLE UART uses PA0/PA1
         * and TEC current monitoring uses a different ADC channel. */
        uint32_t pa_base = GPIOA_BASE;
        GPIO_MODER(pa_base) &= ~(3U << (0 * 2));
        GPIO_MODER(pa_base) |= (GPIO_MODE_AF << (0 * 2));
        GPIO_MODER(pa_base) &= ~(3U << (1 * 2));
        GPIO_MODER(pa_base) |= (GPIO_MODE_AF << (1 * 2));
        REG32(pa_base + GPIO_AFRL_OFFSET) &= ~(0xFU << (0 * 4));
        REG32(pa_base + GPIO_AFRL_OFFSET) |= (6U << (0 * 4));
        REG32(pa_base + GPIO_AFRL_OFFSET) &= ~(0xFU << (1 * 4));
        REG32(pa_base + GPIO_AFRL_OFFSET) |= (6U << (1 * 4));
    }
    
    /* Configure baud rate */
    USART_BRR(uart_base) = apb_freq / baud;
    
    /* Enable TX, RX, and RXNE interrupt */
    USART_CR1(uart_base) = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    
    if (uart_base == USART3_BASE) {
        NVIC_EnableIRQ(USART3_IRQn);
    } else if (uart_base == UART4_BASE) {
        NVIC_EnableIRQ(UART4_IRQn);
    }
}

static void uart_send_byte(uint32_t uart_base, uint8_t byte)
{
    while (!(USART_ISR(uart_base) & USART_ISR_TXE));
    USART_TDR(uart_base) = byte;
}

static void uart_send_bytes(uint32_t uart_base, const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        uart_send_byte(uart_base, data[i]);
    }
}

/* ============================================================
 * BLE UART interrupt handler
 * ============================================================ */

void UART4_IRQHandler(void)
{
    uint32_t uart_base = UART4_BASE;
    
    if (USART_ISR(uart_base) & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)USART_RDR(uart_base);
        uint16_t next = (ble_rx_head + 1) % sizeof(ble_rx_buf);
        if (next != ble_rx_tail) {
            ble_rx_buf[ble_rx_head] = byte;
            ble_rx_head = next;
        }
        ble_connected = true;
    }
}

/* ============================================================
 * Packet parsing
 * ============================================================ */

static bool parse_packet(void)
{
    /* Look for sync byte */
    while (rx_index > 0 && rx_buffer[0] != COMM_SYNC_BYTE) {
        memmove(rx_buffer, rx_buffer + 1, --rx_index);
    }
    
    if (rx_index < COMM_HEADER_SIZE)
        return false;
    
    /* Parse header */
    uint8_t cmd = rx_buffer[1];
    uint16_t len = rx_buffer[2] | ((uint16_t)rx_buffer[3] << 8);
    
    if (len > COMM_MAX_PAYLOAD)
        return false;
    
    /* Check if we have the full packet */
    uint16_t packet_len = COMM_HEADER_SIZE + len + COMM_CRC_SIZE;
    if (rx_index < packet_len)
        return false;
    
    /* Verify CRC */
    uint16_t crc_calc = crc16_ccitt(rx_buffer, COMM_HEADER_SIZE + len);
    uint16_t crc_recv = rx_buffer[COMM_HEADER_SIZE + len] |
                        ((uint16_t)rx_buffer[COMM_HEADER_SIZE + len + 1] << 8);
    
    if (crc_calc != crc_recv) {
        /* CRC error: discard sync byte and retry */
        memmove(rx_buffer, rx_buffer + 1, --rx_index);
        return false;
    }
    
    /* Valid packet: dispatch to handler */
    if (cmd_handler) {
        cmd_handler(cmd, rx_buffer + COMM_HEADER_SIZE, len);
    }
    
    rx_packet_count++;
    
    /* Remove processed packet from buffer */
    uint16_t remaining = rx_index - packet_len;
    if (remaining > 0) {
        memmove(rx_buffer, rx_buffer + packet_len, remaining);
    }
    rx_index = remaining;
    
    return true;
}

/* ============================================================
 * BLE RX processing
 * ============================================================ */

static void process_ble_rx(void)
{
    while (ble_rx_tail != ble_rx_head) {
        uint8_t byte = ble_rx_buf[ble_rx_tail];
        ble_rx_tail = (ble_rx_tail + 1) % sizeof(ble_rx_buf);
        
        if (rx_index < RX_BUF_SIZE) {
            rx_buffer[rx_index++] = byte;
        }
        
        /* Try to parse after each byte */
        if (byte == COMM_SYNC_BYTE || rx_index >= COMM_HEADER_SIZE) {
            if (rx_index >= COMM_HEADER_SIZE) {
                parse_packet();
            }
        }
    }
}

/* ============================================================
 * USB CDC (via USART3 from FT4232H)
 * ============================================================ */

static bool usb_rx_pending(void)
{
    return (USART_ISR(USART3_BASE) & USART_ISR_RXNE) != 0;
}

static uint8_t usb_rx_byte(void)
{
    return (uint8_t)USART_RDR(USART3_BASE);
}

void USART3_IRQHandler(void)
{
    if (usb_rx_pending()) {
        uint8_t byte = usb_rx_byte();
        if (rx_index < RX_BUF_SIZE) {
            rx_buffer[rx_index++] = byte;
        }
        usb_connected = true;
    }
}

/* ============================================================
 * Public API
 * ============================================================ */

void comm_init(void)
{
    rx_index = 0;
    rx_packet_count = 0;
    tx_packet_count = 0;
    ble_rx_head = 0;
    ble_rx_tail = 0;
    
    uart_init(USART3_BASE, USB_UART_BAUD);
    uart_init(UART4_BASE, BLE_UART_BAUD);
}

void comm_process(void)
{
    /* Process USB RX */
    while (usb_rx_pending()) {
        uint8_t byte = usb_rx_byte();
        if (rx_index < RX_BUF_SIZE) {
            rx_buffer[rx_index++] = byte;
        }
    }
    
    /* Process BLE RX ring buffer */
    process_ble_rx();
    
    /* Try to parse any complete packets */
    while (rx_index >= COMM_HEADER_SIZE) {
        if (!parse_packet())
            break;
    }
}

void comm_send_response(uint8_t resp_type, const uint8_t *data, uint16_t len)
{
    if (len > COMM_MAX_PAYLOAD)
        len = COMM_MAX_PAYLOAD;
    
    uint16_t packet_len = COMM_HEADER_SIZE + len + COMM_CRC_SIZE;
    
    /* Build packet */
    tx_buffer[0] = COMM_SYNC_BYTE;
    tx_buffer[1] = resp_type;
    tx_buffer[2] = len & 0xFF;
    tx_buffer[3] = (len >> 8) & 0xFF;
    
    if (data && len > 0) {
        memcpy(&tx_buffer[4], data, len);
    }
    
    /* CRC over header + payload */
    uint16_t crc = crc16_ccitt(tx_buffer, COMM_HEADER_SIZE + len);
    tx_buffer[COMM_HEADER_SIZE + len] = crc & 0xFF;
    tx_buffer[COMM_HEADER_SIZE + len + 1] = (crc >> 8) & 0xFF;
    
    /* Send on both USB and BLE */
    if (usb_connected) {
        uart_send_bytes(USART3_BASE, tx_buffer, packet_len);
    }
    if (ble_connected) {
        uart_send_bytes(UART4_BASE, tx_buffer, packet_len);
    }
    
    tx_packet_count++;
}

void comm_send_status(void)
{
    uint8_t status[16];
    extern system_state_t get_system_state(void);
    extern float get_current_temp(void);
    extern float get_target_temp(void);
    extern uint16_t get_battery_pct(void);
    
    system_state_t state = get_system_state();
    float temp = get_current_temp();
    float target = get_target_temp();
    uint16_t bat = get_battery_pct();
    
    /* Pack status: state(1) + current_temp(4) + target_temp(4) + battery(2) + 
     * tec_duty(4) + flags(1) = 16 bytes */
    status[0] = (uint8_t)state;
    memcpy(&status[1], &temp, 4);
    memcpy(&status[5], &target, 4);
    status[9] = bat & 0xFF;
    status[10] = (bat >> 8) & 0xFF;
    /* TEC duty would go here */
    int32_t duty = 0;
    memcpy(&status[11], &duty, 4);
    status[15] = 0;  /* flags */
    
    comm_send_response(RESP_STATUS, status, sizeof(status));
}

void comm_send_log(const char *msg)
{
    uint16_t len = strlen(msg);
    if (len > COMM_MAX_PAYLOAD)
        len = COMM_MAX_PAYLOAD;
    comm_send_response(RESP_LOG, (const uint8_t *)msg, len);
}

void comm_register_handler(command_handler_t handler)
{
    cmd_handler = handler;
}

uint32_t comm_get_rx_count(void)
{
    return rx_packet_count;
}

uint32_t comm_get_tx_count(void)
{
    return tx_packet_count;
}

bool comm_is_usb_connected(void)
{
    return usb_connected;
}

bool comm_is_ble_connected(void)
{
    return ble_connected;
}