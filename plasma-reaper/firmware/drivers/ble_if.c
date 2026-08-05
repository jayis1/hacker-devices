/*
 * ble_if.c — BLE interface driver (STM32WB55 module)
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Communicates with the STM32WB55 BLE module over USART2 using
 * AT-style commands. The WB55 runs a pre-flashed firmware that
 * exposes a transparent serial-over-BLE GATT service, allowing
 * the host app (iOS) to send/receive command frames as if over
 * a serial port.
 *
 * The driver maintains RX/TX ring buffers and handles the AT
 * command responses from the WB55 module.
 */

#include "ble_if.h"
#include "../registers.h"

/* ---- Private state ------------------------------------------------- */

#define BLE_BUF_SIZE 512
#define BLE_AT_TIMEOUT_MS 1000

static uint8_t  g_rx_buf[BLE_BUF_SIZE];
static volatile uint16_t g_rx_head;
static volatile uint16_t g_rx_tail;
static volatile bool g_connected;
static volatile bool g_at_response_ready;
static char g_at_response[128];

/* ---- AT command helpers -------------------------------------------- */

static void ble_uart_tx_byte(uint8_t byte)
{
    while (!(USART2->ISR & USART_ISR_TXE))
        ;
    USART2->TDR = byte;
}

static void ble_uart_tx_string(const char *str)
{
    while (*str) {
        ble_uart_tx_byte((uint8_t)*str);
        str++;
    }
}

static bool ble_send_at_wait(const char *cmd)
{
    uint32_t start = 0; /* would use board_millis() */
    g_at_response_ready = false;

    ble_uart_tx_string(cmd);
    ble_uart_tx_byte('\r');
    ble_uart_tx_byte('\n');

    /* Wait for response (simplified — real implementation would
     * use a state machine in the ISR) */
    while (!g_at_response_ready) {
        /* timeout check would go here */
        start++;
        if (start > 1000000)
            return false;
    }

    /* Check if response contains "OK" */
    for (int i = 0; g_at_response[i] && i < 126; i++) {
        if (g_at_response[i] == 'O' && g_at_response[i + 1] == 'K')
            return true;
    }
    return false;
}

/* ---- Initialization ------------------------------------------------ */

void ble_if_init(void)
{
    /*
     * Configure USART2 on PD2 (TX) / PD3 (RX) for BLE module.
     * Baudrate: 115200 (WB55 AT command mode default).
     */
    gpio_config(GPIOD, BLE_UART_TX_PIN, GPIO_MODE_AF, GPIO_OSPEED_HIGH,
                GPIO_PUPD_NONE, 7);  /* AF7 for USART2 */
    gpio_config(GPIOD, BLE_UART_RX_PIN, GPIO_MODE_AF, GPIO_OSPEED_HIGH,
                GPIO_PUPD_PULLUP, 7);

    USART2->CR1 = 0;
    USART2->CR2 = 0;
    USART2->CR3 = 0;

    /* Baudrate: USART2 on APB1 (138 MHz) */
    USART2->BRR = 138000000UL / 115200UL;

    /* Enable USART with RX, TX, and RXNE interrupt */
    USART2->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE
                | USART_CR1_RXNEIE;

    /* Enable NVIC for USART2 */
    NVIC_ISER0 |= BIT(USART2_IRQn);

    g_rx_head = 0;
    g_rx_tail = 0;
    g_connected = false;
    g_at_response_ready = false;

    /*
     * Initialize the WB55 BLE module with AT commands:
     *  1. Reset module
     *  2. Set device name to "PlasmaReaper"
     *  3. Configure GATT service (custom UUID for serial-over-BLE)
     *  4. Start advertising
     */
    ble_send_at_wait("AT+RESET");
    ble_send_at_wait("AT+NAME=PlasmaReaper");
    ble_send_at_wait("AT+ADVSTART");
}

/* ---- Poll ---------------------------------------------------------- */

void ble_if_poll(void)
{
    /* Check for connection status changes from the WB55 */
    /* The WB55 sends unsolicited AT responses like "+CONNECT" and
     * "+DISCONNECT" which are handled in the USART2 ISR */
}

/* ---- RX / TX ------------------------------------------------------- */

uint32_t ble_if_rx(uint8_t *buf, uint32_t max_len)
{
    uint32_t count = 0;
    while (count < max_len && g_rx_tail != g_rx_head) {
        buf[count] = g_rx_buf[g_rx_tail];
        g_rx_tail = (g_rx_tail + 1) % BLE_BUF_SIZE;
        count++;
    }
    return count;
}

void ble_if_tx(const uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        ble_uart_tx_byte(buf[i]);
    }
}

bool ble_if_connected(void)
{
    return g_connected;
}

void ble_if_advertise(bool on)
{
    if (on)
        ble_send_at_wait("AT+ADVSTART");
    else
        ble_send_at_wait("AT+ADVSTOP");
}

/* ---- USART2 ISR ---------------------------------------------------- */

/*
 * The USART2 ISR is not defined here because the interrupt handler
 * name must match the startup file. In main.c, the USART2 interrupt
 * would be routed to a handler that processes incoming bytes from
 * the BLE module, distinguishing between AT responses and BLE data.
 */

/* ---- End of file --------------------------------------------------- */