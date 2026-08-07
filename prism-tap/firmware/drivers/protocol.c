/*
 * drivers/protocol.c — Binary Command Protocol for Prism-Tap
 *
 * Implements all BLE/USB command handlers.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include "protocol.h"
#include "board.h"
#include "fpga_if.h"
#include "frame_capture.h"
#include "frame_inject.h"
#include "mipi_bridge.h"
#include "sdcard.h"
#include "ble_if.h"
#include <string.h>

/* ---- Individual command handlers ---- */

static int handle_ping(const uint8_t *payload, uint8_t len,
                        uint8_t *response, uint8_t *resp_len)
{
    (void)payload; (void)len;
    response[0] = 0x00;  /* OK */
    response[1] = FW_VERSION_MAJOR;
    response[2] = FW_VERSION_MINOR;
    response[3] = FW_VERSION_PATCH;
    *resp_len = 4;
    return 0;
}

static int handle_get_status(const uint8_t *payload, uint8_t len,
                              uint8_t *response, uint8_t *resp_len)
{
    (void)payload; (void)len;

    /* Pack device status into response */
    response[0] = g_status.mode;
    response[1] = g_status.battery_pct;
    response[2] = (uint8_t)(g_status.battery_mv >> 8);
    response[3] = (uint8_t)(g_status.battery_mv & 0xFF);
    response[4] = g_status.sd_present;
    response[5] = g_status.ble_connected;
    response[6] = g_status.usb_connected;
    response[7] = g_status.fpga_ready;
    response[8] = g_status.csi_link_up;
    response[9] = g_status.dsi_link_up;
    response[10] = (uint8_t)(g_status.uptime_ms >> 24);
    response[11] = (uint8_t)(g_status.uptime_ms >> 16);
    response[12] = (uint8_t)(g_status.uptime_ms >> 8);
    response[13] = (uint8_t)(g_status.uptime_ms & 0xFF);
    response[14] = (uint8_t)(g_status.capture.frames_captured >> 24);
    response[15] = (uint8_t)(g_status.capture.frames_captured >> 16);
    response[16] = (uint8_t)(g_status.capture.frames_captured >> 8);
    response[17] = (uint8_t)(g_status.capture.frames_captured & 0xFF);
    response[18] = g_status.inject.active;
    response[19] = (uint8_t)(g_status.inject.frame_count >> 8);
    response[20] = (uint8_t)(g_status.inject.frame_count & 0xFF);
    *resp_len = 21;
    return 0;
}

static int handle_set_mode(const uint8_t *payload, uint8_t len,
                            uint8_t *response, uint8_t *resp_len)
{
    if (len < 1)
        return -1;
    uint8_t new_mode = payload[0];

    /* Stop current activities */
    if (g_status.capture.active)
        capture_stop();
    if (g_status.inject.active)
        inject_stop();

    switch (new_mode) {
    case MODE_IDLE:
        fpga_enable_tap(0);
        fpga_enable_capture(0);
        fpga_enable_inject(0);
        bridge_power_down(BRIDGE_CSI_RX);
        bridge_power_down(BRIDGE_CSI_TX);
        bridge_power_down(BRIDGE_DSI_RX);
        bridge_power_down(BRIDGE_DSI_TX);
        break;
    case MODE_PASSTHROUGH:
        fpga_enable_tap(1);
        fpga_enable_capture(0);
        fpga_enable_inject(0);
        bridge_power_up(BRIDGE_CSI_RX);
        bridge_power_up(BRIDGE_CSI_TX);
        break;
    case MODE_CAPTURE_ONLY:
        fpga_enable_tap(1);
        fpga_enable_capture(1);
        fpga_enable_inject(0);
        bridge_power_up(BRIDGE_CSI_RX);
        bridge_power_up(BRIDGE_CSI_TX);
        break;
    case MODE_INJECT_ONLY:
        fpga_enable_tap(1);
        fpga_enable_capture(0);
        /* injection enabled when inject_start is called */
        bridge_power_up(BRIDGE_CSI_RX);
        bridge_power_up(BRIDGE_CSI_TX);
        break;
    case MODE_FULL_MITM:
        fpga_enable_tap(1);
        fpga_enable_capture(1);
        bridge_power_up(BRIDGE_CSI_RX);
        bridge_power_up(BRIDGE_CSI_TX);
        break;
    case MODE_CAPTURE_LOW_POWER:
        fpga_enable_tap(1);
        fpga_enable_capture(1);
        fpga_enable_inject(0);
        bridge_power_up(BRIDGE_CSI_RX);
        /* Don't power up Tx bridge (one-directional tap) */
        bridge_power_down(BRIDGE_CSI_TX);
        break;
    case MODE_DIAGNOSTIC:
        fpga_reset();
        bridge_power_up(BRIDGE_CSI_RX);
        bridge_power_up(BRIDGE_CSI_TX);
        break;
    default:
        return -2;
    }

    g_status.mode = new_mode;
    response[0] = 0x00;
    *resp_len = 1;
    return 0;
}

static int handle_start_capture(const uint8_t *payload, uint8_t len,
                                 uint8_t *response, uint8_t *resp_len)
{
    if (len < 10)
        return -1;

    capture_config_t cfg;
    cfg.active = 1;
    cfg.format = payload[0];
    cfg.width = (uint16_t)(payload[1] | (payload[2] << 8));
    cfg.height = (uint16_t)(payload[3] | (payload[4] << 8));
    cfg.interval_ms = (uint32_t)(payload[5] | (payload[6] << 8) |
                                  (payload[7] << 16) | (payload[8] << 24));
    cfg.max_frames = (uint32_t)(payload[9] | (len > 13 ? (payload[10] << 8) : 0) |
                                (len > 14 ? (payload[11] << 16) : 0) |
                                (len > 15 ? (payload[12] << 24) : 0));
    cfg.jpeg_compress = (len > 14) ? payload[13] : 0;
    cfg.frames_captured = 0;
    cfg.bytes_written = 0;
    cfg.last_frame_ts = 0;

    int ret = capture_start(&cfg);
    if (ret) {
        response[0] = (uint8_t)(-ret);
        *resp_len = 1;
        return ret;
    }

    response[0] = 0x00;
    *resp_len = 1;
    return 0;
}

static int handle_stop_capture(const uint8_t *payload, uint8_t len,
                                uint8_t *response, uint8_t *resp_len)
{
    (void)payload; (void)len;
    capture_stop();
    response[0] = 0x00;
    *resp_len = 1;
    return 0;
}

static int handle_load_inject(const uint8_t *payload, uint8_t len,
                               uint8_t *response, uint8_t *resp_len)
{
    if (len < 5)
        return -1;

    uint16_t width = (uint16_t)(payload[0] | (payload[1] << 8));
    uint16_t height = (uint16_t)(payload[2] | (payload[3] << 8));
    uint8_t format = payload[4];
    const uint8_t *frame_data = &payload[5];
    uint32_t frame_len = len - 5;

    int ret = inject_load_frame(frame_data, frame_len, width, height, format);
    if (ret) {
        response[0] = (uint8_t)(-ret);
        *resp_len = 1;
        return ret;
    }

    response[0] = 0x00;
    *resp_len = 1;
    return 0;
}

static int handle_start_inject(const uint8_t *payload, uint8_t len,
                                 uint8_t *response, uint8_t *resp_len)
{
    if (len < 1)
        return -1;

    inject_config_t cfg;
    cfg.mode = payload[0];
    cfg.overlay_x = (len > 2) ? (uint16_t)(payload[1] | (payload[2] << 8)) : 0;
    cfg.overlay_y = (len > 4) ? (uint16_t)(payload[3] | (payload[4] << 8)) : 0;
    cfg.overlay_w = (len > 6) ? (uint16_t)(payload[5] | (payload[6] << 8)) : 0;
    cfg.overlay_h = (len > 8) ? (uint16_t)(payload[7] | (payload[8] << 8)) : 0;
    cfg.trigger_interval = (len > 12) ? (uint32_t)(payload[9] | (payload[10] << 8) |
                                                    (payload[11] << 16) | (payload[12] << 24)) : 1;
    cfg.active = 1;
    cfg.frame_count = 0;

    int ret = inject_start(&cfg);
    if (ret) {
        response[0] = (uint8_t)(-ret);
        *resp_len = 1;
        return ret;
    }

    response[0] = 0x00;
    *resp_len = 1;
    return 0;
}

static int handle_stop_inject(const uint8_t *payload, uint8_t len,
                               uint8_t *response, uint8_t *resp_len)
{
    (void)payload; (void)len;
    inject_stop();
    response[0] = 0x00;
    *resp_len = 1;
    return 0;
}

static int handle_set_timing(const uint8_t *payload, uint8_t len,
                              uint8_t *response, uint8_t *resp_len)
{
    if (len < 5)
        return -1;

    uint8_t enable = payload[0];
    int32_t delay_ns = (int32_t)(payload[1] | (payload[2] << 8) |
                                  (payload[3] << 16) | (payload[4] << 24));
    uint8_t jitter = (len > 5) ? payload[5] : 0;
    uint8_t drop_pattern = (len > 6) ? payload[6] : 0;

    fpga_set_timing_delay(delay_ns);
    fpga_set_timing_jitter(jitter);
    fpga_set_timing_drop_pattern(drop_pattern);
    fpga_enable_timing(enable);

    g_status.timing.enabled = enable;
    g_status.timing.delay_ns = delay_ns;
    g_status.timing.jitter_pct = jitter;
    g_status.timing.drop_pattern = drop_pattern;

    response[0] = 0x00;
    *resp_len = 1;
    return 0;
}

static int handle_drop_frames(const uint8_t *payload, uint8_t len,
                               uint8_t *response, uint8_t *resp_len)
{
    if (len < 2)
        return -1;

    uint16_t n_frames = (uint16_t)(payload[0] | (payload[1] << 8));
    /* Set drop pattern to drop n_frames then pass */
    /* For simplicity, set a pattern that drops the next n frames */
    uint8_t pattern = 0xFF;  /* drop all for now */
    (void)n_frames;
    fpga_set_timing_drop_pattern(pattern);
    fpga_enable_timing(1);

    response[0] = 0x00;
    *resp_len = 1;
    return 0;
}

static int handle_get_config(const uint8_t *payload, uint8_t len,
                              uint8_t *response, uint8_t *resp_len)
{
    (void)payload; (void)len;

    config_packet_t cfg;
    cfg.csi_lanes = 2;
    cfg.csi_speed = 1000;
    cfg.csi_width = 1920;
    cfg.csi_height = 1080;
    cfg.csi_format = FMT_RAW10;
    cfg.dsi_lanes = 2;
    cfg.dsi_speed = 800;
    cfg.dsi_width = 1080;
    cfg.dsi_height = 1920;
    cfg.dsi_format = FMT_RGB888;
    cfg.jpeg_compress = g_status.capture.jpeg_compress;
    memset(cfg.reserved, 0, sizeof(cfg.reserved));

    memcpy(response, &cfg, sizeof(config_packet_t));
    *resp_len = sizeof(config_packet_t);
    return 0;
}

static int handle_set_config(const uint8_t *payload, uint8_t len,
                              uint8_t *response, uint8_t *resp_len)
{
    if (len < (uint8_t)sizeof(config_packet_t))
        return -1;

    config_packet_t cfg;
    memcpy(&cfg, payload, sizeof(config_packet_t));

    /* Reconfigure CSI bridges */
    csi_config_t csi_cfg;
    csi_cfg.num_lanes = cfg.csi_lanes;
    csi_cfg.lane_speed_mbps = cfg.csi_speed;
    csi_cfg.width = cfg.csi_width;
    csi_cfg.height = cfg.csi_height;
    csi_cfg.pixel_format = cfg.csi_format;
    csi_cfg.pixel_clock_hz = (uint32_t)cfg.csi_width * cfg.csi_height * 60;

    bridge_init_csi_rx(&csi_cfg);
    bridge_init_csi_tx(&csi_cfg);

    /* Reconfigure DSI bridges */
    dsi_config_t dsi_cfg;
    dsi_cfg.num_lanes = cfg.dsi_lanes;
    dsi_cfg.lane_speed_mbps = cfg.dsi_speed;
    dsi_cfg.width = cfg.dsi_width;
    dsi_cfg.height = cfg.dsi_height;
    dsi_cfg.pixel_format = cfg.dsi_format;
    dsi_cfg.video_mode = 1;
    dsi_cfg.dcs_enabled = 1;

    bridge_init_dsi_rx(&dsi_cfg);
    bridge_init_dsi_tx(&dsi_cfg);

    g_status.capture.jpeg_compress = cfg.jpeg_compress;

    response[0] = 0x00;
    *resp_len = 1;
    return 0;
}

static int handle_erase_frames(const uint8_t *payload, uint8_t len,
                                uint8_t *response, uint8_t *resp_len)
{
    (void)payload; (void)len;
    /* Delete all .ptf files in /prismtap/ directory */
    sdcard_delete("/prismtap/cap_0.ptf");
    sdcard_delete("/prismtap/cap_1.ptf");
    sdcard_delete("/prismtap/cap_2.ptf");
    sdcard_delete("/prismtap/cap_3.ptf");
    sdcard_delete("/prismtap/cap_4.ptf");
    sdcard_delete("/prismtap/cap_5.ptf");
    sdcard_delete("/prismtap/cap_6.ptf");
    sdcard_delete("/prismtap/cap_7.ptf");
    sdcard_delete("/prismtap/cap_8.ptf");
    sdcard_delete("/prismtap/cap_9.ptf");

    response[0] = 0x00;
    *resp_len = 1;
    return 0;
}

static int handle_export_frames(const uint8_t *payload, uint8_t len,
                                 uint8_t *response, uint8_t *resp_len)
{
    (void)payload; (void)len;
    /* Start USB bulk export mode */
    extern int usb_start_bulk_exfil(void);
    usb_start_bulk_exfil();

    response[0] = 0x00;
    *resp_len = 1;
    return 0;
}

/* ---- Dispatch ---- */

int protocol_dispatch(uint8_t opcode, const uint8_t *payload, uint8_t len,
                      uint8_t *response, uint8_t *resp_len)
{
    if (!response || !resp_len)
        return -1;

    switch (opcode) {
    case CMD_PING:          return handle_ping(payload, len, response, resp_len);
    case CMD_GET_STATUS:    return handle_get_status(payload, len, response, resp_len);
    case CMD_SET_MODE:      return handle_set_mode(payload, len, response, resp_len);
    case CMD_START_CAPTURE: return handle_start_capture(payload, len, response, resp_len);
    case CMD_STOP_CAPTURE:  return handle_stop_capture(payload, len, response, resp_len);
    case CMD_LOAD_INJECT:   return handle_load_inject(payload, len, response, resp_len);
    case CMD_START_INJECT:  return handle_start_inject(payload, len, response, resp_len);
    case CMD_STOP_INJECT:   return handle_stop_inject(payload, len, response, resp_len);
    case CMD_SET_TIMING:    return handle_set_timing(payload, len, response, resp_len);
    case CMD_DROP_FRAMES:   return handle_drop_frames(payload, len, response, resp_len);
    case CMD_GET_CONFIG:    return handle_get_config(payload, len, response, resp_len);
    case CMD_SET_CONFIG:    return handle_set_config(payload, len, response, resp_len);
    case CMD_ERASE_FRAMES:  return handle_erase_frames(payload, len, response, resp_len);
    case CMD_EXPORT_FRAMES: return handle_export_frames(payload, len, response, resp_len);
    default:
        response[0] = 0xFF;
        *resp_len = 1;
        return -2;
    }
}

/* BLE handler wrapper */
static int ble_cmd_wrapper(const uint8_t *payload, uint8_t len,
                            uint8_t *response, uint8_t *resp_len)
{
    /* The opcode is determined by which handler was registered.
       We use a global to pass it from the registration. */
    /* This is a simplified approach; in practice each opcode gets its own wrapper. */
    (void)payload; (void)len; (void)response; (void)resp_len;
    return 0;
}

int protocol_init(void)
{
    /* Register all command handlers with BLE module */
    /* In a real implementation, we'd register individual handlers.
       For simplicity, ble_poll calls protocol_dispatch with the opcode. */
    ble_register_handler(CMD_PING, ble_cmd_wrapper);
    ble_register_handler(CMD_GET_STATUS, ble_cmd_wrapper);
    ble_register_handler(CMD_SET_MODE, ble_cmd_wrapper);
    ble_register_handler(CMD_START_CAPTURE, ble_cmd_wrapper);
    ble_register_handler(CMD_STOP_CAPTURE, ble_cmd_wrapper);
    ble_register_handler(CMD_LOAD_INJECT, ble_cmd_wrapper);
    ble_register_handler(CMD_START_INJECT, ble_cmd_wrapper);
    ble_register_handler(CMD_STOP_INJECT, ble_cmd_wrapper);
    ble_register_handler(CMD_SET_TIMING, ble_cmd_wrapper);
    ble_register_handler(CMD_GET_CONFIG, ble_cmd_wrapper);
    ble_register_handler(CMD_SET_CONFIG, ble_cmd_wrapper);
    ble_register_handler(CMD_ERASE_FRAMES, ble_cmd_wrapper);
    ble_register_handler(CMD_EXPORT_FRAMES, ble_cmd_wrapper);

    return 0;
}

/* ---- End of protocol.c ----
 * Author: jayis1
 */