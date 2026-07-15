/*
 * tesla-phantom — drivers/ble_uart.c
 * BLE communication via u-blox NINA-B306 module over USART3.
 * Custom GATT service for command/status/trace/scan-map.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"

/* NINA-B306 BLE module communication:
 * The module is controlled via AT commands over UART.
 * We define a custom GATT service with 4 characteristics.
 *
 * The module handles BLE pairing, encryption, and GATT internally.
 * We send AT commands to configure the service and then use
 * transparent UART mode for data exchange.
 */

#define BLE_BAUDRATE     115200
#define BLE_MAX_PAYLOAD  180  /* MTU - 3 bytes header */

/* BLE command response buffer */
static char ble_rx_buf[256];
static uint16_t ble_rx_len = 0;
static volatile uint8_t ble_rx_ready = 0;

/* ── USART3 init ──────────────────────────────────────────────────── */
static void ble_uart_init(void) {
    volatile usart_regs_t *u = USART3;

    /* Disable UART */
    u->CR1 = 0;

    /* Baud rate: APB1 = 120 MHz, /16 = 7.5 MHz
     * BRR = 75000000 / 115200 ≈ 651 */
    u->BRR = 651;

    /* 8 data bits, no parity, 1 stop bit, enable RX interrupt */
    u->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;

    /* Enable UART */
    u->CR1 |= USART_CR1_UE;

    /* Enable NVIC for USART3 */
    NVIC_ISER(IRQ_USART3 / 32) |= (1U << (IRQ_USART3 % 32));
}

/* ── AT command interface ─────────────────────────────────────────── */
static void ble_send_at(const char *cmd) {
    for (const char *p = cmd; *p; p++)
        usart_write(USART3, (uint8_t)*p);
    usart_write(USART3, '\r');
    usart_write(USART3, '\n');
}

static int ble_wait_response(const char *expected, uint32_t timeout_ms) {
    uint32_t start = millis();
    while ((millis() - start) < timeout_ms) {
        if (ble_rx_ready) {
            ble_rx_buf[ble_rx_len] = 0;
            /* Check if expected string is in the response */
            const char *e = expected;
            const char *b = ble_rx_buf;
            while (*b) {
                const char *m = e;
                const char *s = b;
                while (*s && *m && *s == *m) { s++; m++; }
                if (*m == 0) return 0;  /* found */
                b++;
            }
            ble_rx_ready = 0;
            ble_rx_len = 0;
        }
    }
    return -1;  /* timeout */
}

static void ble_reset_module(void) {
    gpio_clr(GPIO(GPIOD), BLE_RESET_PIN);
    delay_ms(10);
    gpio_set(GPIO(GPIOD), BLE_RESET_PIN);
    delay_ms(500);  /* wait for module boot */
}

/* ── Public API ───────────────────────────────────────────────────── */

int ble_init(void) {
    ble_uart_init();
    ble_reset_module();

    /* Test AT communication */
    ble_send_at("AT");
    if (ble_wait_response("OK", 2000) != 0) {
        return -1;  /* module not responding */
    }

    /* Set device name */
    ble_send_at("AT+NAME=TeslaPhantom");
    ble_wait_response("OK", 1000);

    /* Configure custom GATT service
     * Service UUID: 0000TES0-0000-1000-8000-00805F9B34FB
     * (Simplified — actual NINA-B3 uses specific AT commands) */
    ble_send_at("AT+SERVTX=0000TES0");
    ble_wait_response("OK", 1000);

    /* Configure characteristics */
    ble_send_at("AT+CHRTX=0001");  /* Command characteristic */
    ble_wait_response("OK", 1000);
    ble_send_at("AT+CHRTX=0002");  /* Status characteristic */
    ble_wait_response("OK", 1000);
    ble_send_at("AT+CHRTX=0003");  /* Trace data characteristic */
    ble_wait_response("OK", 1000);
    ble_send_at("AT+CHRTX=0004");  /* Scan map characteristic */
    ble_wait_response("OK", 1000);

    /* Start advertising */
    ble_send_at("AT+ADVSTART");
    ble_wait_response("OK", 1000);

    return 0;
}

int ble_send_status(uint8_t state, uint8_t battery, const char *info) {
    /* Format: {"st":N,"bat":N,"info":"..."} */
    char buf[128];
    int idx = 0;

    /* Manually build JSON string (no libc sprintf) */
    buf[idx++] = '{';
    buf[idx++] = '"';
    buf[idx++] = 's';
    buf[idx++] = 't';
    buf[idx++] = '"';
    buf[idx++] = ':';
    /* state number */
    if (state >= 100) { buf[idx++] = '0'+state/100; state %= 100; }
    if (state >= 10)  { buf[idx++] = '0'+state/10;  state %= 10; }
    buf[idx++] = '0' + state;
    buf[idx++] = ',';
    buf[idx++] = '"';
    buf[idx++] = 'b';
    buf[idx++] = 'a';
    buf[idx++] = 't';
    buf[idx++] = '"';
    buf[idx++] = ':';
    if (battery >= 100) { buf[idx++] = '1'; buf[idx++] = '0'; buf[idx++] = '0'; }
    else if (battery >= 10) {
        buf[idx++] = '0' + battery / 10;
        buf[idx++] = '0' + battery % 10;
    } else {
        buf[idx++] = '0' + battery;
    }
    buf[idx++] = ',';
    buf[idx++] = '"';
    buf[idx++] = 'i';
    buf[idx++] = '"';
    buf[idx++] = ':';
    buf[idx++] = '"';
    if (info) {
        while (*info && idx < 110) buf[idx++] = *info++;
    }
    buf[idx++] = '"';
    buf[idx++] = '}';
    buf[idx] = 0;

    /* Send via BLE notify (AT+NOTI=0002,<data>) */
    ble_send_at("AT+NOTI=0002");
    for (int i = 0; i < idx; i++)
        usart_write(USART3, (uint8_t)buf[i]);
    usart_write(USART3, '\r');
    usart_write(USART3, '\n');

    return 0;
}

int ble_send_trace_chunk(const void *data, uint16_t len) {
    if (len > BLE_MAX_PAYLOAD) len = BLE_MAX_PAYLOAD;

    /* Send binary data as base64 or raw via transparent mode */
    ble_send_at("AT+NOTI=0003");
    const uint8_t *p = (const uint8_t *)data;
    for (uint16_t i = 0; i < len; i++)
        usart_write(USART3, p[i]);
    usart_write(USART3, '\r');
    usart_write(USART3, '\n');

    return 0;
}

int ble_send_scan_map(const void *data, uint16_t len) {
    if (len > BLE_MAX_PAYLOAD) len = BLE_MAX_PAYLOAD;

    ble_send_at("AT+NOTI=0004");
    const uint8_t *p = (const uint8_t *)data;
    for (uint16_t i = 0; i < len; i++)
        usart_write(USART3, p[i]);
    usart_write(USART3, '\r');
    usart_write(USART3, '\n');

    return 0;
}

int ble_recv_cmd(uint8_t *cmd, void *payload, uint16_t *len) {
    if (!ble_rx_ready) return 0;

    /* Parse received data as command.
     * In transparent mode, received data from the command characteristic
     * comes as raw bytes. We check for JSON format. */
    if (ble_rx_len > 0) {
        /* Copy to payload buffer */
        uint16_t copy_len = ble_rx_len;
        if (copy_len > *len) copy_len = *len;
        for (uint16_t i = 0; i < copy_len; i++)
            ((uint8_t *)payload)[i] = (uint8_t)ble_rx_buf[i];
        *len = copy_len;
        *cmd = 1;  /* generic command */

        ble_rx_ready = 0;
        ble_rx_len = 0;
        return 1;
    }

    return 0;
}

int ble_authenticate(const uint8_t *psk, uint32_t len) {
    /* Configure BLE pairing with passkey */
    (void)psk;
    (void)len;
    /* The NINA-B306 handles authentication internally.
     * We would set the passkey via AT command. */
    ble_send_at("AT+AUTH=LE Secure");
    return ble_wait_response("OK", 1000);
}

void ble_process(void) {
    /* Process any pending BLE events.
     * In the interrupt-driven model, the RX ISR fills the buffer.
     * Here we just check if a complete message has been received. */
    if (ble_rx_ready) {
        /* Check for AT response vs data */
        if (ble_rx_len > 0 && ble_rx_buf[0] == '{') {
            /* JSON command — will be processed by main loop */
        }
    }
}

/* USART3 interrupt handler — receives BLE data */
void usart3_irqhandler(void) {
    volatile usart_regs_t *u = USART3;

    while (u->ISR & USART_ISR_RXNE) {
        uint8_t ch = (uint8_t)u->RDR;
        if (ble_rx_len < sizeof(ble_rx_buf) - 1) {
            ble_rx_buf[ble_rx_len++] = (char)ch;
        }
        /* Check for end of message (newline or JSON closing brace) */
        if (ch == '\n' || ch == '}') {
            ble_rx_ready = 1;
        }
    }

    /* Clear any error flags */
    if (u->ISR & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE)) {
        volatile uint32_t *icr = (volatile uint32_t *)((uint32_t)u + 0x20);
        *icr = USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE;
    }
}