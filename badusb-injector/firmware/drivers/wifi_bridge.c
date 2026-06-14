/*
 * PHANTOM — WiFi Bridge Driver Implementation
 * ESP32-C3-MINI-1 AT Command Interface via UART
 *
 * Copyright (C) 2024 Hacker Devices
 * Licensed under GPL-2.0
 *
 * Uses ESP32-C3-MINI-1 in AT-command mode for WiFi scanning,
 * network connection, geofencing, BLE control, and HTTP server.
 */

#include "wifi_bridge.h"
#include "registers.h"
#include "board.h"

/* ============================================================
 * UART Driver (RP2040 UART0 to ESP32-C3)
 * ============================================================ */

static volatile char g_uart_rx_buf[AT_RESPONSE_BUF_SIZE];
static volatile uint32_t g_uart_rx_head = 0;
static volatile uint32_t g_uart_rx_tail = 0;

static void uart_init(void) {
    /* Configure UART0: 115200 8N1 */
    UART0_IBRD = 69;   /* 133MHz / (16 * 115200) = 72.05... */
    UART0_FBRD = 1;    /* Fractional divider: 0.052 * 64 = 3.3 ≈ 1 */
    /* Actually: For 133MHz peripheral clock:
     * Baud rate divisor = 133000000 / (16 * 115200) = 72.05
     * IBRD = 72, FBRD = floor(0.052 * 64 + 0.5) = 3
     */
    UART0_IBRD = 72;
    UART0_FBRD = 3;

    /* 8N1, FIFO enabled */
    UART0_LCR_H = UART_LCR_H_WLEN_8 | UART_LCR_H_FEN;

    /* Enable TX, RX */
    UART0_CR = UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE;

    /* Set RX FIFO interrupt level at 1/8 */
    UART0_IFLS = 0x00;  /* RX FIFO interrupt at 1/8 full */

    /* Enable RX interrupt */
    UART0_IMSC = (1 << 4);  /* RX interrupt */
}

static void uart_putc(char c) {
    while (UART0_FR & UART_FR_TXFF);  /* Wait for TX FIFO space */
    UART0_DR = c;
}

static void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

static int uart_getc(void) {
    if (g_uart_rx_head != g_uart_rx_tail) {
        char c = g_uart_rx_buf[g_uart_rx_tail];
        g_uart_rx_tail = (g_uart_rx_tail + 1) % AT_RESPONSE_BUF_SIZE;
        return c;
    }
    return -1;
}

static uint32_t uart_available(void) {
    return (g_uart_rx_head - g_uart_rx_tail) % AT_RESPONSE_BUF_SIZE;
}

/* UART0 interrupt handler */
void __attribute__((interrupt)) uart0_isr(void) {
    while (!(UART0_FR & UART_FR_RXFE)) {
        char c = (char)(UART0_DR & 0xFF);
        g_uart_rx_buf[g_uart_rx_head] = c;
        g_uart_rx_head = (g_uart_rx_head + 1) % AT_RESPONSE_BUF_SIZE;
    }
    UART0_ICR = 0x7FF;  /* Clear all interrupts */
}

/* ============================================================
 * AT Command Interface
 * ============================================================ */

/* Send AT command and wait for response */
int wifi_send_at_cmd(const char *cmd, char *response, uint32_t response_size, uint32_t timeout_ms) {
    /* Flush RX buffer */
    g_uart_rx_head = 0;
    g_uart_rx_tail = 0;

    /* Send AT command (append \r\n) */
    uart_puts(cmd);
    uart_puts("\r\n");

    if (!response) return 0;

    /* Read response until timeout or OK/ERROR */
    uint32_t start = time_ms();
    uint32_t pos = 0;
    bool found_ok = false;
    bool found_error = false;

    while ((time_ms() - start) < timeout_ms && pos < response_size - 1) {
        int c = uart_getc();
        if (c >= 0) {
            response[pos++] = (char)c;
            response[pos] = '\0';

            /* Check for OK or ERROR */
            if (pos >= 4 && strstr(response, "\r\nOK\r\n")) {
                found_ok = true;
                break;
            }
            if (pos >= 7 && strstr(response, "\r\nERROR")) {
                found_error = true;
                break;
            }
        } else {
            /* No data available, brief wait */
            busy_wait_us(100);
        }
    }

    response[pos] = '\0';

    if (found_ok) return 0;
    if (found_error) return -2;
    if (pos > 0) return 1;  /* Got data but no terminator */
    return -1;  /* Timeout */
}

/* ============================================================
 * WiFi Bridge Initialization
 * ============================================================ */

int wifi_bridge_init(void) {
    /* Initialize UART0 */
    uart_init();

    /* Reset ESP32-C3 */
    SIO_GPIO_OUT_CLR = (1 << PIN_ESP_RST_N);  /* Assert reset */
    busy_wait_ms(100);
    SIO_GPIO_OUT_SET = (1 << PIN_ESP_RST_N);  /* Release reset */
    busy_wait_ms(3000);  /* Wait for ESP32-C3 boot */

    /* Test AT communication */
    int ret = wifi_send_at_cmd("AT", NULL, 0, 2000);
    if (ret != 0) return -1;

    /* Disable echo */
    wifi_send_at_cmd("ATE0", NULL, 0, 1000);

    /* Set station mode */
    wifi_set_mode(WIFI_MODE_STA);

    /* Enable multiple connections */
    wifi_send_at_cmd("AT+CIPMUX=1", NULL, 0, 1000);

    return 0;
}

/* ============================================================
 * WiFi Operations
 * ============================================================ */

int wifi_set_mode(wifi_mode_t mode) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CWMODE=%d", (int)mode);
    return wifi_send_at_cmd(cmd, NULL, 0, 1000);
}

int wifi_connect(const char *ssid, const char *password) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, password);
    return wifi_send_at_cmd(cmd, NULL, 0, WIFI_CONNECT_TIMEOUT_MS);
}

int wifi_disconnect(void) {
    return wifi_send_at_cmd("AT+CWQAP", NULL, 0, 2000);
}

int wifi_scan(wifi_scan_result_t *results, uint8_t max_results) {
    char response[2048];
    int ret = wifi_send_at_cmd("AT+CWLAP", response, sizeof(response), WIFI_SCAN_TIMEOUT_MS);
    if (ret < 0) return ret;

    /* Parse AT+CWLAP response
     * Format: +CWLAP:<ecn>,<ssid>,<rssi>,<mac>,<ch>,<freq offset>,<freq calibration>
     */
    uint8_t count = 0;
    char *line = response;

    while (count < max_results && (line = strstr(line, "+CWLAP:")) != NULL) {
        line += 7;  /* Skip "+CWLAP:" */

        /* Parse encryption type */
        int enc = 0;
        char *p = line;
        while (*p && *p != ',') p++;
        if (*p == ',') {
            *p = '\0';
            enc = atoi(line);
            p++;
        }

        /* Parse SSID */
        char *ssid_start = NULL;
        if (*p == '"') {
            ssid_start = p + 1;
            char *ssid_end = strchr(ssid_start, '"');
            if (ssid_end) {
                int ssid_len = ssid_end - ssid_start;
                if (ssid_len > 32) ssid_len = 32;
                memcpy(results[count].ssid, ssid_start, ssid_len);
                results[count].ssid[ssid_len] = '\0';
                p = ssid_end + 2;  /* Skip ", */
            }
        }

        /* Parse RSSI */
        results[count].rssi = atoi(p);
        while (*p && *p != ',') p++;
        if (*p == ',') p++;

        /* Parse BSSID */
        char *mac_start = NULL;
        if (*p == '"') {
            mac_start = p + 1;
            char *mac_end = strchr(mac_start, '"');
            if (mac_end) {
                sscanf(mac_start, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                       &results[count].bssid[0], &results[count].bssid[1],
                       &results[count].bssid[2], &results[count].bssid[3],
                       &results[count].bssid[4], &results[count].bssid[5]);
                p = mac_end + 2;
            }
        }

        /* Parse channel */
        while (*p && *p != ',') p++;
        if (*p == ',') p++;
        results[count].channel = atoi(p);

        results[count].encryption = enc;
        count++;

        /* Move to next line */
        line = strchr(p, '\n');
        if (line) line++;
    }

    return count;
}

bool wifi_scan_for_ssid(const char *target_ssid) {
    wifi_scan_result_t results[16];
    int count = wifi_scan(results, 16);

    for (int i = 0; i < count; i++) {
        if (strcmp(results[i].ssid, target_ssid) == 0) {
            return true;
        }
    }
    return false;
}

int wifi_start_ap(const char *ssid, const char *password, uint8_t channel) {
    char cmd[128];

    /* Switch to AP mode */
    wifi_set_mode(WIFI_MODE_AP);

    /* Configure AP */
    snprintf(cmd, sizeof(cmd), "AT+CWSAP=\"%s\",\"%s\",%d,3",
             ssid, password, channel);
    return wifi_send_at_cmd(cmd, NULL, 0, 2000);
}

int wifi_start_http_server(uint16_t port) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CIPSERVER=1,%d", port);
    return wifi_send_at_cmd(cmd, NULL, 0, 2000);
}

int wifi_send_http_response(int conn_id, const char *data, uint32_t len) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d", conn_id, len);

    int ret = wifi_send_at_cmd(cmd, NULL, 0, 1000);
    if (ret != 0) return ret;

    /* Send data */
    for (uint32_t i = 0; i < len; i++) {
        uart_putc(data[i]);
    }

    return 0;
}

int wifi_close_connection(int conn_id) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%d", conn_id);
    return wifi_send_at_cmd(cmd, NULL, 0, 1000);
}

wifi_status_t wifi_get_status(void) {
    wifi_status_t status = {0};
    char response[256];

    /* Query connection status */
    int ret = wifi_send_at_cmd("AT+CIPSTATUS", response, sizeof(response), 2000);
    if (ret == 0) {
        /* Parse STATUS response */
        if (strstr(response, "STATUS:3")) {
            status.connected = true;
            /* Parse IP and SSID from response */
            char *ip_start = strstr(response, "ip:");
            if (ip_start) {
                ip_start += 3;
                char *ip_end = ip_start;
                while (*ip_end && *ip_end != '"' && *ip_end != ',') ip_end++;
                int ip_len = ip_end - ip_start;
                if (ip_len > 15) ip_len = 15;
                memcpy(status.ip_addr, ip_start, ip_len);
                status.ip_addr[ip_len] = '\0';
            }
        }
    }

    return status;
}

int wifi_reset(void) {
    SIO_GPIO_OUT_CLR = (1 << PIN_ESP_RST_N);
    busy_wait_ms(100);
    SIO_GPIO_OUT_SET = (1 << PIN_ESP_RST_N);
    busy_wait_ms(3000);

    /* Re-initialize */
    return wifi_bridge_init();
}

/* ============================================================
 * BLE Functions
 * ============================================================ */

int ble_start_advertising(const char *device_name) {
    char cmd[128];

    /* Set device name */
    snprintf(cmd, sizeof(cmd), "AT+BLENAME=\"%s\"", device_name);
    wifi_send_at_cmd(cmd, NULL, 0, 1000);

    /* Set BLE server parameters */
    wifi_send_at_cmd("AT+BLEINIT=1", NULL, 0, 1000);  /* Init as server */
    wifi_send_at_cmd("AT+BLEGATTSSRVCRE=1", NULL, 0, 1000);  /* Create service */
    wifi_send_at_cmd("AT+BLEGATTSCHARCFG=1,1,1,0x0052,0x0001,1,0", NULL, 0, 1000);  /* Control char */
    wifi_send_at_cmd("AT+BLEGATTSCHARCFG=1,2,1,0x0053,0x0002,1,0", NULL, 0, 1000);  /* Data char */
    wifi_send_at_cmd("AT+BLEGATTSCHARCFG=1,3,1,0x0054,0x0003,1,0", NULL, 0, 1000);  /* Config char */

    /* Start advertising */
    snprintf(cmd, sizeof(cmd), "AT+BLEADVSTART");
    wifi_send_at_cmd(cmd, NULL, 0, 1000);

    return 0;
}

int ble_stop_advertising(void) {
    return wifi_send_at_cmd("AT+BLEADVSTOP", NULL, 0, 1000);
}

int ble_send_notification(uint16_t handle, const uint8_t *data, uint16_t len) {
    char cmd[64];
    char hex_data[256 * 2 + 1];

    /* Convert data to hex string */
    for (uint16_t i = 0; i < len && i < 128; i++) {
        snprintf(hex_data + i * 2, 3, "%02X", data[i]);
    }

    snprintf(cmd, sizeof(cmd), "AT+BLEGATTSNTFY=1,%d,%d", handle, len);
    wifi_send_at_cmd(cmd, NULL, 0, 1000);

    /* Send hex data */
    wifi_send_at_cmd(hex_data, NULL, 0, 1000);

    return 0;
}

int ble_receive_data(uint8_t *data, uint16_t *len) {
    /* Check for incoming BLE data in UART buffer */
    /* AT+BLEGATTSCHAR? returns characteristic values */

    *len = 0;
    uint32_t start = time_ms();
    char response[256];

    int ret = wifi_send_at_cmd("AT+BLEGATTSCHAR?", response, sizeof(response), 500);
    if (ret < 0) return ret;

    /* Parse response for characteristic data */
    char *char_start = strstr(response, "+BLEGATTSCHAR:");
    if (!char_start) return 0;

    /* Format: +BLEGATTSCHAR:<conn>,<srv>,<char>,<len>,<data> */
    /* Find data portion */
    char *data_start = strstr(char_start, ",\"");
    if (!data_start) return 0;

    data_start += 2;  /* Skip ," */
    char *data_end = strchr(data_start, '"');
    if (!data_end) return 0;

    /* Convert hex data to bytes */
    int hex_len = data_end - data_start;
    for (int i = 0; i < hex_len / 2 && *len < 256; i++) {
        char hex_byte[3] = {data_start[i*2], data_start[i*2+1], '\0'};
        data[*len] = (uint8_t)strtol(hex_byte, NULL, 16);
        (*len)++;
    }

    return 0;
}

/* ============================================================
 * WiFi Bridge Task (called from main loop)
 * ============================================================ */

void wifi_bridge_task(void) {
    /* Process incoming BLE data */
    static char cmd_buf[256];
    static uint32_t cmd_pos = 0;
    static uint32_t last_wifi_check = 0;

    /* Check for incoming commands every 100ms */
    uint32_t now = time_ms();
    if ((now - last_wifi_check) < 100) return;
    last_wifi_check = now;

    /* Read any available data from UART */
    while (uart_available() > 0 && cmd_pos < sizeof(cmd_buf) - 1) {
        int c = uart_getc();
        if (c >= 0) {
            cmd_buf[cmd_pos++] = (char)c;
            cmd_buf[cmd_pos] = '\0';

            /* Check for complete command */
            if (cmd_pos >= 2 && cmd_buf[cmd_pos-2] == '\r' && cmd_buf[cmd_pos-1] == '\n') {
                /* Process command */
                if (strncmp(cmd_buf, "+BLEGATTSCHAR:", 14) == 0) {
                    /* BLE data received */
                    /* This will be handled by ble_receive_data() */
                }
                cmd_pos = 0;
            }
        }
    }
}