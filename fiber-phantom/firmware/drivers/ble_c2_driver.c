/*
 * ble_c2_driver.c — BLE command & control driver (UART to nRF52840)
 * Author: jayis1
 * Date:   2026-06-17
 *
 * Implements the UART link between the RP2040 and the nRF52840 BLE module,
 * and the FiberPhantom C2 protocol for app ↔ device communication.
 */

#include "ble_c2_driver.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* ---- Private state ---- */
static uint8_t ble_connected = 0;
static uint8_t rx_seq_expected = 0;
static c2_msg_t rx_msg_buf;
static uint8_t rx_byte_buf[256];
static uint8_t rx_byte_idx = 0;

/* ---- UART helper functions ---- */

static void delay_us(uint32_t us)
{
    volatile uint32_t count = us * 44;
    while (count--) __asm__ volatile ("nop");
}

static void gpio_init_simple(uint8_t pin, uint8_t output, uint8_t func)
{
    IO_BANK0_GPIO_CTRL(pin) = func;
    uint32_t pad = PAD_IE | PAD_DRIVE_4MA;
    if (!output) pad |= PAD_OD;
    PADS_BANK0_GPIO(pin) = pad;
    if (output)
        SIO_GPIO_OE_SET = GPIO_BIT(pin);
    else
        SIO_GPIO_OE_CLR = GPIO_BIT(pin);
}

static void uart1_init_hw(void)
{
    /* Configure UART1 pins: TX=GPIO16, RX=GPIO17, CTS=GPIO18, RTS=GPIO19 */
    IO_BANK0_GPIO_CTRL(PIN_BLE_TX) = GPIO_FUNC_UART;
    IO_BANK0_GPIO_CTRL(PIN_BLE_RX) = GPIO_FUNC_UART;
    IO_BANK0_GPIO_CTRL(PIN_BLE_CTS) = GPIO_FUNC_UART;
    IO_BANK0_GPIO_CTRL(PIN_BLE_RTS) = GPIO_FUNC_UART;

    /* Configure nRF52840 reset pin (open-drain output, default high) */
    gpio_init_simple(PIN_BLE_NRESET, 1, GPIO_FUNC_SIO);
    SIO_GPIO_OUT_SET = GPIO_BIT(PIN_BLE_NRESET);  /* Not in reset */

    /* Pad config */
    PADS_BANK0_GPIO(PIN_BLE_TX) = PAD_IE | PAD_DRIVE_8MA | PAD_SLEWFAST;
    PADS_BANK0_GPIO(PIN_BLE_RX) = PAD_IE | PAD_DRIVE_4MA;
    PADS_BANK0_GPIO(PIN_BLE_CTS) = PAD_IE;
    PADS_BANK0_GPIO(PIN_BLE_RTS) = PAD_IE | PAD_DRIVE_8MA;

    /* Reset UART1 */
    PSM_FRCE_OFF |= PSM_UART1;
    delay_us(10);
    PSM_FRCE_OFF &= ~PSM_UART1;
    delay_us(100);

    /* Disable UART for configuration */
    UART1->cr = 0;

    /* Set baud rate: 921600
     * RP2040 UART baud = (PERI_CLK / 16) / (IBRD + FBRD/64)
     * PERI_CLK = 125 MHz
     * Target: 921600 → divisor = 125000000 / (16 * 921600) = 8.467
     * IBRD = 8, FBRD = round(0.467 * 64) = 30
     */
    UART1->ibrd = 8;
    UART1->fbrd = 30;

    /* 8 data bits, no parity, 1 stop bit, enable FIFOs */
    UART1->lcr_h = UART_LCR_H_WLEN_8 | UART_LCR_H_FEN;

    /* Set FIFO interrupt trigger levels */
    UART1->ifls = (2 << 0) | (2 << 3);  /* RX/TX at 1/2 full */

    /* Enable UART with TX, RX, and hardware flow control */
    UART1->cr = UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE |
                UART_CR_RTSEN | UART_CR_CTSEN;

    /* Reset the nRF52840 module */
    delay_us(10000);
    SIO_GPIO_OUT_CLR = GPIO_BIT(PIN_BLE_NRESET);
    delay_us(10000);
    SIO_GPIO_OUT_SET = GPIO_BIT(PIN_BLE_NRESET);
    delay_us(100000);  /* 100ms for nRF52840 boot */
}

static void uart1_putc(char c)
{
    /* Wait for TX FIFO not full */
    while (UART1->fr & UART_FR_TXFF) { }
    UART1->dr = (uint32_t)c;
}

static char uart1_getc(uint32_t timeout_ms)
{
    uint32_t timeout = timeout_ms * 44 * 1000;
    while (!(UART1->fr & UART_FR_RXFE) && --timeout) {
        /* Wait for data in RX FIFO */
        /* RXFE means RX FIFO EMPTY, so we wait for it to clear */
        __asm__ volatile ("nop");
    }
    /* Check if data available (RXFE=0 means data present) */
    if (UART1->fr & UART_FR_RXFE) {
        return -1;  /* Timeout, no data */
    }
    return (char)(UART1->dr & 0xFF);
}

static int uart1_rx_available(void)
{
    return !(UART1->fr & UART_FR_RXFE);
}

static void uart1_write(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        uart1_putc((char)data[i]);
    }
}

/* ============================================================
 * CRC-8 (polynomial 0x07, initial value 0x00)
 * ============================================================ */
uint8_t c2_crc8(const uint8_t *data, uint32_t len)
{
    uint8_t crc = 0x00;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ C2_CRC8_POLY;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/* ============================================================
 * Frame encoding: [SOH] [TYPE] [SEQ] [FLAGS] [LEN] [payload...] [CRC8] [EOT]
 * ============================================================ */

#define C2_FRM_SOH  0x01  /* Start of header */
#define C2_FRM_EOT  0x04  /* End of transmission */

static int ble_c2_send_raw(const c2_msg_t *msg)
{
    uint8_t frame[256];
    uint32_t idx = 0;

    frame[idx++] = C2_FRM_SOH;
    frame[idx++] = msg->type;
    frame[idx++] = msg->seq;
    frame[idx++] = msg->flags | 0x01;  /* Bit 0: CRC present */
    frame[idx++] = msg->length;

    /* Copy payload (may contain SOH/EOT bytes — transparent framing) */
    for (uint8_t i = 0; i < msg->length; i++) {
        frame[idx++] = msg->payload[i];
    }

    /* CRC over type, seq, flags, length, payload */
    uint8_t crc = c2_crc8(frame + 1, 4 + msg->length);
    frame[idx++] = crc;
    frame[idx++] = C2_FRM_EOT;

    uart1_write(frame, idx);
    return (int)idx;
}

/* ============================================================
 * Public API
 * ============================================================ */

void ble_c2_init(void)
{
    uart1_init_hw();
    ble_connected = 0;
    rx_seq_expected = 0;
    rx_byte_idx = 0;
}

int ble_c2_send(const c2_msg_t *msg)
{
    if (!ble_connected) {
        return -1;
    }
    return ble_c2_send_raw(msg);
}

int ble_c2_recv(c2_msg_t *msg, uint32_t timeout_ms)
{
    uint32_t timeout = timeout_ms * 44 * 1000;

    while (timeout--) {
        if (!uart1_rx_available()) {
            __asm__ volatile ("nop");
            continue;
        }

        uint8_t byte = (uint8_t)(UART1->dr & 0xFF);

        if (rx_byte_idx == 0) {
            /* Looking for SOH */
            if (byte == C2_FRM_SOH) {
                rx_byte_buf[rx_byte_idx++] = byte;
            }
        } else if (rx_byte_idx == 1) {
            rx_byte_buf[rx_byte_idx++] = byte;  /* Type */
        } else if (rx_byte_idx == 2) {
            rx_byte_buf[rx_byte_idx++] = byte;  /* Seq */
        } else if (rx_byte_idx == 3) {
            rx_byte_buf[rx_byte_idx++] = byte;  /* Flags */
        } else if (rx_byte_idx == 4) {
            rx_byte_buf[rx_byte_idx++] = byte;  /* Length */
            if (byte == 0 && rx_byte_buf[3] & 0x01) {
                /* No payload, expecting CRC + EOT */
            }
        } else if (rx_byte_idx < 5 + rx_byte_buf[4]) {
            /* Receiving payload */
            rx_byte_buf[rx_byte_idx++] = byte;
        } else if (rx_byte_idx == 5 + rx_byte_buf[4]) {
            /* CRC byte */
            rx_byte_buf[rx_byte_idx++] = byte;
        } else if (rx_byte_idx == 5 + rx_byte_buf[4] + 1) {
            /* Should be EOT */
            if (byte == C2_FRM_EOT) {
                /* Validate CRC */
                uint8_t expected_crc = c2_crc8(rx_byte_buf + 1,
                                               4 + rx_byte_buf[4]);
                if (rx_byte_buf[5 + rx_byte_buf[4]] == expected_crc) {
                    /* Valid message — copy to output */
                    msg->type = rx_byte_buf[1];
                    msg->seq = rx_byte_buf[2];
                    msg->flags = rx_byte_buf[3];
                    msg->length = rx_byte_buf[4];
                    memcpy(msg->payload, rx_byte_buf + 5, msg->length);
                    msg->crc = rx_byte_buf[5 + msg->length];
                    rx_byte_idx = 0;
                    rx_seq_expected = (msg->seq + 1) & 0xFF;
                    return 1;  /* Message received */
                }
            }
            /* Invalid frame — reset */
            rx_byte_idx = 0;
        }
    }

    rx_byte_idx = 0;
    return 0;  /* Timeout, no message */
}

int ble_c2_send_status(const sys_status_t *status)
{
    c2_msg_t msg = {0};
    msg.type = C2_MSG_STATUS;
    msg.seq = rx_seq_expected;
    msg.flags = 0;
    msg.length = sizeof(sys_status_t);
    if (msg.length > C2_MAX_PAYLOAD) msg.length = C2_MAX_PAYLOAD;
    memcpy(msg.payload, status, msg.length);
    return ble_c2_send(&msg);
}

int ble_c2_send_stats(const capture_stats_t *stats)
{
    c2_msg_t msg = {0};
    msg.type = C2_MSG_STATS;
    msg.seq = rx_seq_expected;
    msg.length = sizeof(capture_stats_t);
    if (msg.length > C2_MAX_PAYLOAD) msg.length = C2_MAX_PAYLOAD;
    memcpy(msg.payload, stats, msg.length);
    return ble_c2_send(&msg);
}

int ble_c2_send_alert(alert_code_t alert, uint8_t severity)
{
    c2_msg_t msg = {0};
    msg.type = C2_MSG_ALERT;
    msg.seq = rx_seq_expected;
    msg.length = 2;
    msg.payload[0] = (uint8_t)alert;
    msg.payload[1] = severity;
    return ble_c2_send(&msg);
}

int ble_c2_send_version(void)
{
    c2_msg_t msg = {0};
    msg.type = C2_MSG_VERSION;
    msg.length = 16;
    /* Pack version as: "FP vMAJOR.MINOR.PATCH" */
    const char *ver = "FP v1.0.0     ";
    memcpy(msg.payload, ver, 14);
    msg.payload[14] = BOARD_SERIAL_DEFAULT[3];  /* Serial info */
    msg.payload[15] = 0;
    return ble_c2_send(&msg);
}

void ble_c2_poll(void)
{
    c2_msg_t msg;
    /* Non-blocking receive: 1ms timeout */
    if (ble_c2_recv(&msg, 1)) {
        /* Message received — caller (main loop) handles dispatch */
        /* Store for main loop to read via ble_c2_get_last_msg() */
        /* For simplicity, we set a flag and store in a global */
        extern volatile uint8_t c2_msg_ready;
        extern volatile c2_msg_t c2_last_msg;
        c2_last_msg = msg;
        c2_msg_ready = 1;
    }
}

uint8_t ble_c2_is_connected(void)
{
    return ble_connected;
}

void ble_c2_set_connected(uint8_t connected)
{
    ble_connected = connected;
}

/* ---- Global storage for main loop access ---- */
volatile uint8_t c2_msg_ready = 0;
volatile c2_msg_t c2_last_msg;