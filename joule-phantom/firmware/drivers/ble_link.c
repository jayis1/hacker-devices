/*
 * ble_link.c — BLE back-haul UART protocol implementation (Joule-Phantom).
 *
 * Author : jayis1
 * License: GPL-2.0
 *
 * Implements the framed serial protocol between the STM32L432 and the
 * ESP32-C3 BLE co-processor.  The ESP32-C3 runs a thin GATT server that
 * mirrors this protocol over a Nordic-style UART-BLE service.
 */

#include "ble_link.h"
#include "smbus_port.h"
#include "registers.h"
#include <string.h>

static op_mode_t current_mode = MODE_PASSTHROUGH;

/* Simple ring for UART TX (interrupt-driven) */
#define TX_BUF_SZ 256
static uint8_t  tx_buf[TX_BUF_SZ];
static volatile uint16_t tx_head, tx_tail;

#define RX_BUF_SZ 256
static uint8_t  rx_buf[RX_BUF_SZ];
static volatile uint16_t rx_head, rx_tail;

/* ------------------------------------------------------------------ */
/*  CRC-8                                                              */
/* ------------------------------------------------------------------ */
uint8_t ble_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x07);
            else            crc = (uint8_t)(crc << 1);
        }
    }
    return crc;
}

/* ------------------------------------------------------------------ */
/*  USART2 ISR (TXE / RXNE)                                            */
/* ------------------------------------------------------------------ */
extern void USART2_IRQHandler(void);

void USART2_IRQHandler(void)
{
    uint32_t isr = USART2->ISR;
    if (isr & USART_ISR_RXNE) {
        uint8_t b = (uint8_t)USART2->RDR;
        uint16_t next = (uint16_t)((rx_head + 1u) % RX_BUF_SZ);
        if (next != rx_tail) {
            rx_buf[rx_head] = b;
            rx_head = next;
        }
        /* overflow else: drop */
    }
    if (isr & USART_ISR_TXE) {
        if (tx_head != tx_tail) {
            USART2->TDR = tx_buf[tx_tail];
            tx_tail = (uint16_t)((tx_tail + 1u) % TX_BUF_SZ);
        } else {
            USART2->CR1 &= ~USART_CR1_TXEIE;   /* nothing more to send */
        }
    }
    if (isr & USART_ISR_TC) {
        USART2->ICR = USART_ISR_TC;
    }
}

/* ------------------------------------------------------------------ */
/*  ble_init                                                           */
/* ------------------------------------------------------------------ */
void ble_init(void)
{
    /* Enable USART2 clock (already enabled in board_init) */
    RCC_APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    /* Configure PA2 (TX) and PA3 (RX) as AF7 */
    uint32_t pa_moder = GPIOA->MODER;
    pa_moder &= ~((3u << (PIN_BLE_USART_TX * 2)) | (3u << (PIN_BLE_USART_RX * 2)));
    pa_moder |=  (GPIO_MODE_AF << (PIN_BLE_USART_TX * 2)) | (GPIO_MODE_AF << (PIN_BLE_USART_RX * 2));
    GPIOA->MODER = pa_moder;
    GPIOA->AFRL &= ~((0xFu << (PIN_BLE_USART_TX * 4)) | (0xFu << (PIN_BLE_USART_RX * 4)));
    GPIOA->AFRL |=  ((GPIO_AF7 << (PIN_BLE_USART_TX * 4)) | (GPIO_AF7 << (PIN_BLE_USART_RX * 4)));

    /* Baud = PCLK / BRR; 80 MHz / 115200 = ~694 */
    USART2->BRR = (BOARD_APB1_HZ / BLE_BAUDRATE);
    USART2->CR2 = 0;
    USART2->CR3 = 0;
    USART2->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;

    tx_head = tx_tail = 0;
    rx_head = rx_tail = 0;

    /* Enable NVIC USART2 interrupt */
    NVIC_ISER(0) = (1u << IRQ_USART2);

    /* Send hello */
    uint8_t hello[] = { 0x01, 'J', 'P', '1' };
    ble_send(BLE_MSG_HELLO, hello, sizeof(hello));
}

/* ------------------------------------------------------------------ */
/*  ble_send                                                           */
/* ------------------------------------------------------------------ */
static void uart_put_blocking(uint8_t b)
{
    while (!(USART2->ISR & USART_ISR_TXE)) { /* spin briefly */ }
    USART2->TDR = b;
}

void ble_send(ble_msg_t op, const uint8_t *payload, uint8_t len)
{
    if (len > 64) len = 64;   /* cap payload for BLE MTU safety */

    /* Build frame in a local buffer to compute CRC */
    uint8_t frame[80];
    uint8_t idx = 0;
    frame[idx++] = BLE_SOF;
    frame[idx++] = len;
    frame[idx++] = (uint8_t)op;
    for (uint8_t i = 0; i < len; i++) frame[idx++] = payload[i];
    uint8_t crc = ble_crc8(&frame[1], (uint8_t)(2 + len)); /* LEN+OP+payload */
    frame[idx++] = crc;
    frame[idx++] = BLE_EOF;

    /* Push to TX ring; if ring full, drop oldest */
    for (uint8_t i = 0; i < idx; i++) {
        uint16_t next = (uint16_t)((tx_head + 1u) % TX_BUF_SZ);
        if (next == tx_tail) {
            /* ring full: transmit blocking as fallback */
            uart_put_blocking(frame[i]);
        } else {
            tx_buf[tx_head] = frame[i];
            tx_head = next;
        }
    }
    USART2->CR1 |= USART_CR1_TXEIE;   /* enable TX interrupt */
}

/* ------------------------------------------------------------------ */
/*  ble_poll — reassemble inbound frames                               */
/* ------------------------------------------------------------------ */
void ble_poll(void)
{
    static uint8_t  rxFrm[80];
    static uint8_t  rxLen = 0, rxOp = 0, rxIdx = 0, rxState = 0, rxCrc = 0;

    while (rx_head != rx_tail) {
        uint8_t b = rx_buf[rx_tail];
        rx_tail = (uint16_t)((rx_tail + 1u) % RX_BUF_SZ);

        switch (rxState) {
        case 0: /* wait SOF */
            if (b == BLE_SOF) rxState = 1;
            break;
        case 1: /* length */
            rxLen = b;
            rxIdx = 0;
            rxState = 2;
            break;
        case 2: /* opcode */
            rxOp = b;
            rxState = 3;
            break;
        case 3: /* payload */
            if (rxIdx < sizeof(rxFrm)) rxFrm[rxIdx++] = b;
            if (rxIdx >= rxLen) rxState = 4;
            break;
        case 4: /* CRC */
            rxCrc = b;
            rxState = 5;
            break;
        case 5: /* EOF */
            if (b == BLE_EOF) {
                /* Verify CRC over [LEN][OP][payload] */
                uint8_t tmp[66];
                tmp[0] = rxLen;
                tmp[1] = rxOp;
                memcpy(&tmp[2], rxFrm, rxLen);
                uint8_t calc = ble_crc8(tmp, (uint8_t)(2 + rxLen));
                if (calc == rxCrc) {
                    ble_dispatch((ble_msg_t)rxOp, rxFrm, rxLen);
                } else {
                    uint8_t nack = 0x01;
                    ble_send(BLE_MSG_NACK, &nack, 1);
                }
            }
            rxState = 0;
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  ble_dispatch — handle inbound commands from operator app           */
/* ------------------------------------------------------------------ */
void ble_dispatch(ble_msg_t op, const uint8_t *payload, uint8_t len)
{
    switch (op) {
    case BLE_MSG_RULE_ADD:
        if (len >= 4) {
            mitm_rule_t r = {0};
            r.cmd       = payload[0];
            r.mask      = payload[1];
            r.action    = (mitm_action_t)payload[2];
            r.spoof_len = (len > 3) ? (uint8_t)(len - 3) : 0;
            if (r.spoof_len > sizeof(r.spoof)) r.spoof_len = sizeof(r.spoof);
            memcpy(r.spoof, &payload[3], r.spoof_len);
            r.enabled = 1;
            mitm_rule_add(&r);
            uint8_t ack = 0;
            ble_send(BLE_MSG_ACK, &ack, 1);
        }
        break;
    case BLE_MSG_RULE_CLR:
        mitm_rules_clear();
        ble_send(BLE_MSG_ACK, 0, 0);
        break;
    case BLE_MSG_RULE_LIST: {
        uint8_t rsp[64];
        rsp[0] = mitm_rule_count();
        uint8_t off = 1;
        for (uint8_t i = 0; i < mitm_rule_count() && off < 60; i++) {
            const mitm_rule_t *r = mitm_rule_get(i);
            rsp[off++] = r->cmd;
            rsp[off++] = r->mask;
            rsp[off++] = (uint8_t)r->action;
            rsp[off++] = r->spoof_len;
        }
        ble_send(BLE_MSG_RULE_LIST_RSP, rsp, off);
        break;
    }
    case BLE_MSG_SET_MODE:
        if (len >= 1) ble_set_mode((op_mode_t)payload[0]);
        ble_send(BLE_MSG_ACK, 0, 0);
        break;
    case BLE_MSG_GLITCH:
        if (len >= 2) {
            uint32_t us = payload[0] | (payload[1] << 8);
            if (us > GLITCH_PULSE_MAX_US) us = GLITCH_PULSE_MAX_US;
            board_glitch_pulse(us);
            ble_send(BLE_MSG_ACK, 0, 0);
        }
        break;
    case BLE_MSG_AUTH_BYPASS:
        /* Enable auth-bypass rule (spoof ManufacturerAccess ACK) */
        mitm_rule_t r = {0};
        r.cmd = SBS_MANUFACTURER_ACCESS;
        r.mask = 0xFF;
        r.action = MITM_ACT_SPOOF;
        r.spoof[0] = 0x00; r.spoof[1] = 0x00;
        r.spoof_len = 2;
        r.enabled = 1;
        mitm_rule_add(&r);
        ble_send(BLE_MSG_ACK, 0, 0);
        break;
    case BLE_MSG_INJECT:
        /* payload = [addr][cmd][wlen][w0..wN] */
        if (len >= 3) {
            smb_frame_t f = {0};
            f.port = SMB_PORT_HOST;
            f.dir  = SMB_DIR_HOST_WRITE;
            f.addr = payload[0];
            f.cmd  = payload[1];
            f.wlen = payload[2];
            if (f.wlen > sizeof(f.wbuf)) f.wlen = sizeof(f.wbuf);
            memcpy(f.wbuf, &payload[3], f.wlen);
            f.ts_ms = board_millis();
            f.flags |= SMB_FLAG_INJECTED;
            smbus_bridge_transaction(&f);
        }
        break;
    default:
        ble_send(BLE_MSG_NACK, 0, 0);
        break;
    }
}

void ble_set_mode(op_mode_t m) { current_mode = m; }
op_mode_t ble_get_mode(void)   { return current_mode; }

/* ------------------------------------------------------------------ */
/*  Periodic status beacon                                             */
/* ------------------------------------------------------------------ */
void ble_status_beacon(void)
{
    static uint32_t last = 0;
    if ((board_millis() - last) < 2000) return;
    last = board_millis();

    uint8_t st[8];
    st[0] = (uint8_t)current_mode;
    st[1] = board_battery_present() ? 1 : 0;
    st[2] = mitm_rule_count();
    /* pack temperature dK (low byte) */
    uint16_t t = board_adc_read(ADC_THERM_SENSE);
    st[3] = (uint8_t)(t & 0xFF);
    st[4] = (uint8_t)(t >> 8);
    uint16_t v = board_adc_read(ADC_VBAT_SENSE);
    st[5] = (uint8_t)(v & 0xFF);
    st[6] = (uint8_t)(v >> 8);
    st[7] = (uint8_t)(board_millis() & 0xFF);
    ble_send(BLE_MSG_STATUS, st, sizeof(st));
}