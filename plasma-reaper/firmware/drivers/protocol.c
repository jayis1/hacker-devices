/*
 * protocol.c — Host command protocol parser
 * PlasmaReaper Multi-Vector Fault Injection Toolkit
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Parses incoming command frames from the host (USB CDC or BLE) and
 * dispatches them to the appropriate handlers. Frames use a simple
 * binary format:
 *
 *   [0xA5] [cmd] [len_lo] [len_hi] [payload...] [crc8] [0x5A]
 *
 * Responses use the same format with the response code in the cmd
 * field.
 */

#include "protocol.h"
#include "usb_cdc.h"
#include "ble_if.h"
#include "../registers.h"
#include "../board.h"

/* ---- Private state ------------------------------------------------- */

#define MAX_PAYLOAD 256

static uint8_t  g_rx_frame[260];
static uint16_t g_rx_idx;
static bool     g_in_frame;
static uint16_t g_payload_len;

/* ---- CRC-8 (poly 0x07, init 0x00) ---------------------------------- */

static uint8_t crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* ---- Frame transmission -------------------------------------------- */

static void send_frame(uint8_t code, const uint8_t *payload, uint16_t len)
{
    uint8_t frame[260];
    uint16_t idx = 0;

    frame[idx++] = FRAME_START_BYTE;
    frame[idx++] = code;
    frame[idx++] = len & 0xFF;
    frame[idx++] = (len >> 8) & 0xFF;
    for (uint16_t i = 0; i < len; i++)
        frame[idx++] = payload[i];
    frame[idx++] = crc8(&frame[1], 3 + len); /* CRC over cmd+len+payload */
    frame[idx++] = FRAME_END_BYTE;

    /* Send over USB if connected */
    if (usb_cdc_connected())
        usb_cdc_tx(frame, idx);
    /* Also send over BLE if connected */
    if (ble_if_connected())
        ble_if_tx(frame, idx);
}

/* ---- Payload deserialization helpers ------------------------------- */

static void deserialize_params(const uint8_t *p, glitch_params_t *params)
{
    params->vector_mask       = p[0];
    params->trigger_offset_ns = (uint32_t)p[1] | ((uint32_t)p[2] << 8)
                              | ((uint32_t)p[3] << 16) | ((uint32_t)p[4] << 24);
    params->glitch_width_ns   = (uint32_t)p[5] | ((uint32_t)p[6] << 8)
                              | ((uint32_t)p[7] << 16) | ((uint32_t)p[8] << 24);
    params->power_depth_mv    = (uint16_t)p[9] | ((uint16_t)p[10] << 8);
    params->power_series_r    = p[11];
    params->clock_shape       = p[12];
    params->clock_cycle_offset = (uint32_t)p[13] | ((uint32_t)p[14] << 8)
                              | ((uint32_t)p[15] << 16) | ((uint32_t)p[16] << 24);
    params->em_pulse_mv       = (uint16_t)p[17] | ((uint16_t)p[18] << 8);
    params->em_pulse_width_ns = (uint16_t)p[19] | ((uint16_t)p[20] << 8);
    params->inter_vector_ns   = (uint32_t)p[21] | ((uint32_t)p[22] << 8)
                              | ((uint32_t)p[23] << 16) | ((uint32_t)p[24] << 24);
    params->repeat_count      = (uint16_t)p[25] | ((uint16_t)p[26] << 8);
    params->repeat_delay_ns   = (uint32_t)p[27] | ((uint32_t)p[28] << 8)
                              | ((uint32_t)p[29] << 16) | ((uint32_t)p[30] << 24);
}

static void serialize_result(const glitch_result_t *result, uint8_t *p)
{
    p[0] = (uint8_t)result->outcome;
    p[1] = result->shots_fired & 0xFF;
    p[2] = (result->shots_fired >> 8) & 0xFF;
    p[3] = (result->shots_fired >> 16) & 0xFF;
    p[4] = (result->shots_fired >> 24) & 0xFF;
    p[5] = result->elapsed_us & 0xFF;
    p[6] = (result->elapsed_us >> 8) & 0xFF;
    p[7] = (result->elapsed_us >> 16) & 0xFF;
    p[8] = (result->elapsed_us >> 24) & 0xFF;
    p[9] = result->waveform_len & 0xFF;
    p[10] = (result->waveform_len >> 8) & 0xFF;
    /* Target response: up to 128 bytes, null-terminated */
    for (int i = 0; i < 128 && result->target_response[i]; i++)
        p[11 + i] = (uint8_t)result->target_response[i];
}

/* ---- Command handlers ---------------------------------------------- */

/* These are implemented in main.c */
void protocol_handle_fire(glitch_params_t *params, glitch_result_t *result);
void protocol_handle_start_sweep(sweep_config_t *config);
void protocol_handle_stop_sweep(void);
void protocol_handle_set_trigger(trigger_config_t *config);
void protocol_handle_set_patterns(pattern_config_t *patterns);
void protocol_handle_get_status(sweep_status_t *status,
                                glitch_result_t *last_result,
                                uint32_t *total_shots,
                                uint32_t *success_shots);
void protocol_handle_get_waveform(uint16_t *waveform, uint16_t *len);

/* ---- Process a complete frame -------------------------------------- */

static void process_frame(uint8_t cmd, const uint8_t *payload, uint16_t len)
{
    switch (cmd) {
    case CMD_PING: {
        uint8_t resp = 0x01;
        send_frame(RESP_OK, &resp, 1);
        break;
    }

    case CMD_GET_VERSION: {
        uint8_t version[4] = { 1, 0, 0, 0 }; /* v1.0.0 */
        send_frame(RESP_VERSION, version, 4);
        break;
    }

    case CMD_FIRE: {
        if (len < 31)
            break;
        glitch_params_t params;
        glitch_result_t result;
        deserialize_params(payload, &params);
        protocol_handle_fire(&params, &result);
        uint8_t resp[139];
        serialize_result(&result, resp);
        send_frame(RESP_RESULT, resp, 139);
        break;
    }

    case CMD_START_SWEEP: {
        if (len < sizeof(sweep_config_t))
            break;
        sweep_config_t config;
        /* Copy raw bytes (assumes same layout on host and device) */
        uint8_t *dst = (uint8_t *)&config;
        for (uint16_t i = 0; i < sizeof(config) && i < len; i++)
            dst[i] = payload[i];
        protocol_handle_start_sweep(&config);
        uint8_t resp = 0x01;
        send_frame(RESP_OK, &resp, 1);
        break;
    }

    case CMD_STOP_SWEEP:
        protocol_handle_stop_sweep();
        uint8_t resp = 0x01;
        send_frame(RESP_OK, &resp, 1);
        break;

    case CMD_GET_STATUS: {
        sweep_status_t status;
        glitch_result_t last_result;
        uint32_t total, success;
        protocol_handle_get_status(&status, &last_result, &total, &success);
        uint8_t resp[32];
        resp[0] = status.running ? 1 : 0;
        resp[1] = status.total_cells & 0xFF;
        resp[2] = (status.total_cells >> 8) & 0xFF;
        resp[3] = (status.total_cells >> 16) & 0xFF;
        resp[4] = (status.total_cells >> 24) & 0xFF;
        resp[5] = status.completed_cells & 0xFF;
        resp[6] = (status.completed_cells >> 8) & 0xFF;
        resp[7] = (status.completed_cells >> 16) & 0xFF;
        resp[8] = (status.completed_cells >> 24) & 0xFF;
        resp[9] = status.success_count & 0xFF;
        resp[10] = (status.success_count >> 8) & 0xFF;
        resp[11] = (status.success_count >> 16) & 0xFF;
        resp[12] = (status.success_count >> 24) & 0xFF;
        resp[13] = total & 0xFF;
        resp[14] = (total >> 8) & 0xFF;
        resp[15] = (total >> 16) & 0xFF;
        resp[16] = (total >> 24) & 0xFF;
        resp[17] = success & 0xFF;
        resp[18] = (success >> 8) & 0xFF;
        resp[19] = (success >> 16) & 0xFF;
        resp[20] = (success >> 24) & 0xFF;
        send_frame(RESP_STATUS, resp, 21);
        break;
    }

    case CMD_GET_WAVEFORM: {
        uint16_t waveform[256];
        uint16_t wlen;
        protocol_handle_get_waveform(waveform, &wlen);
        uint8_t resp[514];
        resp[0] = wlen & 0xFF;
        resp[1] = (wlen >> 8) & 0xFF;
        for (uint16_t i = 0; i < wlen; i++) {
            resp[2 + i * 2] = waveform[i] & 0xFF;
            resp[2 + i * 2 + 1] = (waveform[i] >> 8) & 0xFF;
        }
        send_frame(RESP_WAVEFORM, resp, 2 + wlen * 2);
        break;
    }

    case CMD_SET_TRIGGER: {
        trigger_config_t config;
        uint8_t *dst = (uint8_t *)&config;
        for (uint16_t i = 0; i < sizeof(config) && i < len; i++)
            dst[i] = payload[i];
        protocol_handle_set_trigger(&config);
        uint8_t resp = 0x01;
        send_frame(RESP_OK, &resp, 1);
        break;
    }

    default:
        /* Unknown command */
        break;
    }
}

/* ---- Frame parser (state machine) ---------------------------------- */

static void process_byte(uint8_t byte)
{
    if (!g_in_frame) {
        if (byte == FRAME_START_BYTE) {
            g_in_frame = true;
            g_rx_idx = 0;
            g_rx_frame[g_rx_idx++] = byte;
        }
        return;
    }

    if (g_rx_idx >= sizeof(g_rx_frame)) {
        /* Overflow — reset */
        g_in_frame = false;
        return;
    }

    g_rx_frame[g_rx_idx++] = byte;

    /* After we have start + cmd + len (4 bytes), we know the payload length */
    if (g_rx_idx == 4) {
        g_payload_len = g_rx_frame[2] | (g_rx_frame[3] << 8);
        if (g_payload_len > MAX_PAYLOAD) {
            g_in_frame = false;
            return;
        }
    }

    /* Check for end of frame:
     * total = 1 (start) + 1 (cmd) + 2 (len) + payload + 1 (crc) + 1 (end) */
    if (g_rx_idx == 5 + g_payload_len + 1) {
        /* This should be the CRC byte */
        /* Next byte should be end byte */
    }

    if (g_rx_idx == 5 + g_payload_len + 2) {
        /* Full frame received */
        if (byte == FRAME_END_BYTE) {
            /* Verify CRC */
            uint8_t expected_crc = crc8(&g_rx_frame[1], 3 + g_payload_len);
            uint8_t received_crc = g_rx_frame[5 + g_payload_len - 1 + 1 - 1];
            if (expected_crc == received_crc) {
                /* Valid frame — process it */
                process_frame(g_rx_frame[1],
                              &g_rx_frame[4],
                              g_payload_len);
            }
        }
        g_in_frame = false;
    }
}

/* ---- Poll (called from main loop) ---------------------------------- */

void protocol_poll(void)
{
    uint8_t buf[64];
    uint32_t count;

    /* Read from USB */
    count = usb_cdc_rx(buf, sizeof(buf));
    for (uint32_t i = 0; i < count; i++)
        process_byte(buf[i]);

    /* Read from BLE */
    count = ble_if_rx(buf, sizeof(buf));
    for (uint32_t i = 0; i < count; i++)
        process_byte(buf[i]);
}

/* ---- Sweep progress reporting -------------------------------------- */

void protocol_send_sweep_progress(const sweep_status_t *status,
                                  const glitch_result_t *result)
{
    uint8_t payload[160];
    uint16_t idx = 0;

    /* Sweep status */
    payload[idx++] = status->completed_cells & 0xFF;
    payload[idx++] = (status->completed_cells >> 8) & 0xFF;
    payload[idx++] = (status->completed_cells >> 16) & 0xFF;
    payload[idx++] = (status->completed_cells >> 24) & 0xFF;
    payload[idx++] = status->success_count & 0xFF;
    payload[idx++] = (status->success_count >> 8) & 0xFF;
    payload[idx++] = (status->success_count >> 16) & 0xFF;
    payload[idx++] = (status->success_count >> 24) & 0xFF;

    /* Last result */
    payload[idx++] = (uint8_t)result->outcome;

    /* Target response */
    for (int i = 0; i < 128 && result->target_response[i]; i++)
        payload[idx++] = (uint8_t)result->target_response[i];

    send_frame(RESP_SWEEP_PROGRESS, payload, idx);
}

void protocol_send_sweep_complete(const sweep_status_t *status)
{
    uint8_t payload[20];
    payload[0] = status->total_cells & 0xFF;
    payload[1] = (status->total_cells >> 8) & 0xFF;
    payload[2] = (status->total_cells >> 16) & 0xFF;
    payload[3] = (status->total_cells >> 24) & 0xFF;
    payload[4] = status->success_count & 0xFF;
    payload[5] = (status->success_count >> 8) & 0xFF;
    payload[6] = (status->success_count >> 16) & 0xFF;
    payload[7] = (status->success_count >> 24) & 0xFF;
    payload[8] = status->failure_count & 0xFF;
    payload[9] = (status->failure_count >> 8) & 0xFF;
    payload[10] = (status->failure_count >> 16) & 0xFF;
    payload[11] = (status->failure_count >> 24) & 0xFF;

    send_frame(RESP_SWEEP_COMPLETE, payload, 12);
}

/* ---- End of file --------------------------------------------------- */