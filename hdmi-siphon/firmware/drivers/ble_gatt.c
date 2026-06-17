/**
 * @file    ble_gatt.c
 * @brief   BLE GATT service for companion app control
 * @author  jayis1
 * @version 1.0.0
 * @license Proprietary — Authorized Security Research Use Only
 *
 * Implements a BLE GATT server that advertises the HDMI-Siphon service.
 * Provides characteristics for command/control, status notifications,
 * and frame data transfer as an alternative to WiFi-based control.
 */

#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"

#include "board.h"
#include "ble_gatt.h"
#include "protocol.h"
#include "capture_ctrl.h"

/* =========================================================================
 * Local Constants
 * ========================================================================= */

static const char *TAG = "BLE_GATT";

/** BLE advertisement parameters */
#define BLE_ADV_DURATION_SEC             180U
#define BLE_ADV_INTERVAL_MS               100U

/** Maximum BLE MTU size */
#define BLE_MTU_MAX                       512U

/** Maximum number of BLE clients */
#define BLE_MAX_CLIENTS                     1U

/* =========================================================================
 * Local State
 * ========================================================================= */

static bool s_ble_initialized = false;
static uint16_t s_gatts_if = 0;
static uint16_t s_app_id = 0;
static uint16_t s_conn_id = 0;
static uint16_t s_cmd_char_handle = 0;
static uint16_t s_status_char_handle = 0;
static bool s_connected = false;

/* =========================================================================
 * GATT Attribute Table
 * ========================================================================= */

#define GATTS_SERVICE_UUID_HDMI_SIPHON  0x00FF
#define GATTS_CHAR_UUID_CMD             0xFF01
#define GATTS_CHAR_UUID_STATUS          0xFF02
#define GATTS_CHAR_UUID_FRAME           0xFF03

#define GATTS_DEMO_CHAR_VAL_LEN_MAX     0x40

/** Device name for BLE advertising */
static const char s_device_name[] = "HDMI-Siphon";

/** Full 128-bit UUID for HDMI-Siphon service */
static const uint8_t s_service_uuid128[16] = {
    0x6A, 0x4E, 0x9C, 0x80, 0x1B, 0x3C, 0x11, 0xEF,
    0xA3, 0x5B, 0x2B, 0x1A, 0x5C, 0x5A, 0x8B, 0x6C
};

/** GATT attribute table (service, characteristics, descriptors) */
static const uint16_t s_primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t s_char_decl_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t s_char_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t s_char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t s_char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

/** Full attribute database */
static esp_gatts_attr_db_t s_gatt_db[7] = {
    /* Service Declaration */
    [0] = {
        .attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&s_primary_service_uuid,
            .perm = ESP_GATT_PERM_READ,
            .max_length = ESP_UUID_LEN_128,
            .length = ESP_UUID_LEN_128,
            .value = (uint8_t *)s_service_uuid128
        }
    },
    /* Command Characteristic Declaration */
    [1] = {
        .attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&s_char_decl_uuid,
            .perm = ESP_GATT_PERM_READ,
            .max_length = 1,
            .length = 1,
            .value = (uint8_t *)&s_char_prop_read_write
        }
    },
    /* Command Characteristic Value */
    [2] = {
        .attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&GATTS_CHAR_UUID_CMD,
            .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
            .max_length = GATTS_DEMO_CHAR_VAL_LEN_MAX,
            .length = 4,
            .value = (uint8_t *)"ready"
        }
    },
    /* Status Characteristic Declaration */
    [3] = {
        .attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&s_char_decl_uuid,
            .perm = ESP_GATT_PERM_READ,
            .max_length = 1,
            .length = 1,
            .value = (uint8_t *)&s_char_prop_read_notify
        }
    },
    /* Status Characteristic Value */
    [4] = {
        .attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&GATTS_CHAR_UUID_STATUS,
            .perm = ESP_GATT_PERM_READ,
            .max_length = GATTS_DEMO_CHAR_VAL_LEN_MAX,
            .length = 4,
            .value = (uint8_t *)"idle"
        }
    },
    /* Client Characteristic Configuration (for notifications) */
    [5] = {
        .attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&s_char_client_config_uuid,
            .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
            .max_length = 2,
            .length = 2,
            .value = (uint8_t *)"\x00\x00"
        }
    },
    /* Frame Transfer Characteristic */
    [6] = {
        .attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&GATTS_CHAR_UUID_FRAME,
            .perm = ESP_GATT_PERM_READ,
            .max_length = GATTS_DEMO_CHAR_VAL_LEN_MAX,
            .length = 4,
            .value = (uint8_t *)"none"
        }
    },
};

/* =========================================================================
 * BLE GATT Event Handler
 * ========================================================================= */

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                 esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:
            s_gatts_if = gatts_if;
            ESP_LOGI(TAG, "GATT registered, app_id=%d", param->reg.app_id);

            /* Create attribute database */
            esp_ble_gatts_create_attr_tab(s_gatt_db, gatts_if,
                                           sizeof(s_gatt_db) / sizeof(s_gatt_db[0]), 0);
            break;

        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
            ESP_LOGI(TAG, "Attribute table created, status=%d", param->add_attr_tab.status);
            if (param->add_attr_tab.num_handle > 2) {
                s_cmd_char_handle = param->add_attr_tab.handles[2];
                s_status_char_handle = param->add_attr_tab.handles[4];
            }

            /* Start service */
            esp_ble_gatts_start_service(param->add_attr_tab.handles[0]);
            break;

        case ESP_GATTS_START_EVT:
            ESP_LOGI(TAG, "Service started");

            /* Set advertisement data */
            esp_ble_adv_data_t adv_data = {
                .set_scan_rsp = false,
                .include_name = true,
                .include_txpower = true,
                .min_interval = 0x20,
                .max_interval = 0x40,
                .appearance = 0x00,
                .manufacturer_len = 0,
                .p_manufacturer_data = NULL,
                .service_data_len = 0,
                .p_service_data = NULL,
                .service_uuid_len = 16,
                .p_service_uuid = s_service_uuid128,
                .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
            };
            esp_ble_gap_config_adv_data(&adv_data);

            /* Start advertising */
            esp_ble_adv_params_t adv_params = {
                .adv_int_min = 0x20,
                .adv_int_max = 0x40,
                .adv_type = ADV_TYPE_IND,
                .channel_map = ADV_CHNL_ALL,
                .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
                .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
                .peer_addr = {0},
                .scan_rsp_undirected = true,
            };
            esp_ble_gap_start_advertising(&adv_params);
            ESP_LOGI(TAG, "BLE advertising started as '%s'", s_device_name);
            break;

        case ESP_GATTS_CONNECT_EVT:
            s_conn_id = param->connect.conn_id;
            s_connected = true;
            ESP_LOGI(TAG, "BLE client connected, conn_id=%d", s_conn_id);

            /* Update connection parameters for lower latency */
            esp_ble_conn_update_params_t conn_params = {
                .latency = 0,
                .min_int = 0x06,   /* 7.5 ms */
                .max_int = 0x0C,   /* 15 ms */
                .timeout = 400,    /* 4 seconds */
            };
            esp_ble_gap_update_conn_params(&conn_params);
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            s_connected = false;
            ESP_LOGI(TAG, "BLE client disconnected");

            /* Restart advertising */
            esp_ble_gap_start_advertising(NULL);
            break;

        case ESP_GATTS_WRITE_EVT: {
            if (param->write.handle == s_cmd_char_handle) {
                /* Command received — process it */
                const char *cmd = (const char *)param->write.value;
                uint16_t cmd_len = param->write.len;

                /* Null-terminate the command */
                char cmd_str[256];
                uint16_t copy_len = (cmd_len < sizeof(cmd_str) - 1) ? cmd_len : sizeof(cmd_str) - 1;
                memcpy(cmd_str, cmd, copy_len);
                cmd_str[copy_len] = '\0';

                ESP_LOGI(TAG, "BLE command: %s", cmd_str);

                /* Execute command and get response */
                char response[512];
                protocol_parse_and_execute(cmd_str, response, sizeof(response));

                /* Write response to status characteristic */
                esp_ble_gatts_set_attr_value(s_status_char_handle,
                                              strlen(response), (uint8_t *)response);

                /* Notify client of status update */
                esp_ble_gatts_send_indicate(s_gatts_if, s_conn_id,
                                             s_status_char_handle,
                                             strlen(response), (uint8_t *)response, false);
            }
            break;
        }

        case ESP_GATTS_READ_EVT:
            /* Handle read requests (if needed) */
            break;

        default:
            break;
    }
}

/* =========================================================================
 * GAP Event Handler
 * ========================================================================= */

static void gap_event_handler(esp_gap_ble_cb_event_t event,
                               esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(NULL);
            break;
        default:
            break;
    }
}

/* =========================================================================
 * Public API
 * ========================================================================= */

esp_err_t ble_gatt_init(void)
{
    ESP_LOGI(TAG, "Initializing BLE GATT service");

    esp_err_t ret;

    /* Initialize Bluetooth controller */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Initialize Bluedroid stack */
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Register GATT and GAP callbacks */
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GATTS callback register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GAP callback register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Register application */
    ret = esp_ble_gatts_app_register(0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GATTS app register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Set device name */
    esp_ble_gap_set_device_name(s_device_name);

    s_ble_initialized = true;
    ESP_LOGI(TAG, "BLE GATT service initialized");
    return ESP_OK;
}

bool ble_gatt_is_connected(void)
{
    return s_connected;
}

esp_err_t ble_gatt_send_status(const char *json_status)
{
    if (!s_connected) return ESP_ERR_INVALID_STATE;

    esp_ble_gatts_set_attr_value(s_status_char_handle,
                                  strlen(json_status), (uint8_t *)json_status);
    esp_ble_gatts_send_indicate(s_gatts_if, s_conn_id,
                                 s_status_char_handle,
                                 strlen(json_status), (uint8_t *)json_status, false);
    return ESP_OK;
}

esp_err_t ble_gatt_send_frame(const uint8_t *data, size_t len)
{
    if (!s_connected || data == NULL) return ESP_ERR_INVALID_STATE;

    /* Send frame data in chunks if larger than MTU */
    size_t offset = 0;
    const size_t chunk_size = BLE_MTU_MAX - 3; /* Account for ATT header */

    while (offset < len) {
        size_t chunk = (len - offset < chunk_size) ? (len - offset) : chunk_size;
        esp_ble_gatts_set_attr_value(s_status_char_handle, chunk, (uint8_t *)(data + offset));
        esp_ble_gatts_send_indicate(s_gatts_if, s_conn_id,
                                     s_status_char_handle, chunk,
                                     (uint8_t *)(data + offset), false);
        offset += chunk;
    }

    return ESP_OK;
}

/* =========================================================================
 * FreeRTOS Task
 * ========================================================================= */

void ble_task(void *pvParameters)
{
    ESP_LOGI(TAG, "BLE task started");
    ble_gatt_init();

    /* Periodically send status updates to connected client */
    while (1) {
        if (s_connected) {
            char status[512];
            protocol_build_status(status, sizeof(status));
            ble_gatt_send_status(status);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));  /* Every 2 seconds */
    }
}
