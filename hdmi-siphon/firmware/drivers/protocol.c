/**
 * @file    protocol.c
 * @brief   JSON wire protocol for device control and status
 * @author  jayis1
 * @version 1.0.0
 * @license Proprietary — Authorized Security Research Use Only
 *
 * Implements the JSON command/response protocol used between the
 * HDMI-Siphon and its companion app (React Native) or CLI tool (Python).
 * All communication is over WebSocket (WiFi) or BLE GATT notifications.
 *
 * Command format: {"cmd":"<command>","params":{...}}
 * Response format: {"status":"ok|error","<key>":<value>,...}
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_log.h"
#include "cJSON.h"

#include "protocol.h"
#include "board.h"
#include "registers.h"
#include "capture_ctrl.h"
#include "cec_monitor.h"
#include "edid_manager.h"
#include "sd_card.h"

/* =========================================================================
 * Local Constants
 * ========================================================================= */

static const char *TAG = "PROTOCOL";

/* =========================================================================
 * Status JSON Builder
 * ========================================================================= */

size_t protocol_build_status(char *buf, size_t buf_size)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) return 0;

    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddStringToObject(root, "device", DEVICE_NAME);
    cJSON_AddStringToObject(root, "version", FIRMWARE_VERSION_STRING);
    cJSON_AddStringToObject(root, "author", AUTHOR_STRING);

    /* Video status */
    bool hdmi_locked = (reg_read(REG_SYS_STATUS) & SYS_STATUS_HDMI_IN_LOCKED) != 0;
    bool display_connected = (reg_read(REG_SYS_STATUS) & SYS_STATUS_HDMI_OUT_HOTPLUG) != 0;
    bool hdcp_active = (reg_read(REG_SYS_STATUS) & SYS_STATUS_HDCP_ACTIVE) != 0;
    bool fpga_ready = (reg_read(REG_SYS_STATUS) & SYS_STATUS_FPGA_READY) != 0;

    cJSON_AddBoolToObject(root, "hdmi_locked", hdmi_locked);
    cJSON_AddBoolToObject(root, "display_connected", display_connected);
    cJSON_AddBoolToObject(root, "hdcp_active", hdcp_active);
    cJSON_AddBoolToObject(root, "fpga_ready", fpga_ready);

    /* Video timing */
    uint16_t h_active = (uint16_t)reg_read(REG_VIDEO_HACTIVE);
    uint16_t v_active = (uint16_t)reg_read(REG_VIDEO_VACTIVE);
    uint32_t pclk = reg_read(REG_VIDEO_PCLK_KHZ);
    uint16_t refresh = (uint16_t)reg_read(REG_VIDEO_REFRESH_HZ);

    char resolution[32];
    snprintf(resolution, sizeof(resolution), "%ux%u", h_active, v_active);
    cJSON_AddStringToObject(root, "resolution", resolution);
    cJSON_AddNumberToObject(root, "pclk_khz", pclk);
    cJSON_AddNumberToObject(root, "refresh_x100", refresh);

    /* Capture */
    cJSON_AddNumberToObject(root, "frame_count", capture_ctrl_get_frame_count());
    cJSON_AddBoolToObject(root, "capture_busy", capture_ctrl_is_busy());
    cJSON_AddBoolToObject(root, "capture_ready", capture_ctrl_is_ready());

    /* CEC */
    cJSON_AddNumberToObject(root, "cec_msg_count", cec_monitor_get_message_count());
    cJSON_AddBoolToObject(root, "cec_bus_active", cec_monitor_is_bus_active());

    /* EDID */
    size_t edid_len;
    const uint8_t *edid = edid_get_current(&edid_len);
    cJSON_AddBoolToObject(root, "edid_valid", edid_is_valid());
    cJSON_AddNumberToObject(root, "edid_source", (int)edid_get_source());

    /* Storage */
    if (sd_card_is_mounted()) {
        cJSON_AddBoolToObject(root, "sd_mounted", true);
        cJSON_AddNumberToObject(root, "sd_free", sd_card_get_free_space());
        cJSON_AddNumberToObject(root, "sd_total", sd_card_get_total_space());
    } else {
        cJSON_AddBoolToObject(root, "sd_mounted", false);
    }

    /* Battery */
    uint32_t batt_mv = battery_read_mv();
    cJSON_AddNumberToObject(root, "battery_mv", batt_mv);
    cJSON_AddNumberToObject(root, "battery_pct", board_batt_percent(batt_mv));

    /* Mode */
    cJSON_AddStringToObject(root, "mode", "passthrough");  /* Could add mode tracking */

    /* Serialize */
    char *json = cJSON_PrintUnformatted(root);
    size_t len = 0;
    if (json) {
        len = strlen(json);
        if (len >= buf_size) len = buf_size - 1;
        strncpy(buf, json, len);
        buf[len] = '\0';
        free(json);
    }

    cJSON_Delete(root);
    return len;
}

/* =========================================================================
 * Gallery JSON Builder
 * ========================================================================= */

size_t protocol_build_gallery_json(char *buf, size_t buf_size,
                                   const sd_card_file_info_t *files, int count)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) return 0;

    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddNumberToObject(root, "count", count);

    cJSON *arr = cJSON_AddArrayToObject(root, "files");
    for (int i = 0; i < count && i < 32; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", files[i].name);
        cJSON_AddNumberToObject(item, "size", files[i].size);
        cJSON_AddNumberToObject(item, "mod_time", (double)files[i].mod_time);
        cJSON_AddItemToArray(arr, item);
    }

    char *json = cJSON_PrintUnformatted(root);
    size_t len = 0;
    if (json) {
        len = strlen(json);
        if (len >= buf_size) len = buf_size - 1;
        strncpy(buf, json, len);
        buf[len] = '\0';
        free(json);
    }

    cJSON_Delete(root);
    return len;
}

/* =========================================================================
 * Command Parsing & Dispatch
 * ========================================================================= */

/**
 * @brief Parse JSON command and dispatch to appropriate handler
 * @param json_str Incoming JSON command string
 * @param result_out Output buffer for response JSON
 * @param result_size Output buffer size
 * @return ESP_OK if command handled
 */
esp_err_t protocol_parse_and_execute(const char *json_str,
                                      char *result_out, size_t result_size)
{
    if (json_str == NULL || result_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        snprintf(result_out, result_size,
                 "{\"status\":\"error\",\"msg\":\"invalid JSON\"}");
        return ESP_FAIL;
    }

    cJSON *cmd_item = cJSON_GetObjectItem(root, "cmd");
    if (cmd_item == NULL || !cJSON_IsString(cmd_item)) {
        cJSON_Delete(root);
        snprintf(result_out, result_size,
                 "{\"status\":\"error\",\"msg\":\"missing cmd field\"}");
        return ESP_FAIL;
    }

    const char *cmd = cmd_item->valuestring;
    cJSON *params = cJSON_GetObjectItem(root, "params");

    esp_err_t result = ESP_OK;

    if (strcmp(cmd, "status") == 0) {
        protocol_build_status(result_out, result_size);

    } else if (strcmp(cmd, "capture") == 0) {
        int r = capture_ctrl_trigger();
        if (r == 0) {
            snprintf(result_out, result_size,
                     "{\"status\":\"ok\",\"frame_id\":%u}",
                     capture_ctrl_get_frame_count());
        } else {
            snprintf(result_out, result_size,
                     "{\"status\":\"error\",\"code\":%d}", r);
        }

    } else if (strcmp(cmd, "cec_send") == 0) {
        if (params) {
            cJSON *addr = cJSON_GetObjectItem(params, "address");
            cJSON *opcode = cJSON_GetObjectItem(params, "opcode");
            if (cJSON_IsNumber(addr) && cJSON_IsNumber(opcode)) {
                uint8_t dest = (uint8_t)addr->valueint;
                uint8_t op = (uint8_t)opcode->valueint;
                uint8_t payload[16];
                uint8_t plen = 0;

                cJSON *payload_arr = cJSON_GetObjectItem(params, "payload");
                if (cJSON_IsArray(payload_arr)) {
                    int arr_size = cJSON_GetArraySize(payload_arr);
                    for (int i = 0; i < arr_size && i < 16; i++) {
                        cJSON *p = cJSON_GetArrayItem(payload_arr, i);
                        if (cJSON_IsNumber(p)) {
                            payload[plen++] = (uint8_t)p->valueint;
                        }
                    }
                }

                int r = cec_monitor_send(dest, op, payload, plen);
                snprintf(result_out, result_size,
                         "{\"status\":\"%s\"}", r == 0 ? "ok" : "error");
            }
        }

    } else if (strcmp(cmd, "edid_set") == 0) {
        if (params) {
            cJSON *hex = cJSON_GetObjectItem(params, "edid_hex");
            if (cJSON_IsString(hex)) {
                size_t hex_len = strlen(hex->valuestring);
                uint8_t edid_bin[EDID_BLOCK_SIZE];
                size_t bin_len = 0;

                /* Convert hex string to binary (skip any whitespace) */
                for (size_t i = 0; i + 1 < hex_len && bin_len < EDID_BLOCK_SIZE; i += 2) {
                    char byte_str[3] = { hex->valuestring[i], hex->valuestring[i+1], '\0' };
                    edid_bin[bin_len++] = (uint8_t)strtol(byte_str, NULL, 16);
                }

                if (bin_len == EDID_BLOCK_SIZE) {
                    result = edid_inject(edid_bin, bin_len);
                }
            }
        }
        snprintf(result_out, result_size,
                 "{\"status\":\"%s\"}", result == ESP_OK ? "ok" : "error");

    } else if (strcmp(cmd, "osd_text") == 0) {
        if (params) {
            cJSON *text = cJSON_GetObjectItem(params, "text");
            cJSON *x = cJSON_GetObjectItem(params, "x");
            cJSON *y = cJSON_GetObjectItem(params, "y");
            cJSON *color = cJSON_GetObjectItem(params, "color");
            cJSON *alpha = cJSON_GetObjectItem(params, "alpha");

            if (cJSON_IsString(text) && cJSON_IsNumber(x) && cJSON_IsNumber(y)) {
                /* Write OSD text via FPGA registers */
                reg_write(REG_OSD_XPOS, (uint32_t)x->valueint);
                reg_write(REG_OSD_YPOS, (uint32_t)y->valueint);

                if (cJSON_IsString(color)) {
                    /* Parse hex color RRGGBB to RGB565 */
                    uint32_t rgb = (uint32_t)strtol(color->valuestring + 1, NULL, 16);
                    reg_write(REG_OSD_COLOR, rgb);
                }

                if (cJSON_IsNumber(alpha)) {
                    reg_write(REG_OSD_ALPHA, (uint32_t)alpha->valueint);
                }

                /* Write each character to OSD */
                reg_set_bits(REG_OSD_CTRL, OSD_CTRL_ENABLE);
                reg_clear_bits(REG_OSD_CTRL, OSD_CTRL_CLEAR);

                for (const char *c = text->valuestring; *c; c++) {
                    reg_write(REG_OSD_CHAR_DATA, (uint32_t)(*c));
                }

                snprintf(result_out, result_size, "{\"status\":\"ok\"}");
            }
        }

    } else if (strcmp(cmd, "covert_start") == 0) {
        /* Enter covert mode */
        reg_set_bits(REG_SYS_CTRL, SYS_CTRL_STEALTH);
        reg_write(REG_SYS_CTRL, SYS_CTRL_PASSTHROUGH); /* Disable passthrough for capture */

        if (params) {
            cJSON *interval = cJSON_GetObjectItem(params, "interval_ms");
            cJSON *max_frames = cJSON_GetObjectItem(params, "max_frames");
            cJSON *trigger = cJSON_GetObjectItem(params, "trigger");

            if (cJSON_IsNumber(interval)) {
                /* Convert ms to frames (assuming ~60fps) */
                uint32_t frame_interval = (uint32_t)interval->valueint / 16;
                reg_write(REG_CAPTURE_INTERVAL, frame_interval);
            }

            if (cJSON_IsString(trigger) && strcmp(trigger->valuestring, "motion") == 0) {
                reg_set_bits(REG_CAPTURE_CTRL, CAPTURE_CTRL_MOTION_DETECT);
            }

            reg_set_bits(REG_CAPTURE_CTRL, CAPTURE_CTRL_INTERVAL_EN);
        }

        snprintf(result_out, result_size, "{\"status\":\"ok\",\"mode\":\"covert\"}");

    } else if (strcmp(cmd, "covert_stop") == 0) {
        reg_clear_bits(REG_SYS_CTRL, SYS_CTRL_STEALTH);
        reg_clear_bits(REG_CAPTURE_CTRL, CAPTURE_CTRL_INTERVAL_EN | CAPTURE_CTRL_MOTION_DETECT);
        snprintf(result_out, result_size, "{\"status\":\"ok\"}");

    } else if (strcmp(cmd, "mode") == 0) {
        if (params) {
            cJSON *mode_str = cJSON_GetObjectItem(params, "mode");
            if (cJSON_IsString(mode_str)) {
                if (strcmp(mode_str->valuestring, "passthrough") == 0) {
                    reg_write(REG_VIDEO_MODE, VIDEO_MODE_PASSTHROUGH);
                    reg_set_bits(REG_SYS_CTRL, SYS_CTRL_PASSTHROUGH);
                } else if (strcmp(mode_str->valuestring, "invert") == 0) {
                    reg_write(REG_VIDEO_MODE, VIDEO_MODE_INVERT_COLORS);
                } else if (strcmp(mode_str->valuestring, "grayscale") == 0) {
                    reg_write(REG_VIDEO_MODE, VIDEO_MODE_GRAYSCALE);
                } else if (strcmp(mode_str->valuestring, "blank") == 0) {
                    reg_write(REG_VIDEO_MODE, VIDEO_MODE_BLANK);
                }
                snprintf(result_out, result_size, "{\"status\":\"ok\"}");
            }
        }

    } else {
        snprintf(result_out, result_size,
                 "{\"status\":\"error\",\"msg\":\"unknown command: %s\"}", cmd);
        result = ESP_FAIL;
    }

    cJSON_Delete(root);
    return result;
}

/* =========================================================================
 * Dedicated Handler Wrappers (for HTTP handlers)
 * ========================================================================= */

esp_err_t protocol_handle_edid_command(const char *json_str)
{
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) return ESP_FAIL;

    cJSON *hex = cJSON_GetObjectItem(root, "edid_hex");
    if (cJSON_IsString(hex)) {
        uint8_t edid_bin[EDID_BLOCK_SIZE];
        size_t bin_len = 0;
        size_t hex_len = strlen(hex->valuestring);

        for (size_t i = 0; i + 1 < hex_len && bin_len < EDID_BLOCK_SIZE; i += 2) {
            char byte_str[3] = { hex->valuestring[i], hex->valuestring[i+1], '\0' };
            edid_bin[bin_len++] = (uint8_t)strtol(byte_str, NULL, 16);
        }

        if (bin_len == EDID_BLOCK_SIZE) {
            cJSON_Delete(root);
            return edid_inject(edid_bin, bin_len);
        }
    }

    cJSON_Delete(root);
    return ESP_FAIL;
}

esp_err_t protocol_handle_cec_command(const char *json_str)
{
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) return ESP_FAIL;

    cJSON *address = cJSON_GetObjectItem(root, "address");
    cJSON *opcode = cJSON_GetObjectItem(root, "opcode");
    cJSON *payload = cJSON_GetObjectItem(root, "payload");

    if (cJSON_IsNumber(address) && cJSON_IsNumber(opcode)) {
        uint8_t data[16];
        uint8_t dlen = 0;

        if (cJSON_IsString(payload)) {
            /* Parse hex payload string */
            size_t hex_len = strlen(payload->valuestring);
            for (size_t i = 0; i + 1 < hex_len && dlen < 16; i += 2) {
                char byte_str[3] = { payload->valuestring[i], payload->valuestring[i+1], '\0' };
                data[dlen++] = (uint8_t)strtol(byte_str, NULL, 16);
            }
        }

        cec_monitor_send((uint8_t)address->valueint, (uint8_t)opcode->valueint, data, dlen);
    }

    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t protocol_handle_config_command(const char *json_str)
{
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) return ESP_FAIL;

    /* Handle various config options */
    cJSON *item;

    item = cJSON_GetObjectItem(root, "stealth");
    if (cJSON_IsBool(item)) {
        if (cJSON_IsTrue(item)) {
            reg_set_bits(REG_SYS_CTRL, SYS_CTRL_STEALTH);
        } else {
            reg_clear_bits(REG_SYS_CTRL, SYS_CTRL_STEALTH);
        }
    }

    item = cJSON_GetObjectItem(root, "brightness");
    if (cJSON_IsNumber(item)) {
        reg_write(REG_VIDEO_BRIGHTNESS, (uint32_t)(item->valueint & 0xFF));
    }

    item = cJSON_GetObjectItem(root, "contrast");
    if (cJSON_IsNumber(item)) {
        reg_write(REG_VIDEO_CONTRAST, (uint32_t)(item->valueint & 0xFF));
    }

    cJSON_Delete(root);
    return ESP_OK;
}
