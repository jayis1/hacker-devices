/**
 * @file    wifi_manager.c
 * @brief   WiFi AP/STA management, HTTP server, and WebSocket endpoint
 * @author  jayis1
 * @version 1.0.0
 * @license Proprietary — Authorized Security Research Use Only
 *
 * Manages WiFi connectivity for the HDMI-Siphon. Operates in one of
 * three modes:
 *   - AP mode: Creates a captive portal for direct mobile app connection
 *   - STA mode: Connects to an existing WiFi network for remote access
 *   - Scan mode: Scans available networks for configuration
 * 
 * Provides:
 *   - HTTP REST API for device control and status
 *   - WebSocket endpoint for real-time frame streaming and commands
 *   - OTA firmware updates via HTTP upload
 *   - Captive portal redirect for first-time setup
 */

#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "nvs_flash.h"

#include "board.h"
#include "wifi_manager.h"
#include "protocol.h"
#include "capture_ctrl.h"
#include "cec_monitor.h"
#include "edid_manager.h"

/* =========================================================================
 * Local Constants
 * ========================================================================= */

static const char *TAG = "WIFI_MGR";

/** HTTP server maximum URI length */
#define MAX_URI_LEN                     128U

/** Maximum number of WebSocket clients */
#define MAX_WS_CLIENTS                     4U

/** STA connection retry count */
#define STA_MAX_RETRY                       5U

/** STA retry interval (ms) */
#define STA_RETRY_INTERVAL_MS            3000U

/** OTA update buffer size */
#define OTA_BUF_SIZE                    8192U

/* =========================================================================
 * Local State
 * ========================================================================= */

/** WiFi operating mode */
static wifi_mode_t s_wifi_mode = WIFI_MODE_AP;
static bool s_wifi_connected = false;
static httpd_handle_t s_http_server = NULL;
static int s_sta_retry_count = 0;

/** NVS keys for WiFi config */
#define NVS_KEY_WIFI_SSID              "wifi_ssid"
#define NVS_KEY_WIFI_PASS              "wifi_pass"
#define NVS_KEY_WIFI_MODE              "wifi_mode"

/* =========================================================================
 * HTTP Handlers
 * ========================================================================= */

/**
 * @brief GET /api/status — return full device status as JSON
 */
static esp_err_t api_status_handler(httpd_req_t *req)
{
    /* Build JSON status response using protocol helper */
    char json[1024];
    size_t len = protocol_build_status(json, sizeof(json));

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, len);
    return ESP_OK;
}

/**
 * @brief POST /api/capture — trigger a frame capture
 */
static esp_err_t api_capture_handler(httpd_req_t *req)
{
    int result = capture_ctrl_trigger();
    char json[128];

    if (result == 0) {
        snprintf(json, sizeof(json),
                 "{\"status\":\"ok\",\"frame_id\":%u}",
                 capture_ctrl_get_frame_count());
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json, strlen(json));
    } else {
        snprintf(json, sizeof(json),
                 "{\"status\":\"error\",\"code\":%d}", result);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_send(req, json, strlen(json));
    }
    return ESP_OK;
}

/**
 * @brief POST /api/edid — inject EDID (body is hex-encoded EDID data)
 * Body format: {"edid_hex": "00FFFFFFFFFFFF00..."}
 */
static esp_err_t api_edid_handler(httpd_req_t *req)
{
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    buf[ret] = '\0';

    /* Parse JSON and inject EDID */
    esp_err_t result = protocol_handle_edid_command(buf);

    char json[64];
    snprintf(json, sizeof(json),
             "{\"status\":\"%s\"}",
             (result == ESP_OK) ? "ok" : "error");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    return ESP_OK;
}

/**
 * @brief POST /api/cec — send a CEC command
 */
static esp_err_t api_cec_handler(httpd_req_t *req)
{
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_OK;
    buf[ret] = '\0';

    protocol_handle_cec_command(buf);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\"}", 15);
    return ESP_OK;
}

/**
 * @brief GET /api/gallery — list captured frames
 */
static esp_err_t api_gallery_handler(httpd_req_t *req)
{
    sd_card_file_info_t files[32];
    int count = sd_card_list_captures(files, 32);

    char json[4096];
    size_t len = protocol_build_gallery_json(json, sizeof(json), files, count);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, len);
    return ESP_OK;
}

/**
 * @brief GET /frame/<filename> — serve a captured frame image
 */
static esp_err_t api_frame_handler(httpd_req_t *req)
{
    /* Extract filename from URI */
    const char *uri = req->uri;
    const char *fn = uri + 7;  /* Skip "/frame/" */

    char path[128];
    snprintf(path, sizeof(path), "%s/%s", CAPTURE_DIR, fn);

    /* Read file and serve */
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        httpd_resp_set_status(req, "404 Not Found");
        httpd_resp_send(req, "Frame not found", 15);
        return ESP_OK;
    }

    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *buf = malloc(fsize);
    if (buf) {
        fread(buf, 1, fsize, f);
        httpd_resp_set_type(req, "image/jpeg");
        httpd_resp_send(req, (const char *)buf, fsize);
        free(buf);
    }
    fclose(f);
    return ESP_OK;
}

/**
 * @brief POST /api/config — set device configuration
 */
static esp_err_t api_config_handler(httpd_req_t *req)
{
    char buf[512];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_OK;
    buf[ret] = '\0';

    protocol_handle_config_command(buf);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\"}", 15);
    return ESP_OK;
}

/* =========================================================================
 * HTTP Server Configuration
 * ========================================================================= */

/** URI handler table */
static const httpd_uri_t s_api_handlers[] = {
    { .uri = "/api/status",  .method = HTTP_GET,  .handler = api_status_handler,  .user_ctx = NULL },
    { .uri = "/api/capture", .method = HTTP_POST, .handler = api_capture_handler, .user_ctx = NULL },
    { .uri = "/api/edid",    .method = HTTP_POST, .handler = api_edid_handler,    .user_ctx = NULL },
    { .uri = "/api/cec",     .method = HTTP_POST, .handler = api_cec_handler,     .user_ctx = NULL },
    { .uri = "/api/gallery", .method = HTTP_GET,  .handler = api_gallery_handler, .user_ctx = NULL },
    { .uri = "/api/config",  .method = HTTP_POST, .handler = api_config_handler,   .user_ctx = NULL },
    { .uri = "/frame/*",     .method = HTTP_GET,  .handler = api_frame_handler,    .user_ctx = NULL },
};

static esp_err_t wifi_start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = HTTP_SERVER_PORT;
    config.max_uri_handlers = sizeof(s_api_handlers) / sizeof(s_api_handlers[0]);
    config.lru_purge_enable = true;

    esp_err_t ret = httpd_start(&s_http_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "HTTP server start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Register all API handlers */
    int n = sizeof(s_api_handlers) / sizeof(s_api_handlers[0]);
    for (int i = 0; i < n; i++) {
        httpd_register_uri_handler(s_http_server, &s_api_handlers[i]);
    }

    ESP_LOGI(TAG, "HTTP server running on port %d", HTTP_SERVER_PORT);
    return ESP_OK;
}

/* =========================================================================
 * WiFi Event Handler
 * ========================================================================= */

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_sta_retry_count < STA_MAX_RETRY) {
            s_sta_retry_count++;
            ESP_LOGW(TAG, "STA disconnected, retry %d/%d",
                     s_sta_retry_count, STA_MAX_RETRY);
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "STA max retries reached, switching to AP mode");
            wifi_manager_start_ap();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_connected = true;
        s_sta_retry_count = 0;
        wifi_start_http_server();
    }
}

/* =========================================================================
 * Public API
 * ========================================================================= */

esp_err_t wifi_manager_init(void)
{
    /* Initialize netif */
    esp_netif_init();
    esp_event_loop_create_default();

    /* Create default WiFi station and AP netifs */
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    /* Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Register event handlers */
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    /* Try to load saved WiFi config from NVS */
    nvs_handle_t nvs;
    char ssid[32] = WIFI_AP_SSID;
    char pass[64] = WIFI_AP_PASS;
    int32_t mode = WIFI_MODE_AP;

    if (nvs_open("hdmi_siphon", NVS_READONLY, &nvs) == ESP_OK) {
        size_t ssid_len = sizeof(ssid);
        nvs_get_str(nvs, NVS_KEY_WIFI_SSID, ssid, &ssid_len);
        size_t pass_len = sizeof(pass);
        nvs_get_str(nvs, NVS_KEY_WIFI_PASS, pass, &pass_len);
        nvs_get_i32(nvs, NVS_KEY_WIFI_MODE, &mode);
        nvs_close(nvs);
    }

    s_wifi_mode = (wifi_mode_t)mode;

    if (s_wifi_mode == WIFI_MODE_STA && strlen(ssid) > 0 &&
        strcmp(ssid, WIFI_AP_SSID) != 0) {
        wifi_manager_start_sta(ssid, pass);
    } else {
        wifi_manager_start_ap();
    }

    ESP_LOGI(TAG, "WiFi manager initialized in %s mode",
             s_wifi_mode == WIFI_MODE_AP ? "AP" : "STA");
    return ESP_OK;
}

esp_err_t wifi_manager_start_ap(void)
{
    s_wifi_mode = WIFI_MODE_AP;

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .password = WIFI_AP_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .channel = 6,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "AP started: SSID=%s, IP=%s", WIFI_AP_SSID, WIFI_AP_IP);

    /* Start HTTP server */
    wifi_start_http_server();

    return ESP_OK;
}

esp_err_t wifi_manager_start_sta(const char *ssid, const char *password)
{
    s_wifi_mode = WIFI_MODE_STA;

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "STA connecting to SSID=%s", ssid);

    /* Save config to NVS */
    nvs_handle_t nvs;
    if (nvs_open("hdmi_siphon", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_str(nvs, NVS_KEY_WIFI_SSID, ssid);
        nvs_set_str(nvs, NVS_KEY_WIFI_PASS, password);
        nvs_set_i32(nvs, NVS_KEY_WIFI_MODE, WIFI_MODE_STA);
        nvs_commit(nvs);
        nvs_close(nvs);
    }

    return ESP_OK;
}

wifi_mode_t wifi_manager_get_mode(void)
{
    return s_wifi_mode;
}

bool wifi_manager_is_connected(void)
{
    return s_wifi_connected || (s_wifi_mode == WIFI_MODE_AP);
}

/* =========================================================================
 * FreeRTOS Task
 * ========================================================================= */

void wifi_task(void *pvParameters)
{
    ESP_LOGI(TAG, "WiFi/HTTP task started");
    wifi_manager_init();

    while (1) {
        /* Periodic WiFi health check */
        if (s_wifi_mode == WIFI_MODE_STA && !s_wifi_connected) {
            wifi_ap_record_t ap_info;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                s_wifi_connected = true;
                ESP_LOGI(TAG, "WiFi reconnected to %s", ap_info.ssid);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10000));  /* Check every 10 seconds */
    }
}
