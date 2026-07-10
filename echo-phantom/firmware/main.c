/*
 * main.c — ECHO-Phantom Main Application
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The main application implements the ECHO-Phantom state machine:
 *
 *  1. Initialize hardware (clocks, GPIO, SDRAM, SD, BLE, USB, SAI)
 *  2. Auto-detect the I²S/TDM audio format on the target bus
 *  3. Configure SAI1_A (RX from mic) and SAI1_B (TX to host AP)
 *  4. Enter the command loop: process commands from BLE or USB,
 *     manage audio capture/injection/modification, and stream
 *     telemetry and audio back to the operator's app.
 *
 * The firmware runs entirely on the STM32H743 at 480 MHz with
 * no external HAL or RTOS — a bare-metal super-loop with
 * interrupt-driven DMA for the audio path.
 */

#include "board.h"
#include "registers.h"
#include <string.h>

/* Version info (used in ping response) */
#define BOARD_VERSION_MAJOR 1
#define BOARD_VERSION_MINOR 0
#define BOARD_VERSION_PATCH 0

/* Forward declarations */
void NVIC_SystemReset(void);

/* ========================================================================
 *  Global State
 * ======================================================================== */

static device_status_t g_status;
static audio_format_t  g_format;
static echo_mode_t     g_mode = MODE_PASSTHROUGH;
static inject_mode_t   g_inject_mode = INJECT_REPLACE;
static uint8_t         g_inject_gain = 100;    /* percent */
static uint8_t         g_active_clip_id = 0xFF;
static cap_dest_t      g_cap_dest = CAP_DEST_SDRAM;
static uint8_t         g_stream_active = 0;
static uint8_t         g_stream_quality = 16;  /* kbps */

/* DMA double-buffer */
static int32_t  g_rx_buf0[AUDIO_FRAME_SAMPLES] __attribute__((aligned(32)));
static int32_t  g_rx_buf1[AUDIO_FRAME_SAMPLES] __attribute__((aligned(32)));
static int32_t  g_tx_buf0[AUDIO_FRAME_SAMPLES] __attribute__((aligned(32)));
static int32_t  g_tx_buf1[AUDIO_FRAME_SAMPLES] __attribute__((aligned(32)));
static volatile uint8_t g_rx_ready = 0;   /* 1 = buffer 0 ready, 2 = buffer 1 ready */

/* Boot counter stored in backup register */
static uint32_t g_boot_count = 0;

/* ========================================================================
 *  Interrupt Handlers
 * ======================================================================== */

/* DMA1 Stream 0 — SAI1_A RX (audio from microphone) */
void DMA1_Stream0_IRQHandler(void)
{
    uint32_t isr = DMA_LISR;

    if (isr & BIT(11)) {       /* Transfer Complete (TCIF0) */
        /* Buffer 0 (or 1, depending on CT) is full */
        if (DMA_SCR(0) & DMA_SCR_CT)
            g_rx_ready = 2;     /* Buffer 1 filled → process buf1 */
        else
            g_rx_ready = 1;     /* Buffer 0 filled → process buf0 */
    }
    if (isr & BIT(9)) {       /* Half Transfer (HTIF0) */
        /* The other half is ready too */
        if (DMA_SCR(0) & DMA_SCR_CT)
            g_rx_ready = 1;
        else
            g_rx_ready = 2;
    }

    /* Clear flags */
    DMA_LIFCR = 0x3DU;  /* Clear all stream 0 flags */
}

/* DMA1 Stream 1 — SAI1_B TX (audio to host AP) */
void DMA1_Stream1_IRQHandler(void)
{
    /* TX complete — nothing to do, DMA double-buffers automatically */
    DMA_LIFCR = 0x3D0U; /* Clear all stream 1 flags */
}

/* USART1 — BLE C2 link */
void USART1_IRQHandler(void)
{
    if (USART_ISR & USART_ISR_RXNE) {
        /* BLE data received — handled in poll loop */
        USART_ICR = USART_ISR_RXNE;
    }
}

/* TIM2 — used for BCLK frequency measurement */
void TIM2_IRQHandler(void)
{
    TIM2_SR = 0;  /* Clear all timer flags */
}

/* ========================================================================
 *  Local prototypes
 * ======================================================================== */

static void process_command(uint8_t *cmd, uint16_t len);
static void handle_audio_frame(void);
static void send_status_response(void);
static void send_format_response(void);
static void send_battery_response(void);
static void audio_passthrough(const int32_t *src, int32_t *dst, uint16_t samples);
static void audio_inject_process(const int32_t *src, int32_t *dst, uint16_t samples);
static void audio_modify_process(const int32_t *src, int32_t *dst, uint16_t samples);
static void start_capture(void);
static void stop_capture(void);
static void start_injection(void);
static void stop_injection(void);
static void set_mode(echo_mode_t new_mode);
static void stream_audio(const int32_t *buf, uint16_t samples);
static void update_status(void);

/* ========================================================================
 *  main()
 * ======================================================================== */

int main(void)
{
    /* Step 1: Initialize all hardware */
    board_init();

    /* Step 2: Initialize SDRAM (128 MB capture buffer) */
    sdram_init();

    /* Step 3: Initialize SD card (if present) */
    sdcard_init();

    /* Step 4: Initialize BLE C2 link */
    ble_c2_init();

    /* Step 5: Initialize USB CDC */
    usb_cdc_init();

    /* Step 6: Initialize audio DSP pipeline */
    audio_dsp_init();

    /* Step 7: Initialize injection engine */
    inject_init();

    /* Step 8: Auto-detect I²S/TDM format on target bus */
    LED_RED_ON();  /* Red LED = detecting format */

    memset(&g_format, 0, sizeof(g_format));
    g_format.sample_rate = 48000;
    g_format.bit_depth = 16;
    g_format.channels = 2;
    g_format.protocol = PROTO_I2S_PHILIPS;

    /* Try to auto-detect — may fail if bus is not connected yet */
    if (format_detect(&g_format) == 0) {
        /* Detection successful */
        LED_RED_OFF();
    } else {
        /* No bus detected — start in passthrough with default format */
        LED_RED_OFF();
    }

    /* Step 9: Configure SAI for detected format */
    sai_audio_init(&g_format);

    /* Step 10: Start in passthrough mode (transparent) */
    g_mode = MODE_PASSTHROUGH;
    sai_audio_start();

    /* Step 11: Initialize status */
    memset(&g_status, 0, sizeof(g_status));
    g_status.mode = g_mode;
    g_status.format = g_format;
    g_status.battery_mv = battery_read_mv();
    g_status.battery_pct = battery_read_pct();

    /* Main super-loop */
    uint32_t last_status_update = 0;
    uint32_t last_battery_update = 0;

    while (1) {
        /* ---- Process incoming commands (BLE + USB) ---- */
        uint8_t cmd_buf[256];
        int cmd_len;

        /* Check BLE */
        cmd_len = ble_c2_recv(cmd_buf, sizeof(cmd_buf));
        if (cmd_len > 0) {
            process_command(cmd_buf, (uint16_t)cmd_len);
        }

        /* Check USB */
        cmd_len = usb_cdc_recv(cmd_buf, sizeof(cmd_buf));
        if (cmd_len > 0) {
            process_command(cmd_buf, (uint16_t)cmd_len);
        }

        /* ---- Process audio frames ---- */
        if (g_rx_ready) {
            handle_audio_frame();
            g_rx_ready = 0;
        }

        /* ---- Periodic status update ---- */
        uint32_t now = millis();
        if (now - last_status_update > 1000) {  /* every 1s */
            update_status();
            last_status_update = now;
        }

        /* ---- Periodic battery update ---- */
        if (now - last_battery_update > 5000) {  /* every 5s */
            g_status.battery_mv = battery_read_mv();
            g_status.battery_pct = battery_read_pct();
            g_status.charging = battery_charging();
            last_battery_update = now;

            /* Low battery warning — blink red LED */
            if (g_status.battery_pct < 20 && !g_status.charging) {
                LED_RED_TOGGLE();
            }
        }
    }

    return 0;  /* Never reached */
}

/* ========================================================================
 *  Audio Frame Processing
 *
 *  Called when the DMA has filled a buffer with samples from the
 *  microphone (SAI1_A RX). Depending on the current mode, it:
 *    - passthrough: copy RX → TX unchanged
 *    - capture: copy RX → TX and also write to capture buffer
 *    - inject: generate TX from inject clip (ignore RX)
 *    - modify: apply DSP filter to RX, write result to TX
 * ======================================================================== */

static void handle_audio_frame(void)
{
    const int32_t *rx_buf;
    int32_t *tx_buf;

    /* Select the buffer that just completed */
    if (g_rx_ready == 1) {
        rx_buf = g_rx_buf0;
        tx_buf = g_tx_buf0;
    } else {
        rx_buf = g_rx_buf1;
        tx_buf = g_tx_buf1;
    }

    switch (g_mode) {
    case MODE_PASSTHROUGH:
        audio_passthrough(rx_buf, tx_buf, AUDIO_FRAME_SAMPLES);
        break;

    case MODE_CAPTURE:
        audio_passthrough(rx_buf, tx_buf, AUDIO_FRAME_SAMPLES);
        capture_feed(rx_buf, AUDIO_FRAME_SAMPLES);
        break;

    case MODE_INJECT:
        audio_inject_process(rx_buf, tx_buf, AUDIO_FRAME_SAMPLES);
        LED_BLUE_ON();
        break;

    case MODE_MODIFY:
        audio_modify_process(rx_buf, tx_buf, AUDIO_FRAME_SAMPLES);
        break;

    case MODE_OFFLINE:
        /* Don't touch TX — let SAI1_B idle */
        break;
    }

    /* Update SAI1_B TX DMA with the processed buffer */
    if (g_mode != MODE_OFFLINE) {
        if (g_rx_ready == 1) {
            DMA_SM0AR(1) = (uint32_t)g_tx_buf0;
        } else {
            DMA_SM1AR(1) = (uint32_t)g_tx_buf1;
        }
    }

    /* Stream audio over BLE if enabled */
    if (g_stream_active && g_mode != MODE_OFFLINE) {
        stream_audio(rx_buf, AUDIO_FRAME_SAMPLES);
    }

    /* Update capture statistics */
    g_status.capture_frames++;
    g_status.capture_bytes += AUDIO_FRAME_SAMPLES * 4;
}

/* ========================================================================
 *  Audio processing functions
 * ======================================================================== */

static void audio_passthrough(const int32_t *src, int32_t *dst, uint16_t samples)
{
    /* Direct copy — no processing. The 1-frame delay is inaudible. */
    memcpy(dst, src, samples * sizeof(int32_t));
}

static void audio_inject_process(const int32_t *src, int32_t *dst, uint16_t samples)
{
    int32_t inject_buf[AUDIO_FRAME_SAMPLES];

    switch (g_inject_mode) {
    case INJECT_REPLACE:
        /* Fully replace mic audio with the inject clip */
        if (inject_get_next_frame(inject_buf, samples) == 0) {
            memcpy(dst, inject_buf, samples * sizeof(int32_t));
        } else {
            /* Clip exhausted — fall back to silence */
            memset(dst, 0, samples * sizeof(int32_t));
            stop_injection();
        }
        break;

    case INJECT_MIX:
        /* Mix inject clip with mic audio at gain ratio */
        if (inject_get_next_frame(inject_buf, samples) == 0) {
            audio_dsp_mix(dst, inject_buf, samples, g_inject_gain);
            /* Also add the original mic signal at (100 - gain)% */
            uint8_t mic_gain = 100 - g_inject_gain;
            for (uint16_t i = 0; i < samples; i++) {
                dst[i] = dst[i] + (int32_t)((int64_t)src[i] * mic_gain / 100);
            }
        } else {
            memcpy(dst, src, samples * sizeof(int32_t));
            stop_injection();
        }
        break;

    case INJECT_OVERLAY:
        /* Overlay: inject clip on top of mic audio, both audible */
        if (inject_get_next_frame(inject_buf, samples) == 0) {
            for (uint16_t i = 0; i < samples; i++) {
                int64_t mixed = (int64_t)src[i] + (int64_t)inject_buf[i] * g_inject_gain / 100;
                /* Clip to 32-bit range */
                if (mixed > 0x7FFFFFFFLL) mixed = 0x7FFFFFFFLL;
                if (mixed < -0x80000000LL) mixed = -0x80000000LL;
                dst[i] = (int32_t)mixed;
            }
        } else {
            memcpy(dst, src, samples * sizeof(int32_t));
            stop_injection();
        }
        break;
    }
}

static void audio_modify_process(const int32_t *src, int32_t *dst, uint16_t samples)
{
    /* Apply real-time DSP filter */
    memcpy(dst, src, samples * sizeof(int32_t));
    audio_dsp_process(dst, samples);

    /* Also capture the modified audio */
    if (g_stream_active) {
        capture_feed(dst, samples);
    }
}

/* ========================================================================
 *  Mode Management
 * ======================================================================== */

static void set_mode(echo_mode_t new_mode)
{
    if (new_mode == g_mode)
        return;

    /* Stop any active operations */
    if (g_mode == MODE_CAPTURE)
        stop_capture();
    if (g_mode == MODE_INJECT)
        stop_injection();

    g_mode = new_mode;
    g_status.mode = new_mode;

    /* Update LEDs */
    LED_RED_OFF();
    LED_BLUE_OFF();

    switch (new_mode) {
    case MODE_PASSTHROUGH:
        /* Both LEDs off — transparent mode */
        break;
    case MODE_CAPTURE:
        LED_RED_ON();
        start_capture();
        break;
    case MODE_INJECT:
        LED_BLUE_ON();
        start_injection();
        break;
    case MODE_MODIFY:
        LED_RED_ON();
        LED_BLUE_ON();
        break;
    case MODE_OFFLINE:
        /* Both LEDs off — bus disconnected */
        break;
    }
}

static void start_capture(void)
{
    capture_init(g_cap_dest);
    capture_start();
    g_status.capture_frames = 0;
    g_status.capture_bytes = 0;
}

static void stop_capture(void)
{
    capture_stop();
}

static void start_injection(void)
{
    if (g_active_clip_id == 0xFF) {
        /* No clip selected — can't inject */
        LED_BLUE_OFF();
        return;
    }
    inject_start(g_active_clip_id, g_inject_mode, g_inject_gain);
}

static void stop_injection(void)
{
    inject_stop();
    LED_BLUE_OFF();
    if (g_mode == MODE_INJECT) {
        /* Return to passthrough after injection completes */
        g_mode = MODE_PASSTHROUGH;
        g_status.mode = MODE_PASSTHROUGH;
    }
}

/* ========================================================================
 *  Audio Streaming (over BLE to operator's phone)
 * ======================================================================== */

static void stream_audio(const int32_t *buf, uint16_t samples)
{
    /*
     * Simple adaptive quantization for BLE streaming:
     * Downsample to the target quality level.
     *
     * Quality 8 kbps:  8 kHz mono, 8-bit µ-law
     * Quality 16 kbps: 16 kHz mono, 8-bit µ-law
     * Quality 32 kbps: 16 kHz mono, 16-bit PCM
     *
     * This is a minimal implementation. A real deployment would
     * use a lightweight codec like Opus or SBC.
     */
    uint8_t stream_buf[128];
    uint16_t out_idx = 0;

    uint16_t stride;
    uint16_t bytes_per_sample;

    switch (g_stream_quality) {
    case 8:  stride = 6; bytes_per_sample = 1; break;
    case 16: stride = 3; bytes_per_sample = 1; break;
    case 32: stride = 3; bytes_per_sample = 2; break;
    default: stride = 3; bytes_per_sample = 1; break;
    }

    /* Take every Nth sample (downsample) and quantize */
    for (uint16_t i = 0; i < samples && out_idx < sizeof(stream_buf) - 2; i += stride) {
        int32_t sample = buf[i];

        if (bytes_per_sample == 1) {
            /* µ-law encode (simplified) */
            int16_t s16 = (int16_t)(sample >> 16);
            uint8_t ulaw = 0xFF;
            if (s16 < 0) {
                ulaw = 0x7F - ((~s16 >> 2) & 0x7F);
            } else {
                ulaw = 0xFF - ((s16 >> 2) & 0x7F);
            }
            stream_buf[out_idx++] = ulaw;
        } else {
            /* 16-bit PCM */
            int16_t s16 = (int16_t)(sample >> 16);
            stream_buf[out_idx++] = (uint8_t)(s16 & 0xFF);
            stream_buf[out_idx++] = (uint8_t)(s16 >> 8);
        }
    }

    /* Send via BLE */
    if (ble_c2_connected()) {
        ble_c2_send(stream_buf, out_idx);
    }
}

/* ========================================================================
 *  Command Processing
 * ======================================================================== */

static void process_command(uint8_t *cmd, uint16_t len)
{
    if (len < 2)
        return;

    uint8_t cmd_code = cmd[0];
    uint8_t payload_len = cmd[1];

    /* Verify CRC (last byte) */
    if (len >= 3) {
        uint8_t expected_crc = crc8(cmd, len - 1);
        if (expected_crc != cmd[len - 1]) {
            /* CRC mismatch — send NACK */
            uint8_t nack[] = { 0xFF, 0x01, 0x00, 0x00 };
            ble_c2_send(nack, 4);
            return;
        }
    }

    uint8_t *payload = cmd + 2;
    uint8_t response[64];
    uint16_t resp_len = 0;

    switch (cmd_code) {
    case CMD_PING: {
        /* Respond with firmware version */
        response[0] = 0x01;  /* ACK */
        response[1] = 0x06;
        response[2] = BOARD_VERSION_MAJOR & 0xFF;
        response[3] = BOARD_VERSION_MINOR & 0xFF;
        response[4] = BOARD_VERSION_PATCH & 0xFF;
        response[5] = 0;  /* hardware rev */
        response[6] = crc8(response, 6);
        resp_len = 7;
        break;
    }

    case CMD_GET_STATUS:
        send_status_response();
        return;

    case CMD_GET_FORMAT:
        send_format_response();
        return;

    case CMD_GET_BATTERY:
        send_battery_response();
        return;

    case CMD_START_CAPTURE:
        if (payload_len >= 1) {
            g_cap_dest = (cap_dest_t)payload[0];
        }
        set_mode(MODE_CAPTURE);
        response[0] = 0x01;
        response[1] = 0x01;
        response[2] = 0x00;
        response[3] = crc8(response, 3);
        resp_len = 4;
        break;

    case CMD_STOP_CAPTURE:
        stop_capture();
        set_mode(MODE_PASSTHROUGH);
        response[0] = 0x01;
        response[1] = 0x01;
        response[2] = 0x00;
        response[3] = crc8(response, 3);
        resp_len = 4;
        break;

    case CMD_START_INJECT:
        if (payload_len >= 3) {
            g_active_clip_id = payload[0];
            g_inject_mode = (inject_mode_t)payload[1];
            g_inject_gain = payload[2];
        }
        set_mode(MODE_INJECT);
        response[0] = 0x01;
        response[1] = 0x01;
        response[2] = 0x00;
        response[3] = crc8(response, 3);
        resp_len = 4;
        break;

    case CMD_STOP_INJECT:
        stop_injection();
        set_mode(MODE_PASSTHROUGH);
        response[0] = 0x01;
        response[1] = 0x01;
        response[2] = 0x00;
        response[3] = crc8(response, 3);
        resp_len = 4;
        break;

    case CMD_UPLOAD_CLIP: {
        /* Format: clip_id(1) + total_size(4) + offset(4) + data_len(2) + data(N) */
        if (payload_len >= 11) {
            uint8_t clip_id = payload[0];
            uint32_t total_size = payload[1] | (payload[2] << 8) |
                                  (payload[3] << 16) | (payload[4] << 24);
            uint32_t offset = payload[5] | (payload[6] << 8) |
                              (payload[7] << 16) | (payload[8] << 24);
            uint16_t data_len = payload[9] | (payload[10] << 8);
            uint8_t *data = payload + 11;

            if (data_len > 0 && (uint16_t)(data_len + 11) <= payload_len) {
                /* Write clip data to flash */
                uint32_t flash_addr = INJECT_FLASH_BASE + clip_id * 65536 + offset;
                flash_write(flash_addr, data, data_len);
            }

            response[0] = 0x01;
            response[1] = 0x01;
            response[2] = 0x00;
            response[3] = crc8(response, 3);
            resp_len = 4;
        }
        break;
    }

    case CMD_SET_MODE:
        if (payload_len >= 1) {
            echo_mode_t new_mode = (echo_mode_t)payload[0];
            set_mode(new_mode);
            response[0] = 0x01;
            response[1] = 0x01;
            response[2] = 0x00;
            response[3] = crc8(response, 3);
            resp_len = 4;
        }
        break;

    case CMD_SET_FILTER:
        if (payload_len >= 2) {
            uint8_t filter_type = payload[0];
            uint8_t n_coeffs = payload[1];
            if (n_coeffs > 0 && payload_len >= (uint16_t)(2 + n_coeffs * 4)) {
                int32_t coeffs[64];
                for (uint8_t i = 0; i < n_coeffs && i < 64; i++) {
                    coeffs[i] = (int32_t)(payload[2 + i*4] |
                                         (payload[3 + i*4] << 8) |
                                         (payload[4 + i*4] << 16) |
                                         (payload[5 + i*4] << 24));
                }
                audio_dsp_set_filter(filter_type, coeffs, n_coeffs);
            }
            response[0] = 0x01;
            response[1] = 0x01;
            response[2] = 0x00;
            response[3] = crc8(response, 3);
            resp_len = 4;
        }
        break;

    case CMD_STREAM_START:
        if (payload_len >= 1) {
            g_stream_quality = payload[0];
            g_stream_active = 1;
        }
        response[0] = 0x01;
        response[1] = 0x01;
        response[2] = 0x00;
        response[3] = crc8(response, 3);
        resp_len = 4;
        break;

    case CMD_STREAM_STOP:
        g_stream_active = 0;
        response[0] = 0x01;
        response[1] = 0x01;
        response[2] = 0x00;
        response[3] = crc8(response, 3);
        resp_len = 4;
        break;

    case CMD_ERASE_CLIPS:
        /* Erase all inject clip flash sectors */
        for (uint8_t i = 0; i < INJECT_MAX_CLIPS; i++) {
            uint32_t addr = INJECT_FLASH_BASE + i * 65536;
            flash_erase_sector(addr);
        }
        response[0] = 0x01;
        response[1] = 0x01;
        response[2] = 0x00;
        response[3] = crc8(response, 3);
        resp_len = 4;
        break;

    case CMD_FACTORY_RESET:
        /* Check for magic bytes */
        if (payload_len >= 4 && payload[0] == 0xDE && payload[1] == 0xAD &&
            payload[2] == 0xBE && payload[3] == 0xEF) {
            /* Erase all flash and reboot */
            for (uint32_t addr = INJECT_FLASH_BASE; addr < INJECT_FLASH_BASE + INJECT_FLASH_SIZE; addr += 0x20000) {
                flash_erase_sector(addr);
            }
            NVIC_SystemReset();
        }
        break;

    default:
        /* Unknown command */
        response[0] = 0xFF;
        response[1] = 0x01;
        response[2] = 0x01;
        response[3] = crc8(response, 3);
        resp_len = 4;
        break;
    }

    /* Send response */
    if (resp_len > 0) {
        ble_c2_send(response, resp_len);
        if (usb_cdc_connected()) {
            usb_cdc_send(response, resp_len);
        }
    }
}

/* ========================================================================
 *  Status / Format / Battery Responses
 * ======================================================================== */

static void send_status_response(void)
{
    uint8_t resp[48];
    resp[0] = 0x01;  /* ACK */
    resp[1] = 0x26;  /* payload length: 38 bytes */

    /* Uptime (4 bytes) */
    resp[2] = g_status.uptime_s & 0xFF;
    resp[3] = (g_status.uptime_s >> 8) & 0xFF;
    resp[4] = (g_status.uptime_s >> 16) & 0xFF;
    resp[5] = (g_status.uptime_s >> 24) & 0xFF;

    /* Mode (1 byte) */
    resp[6] = (uint8_t)g_status.mode;

    /* Format (8 bytes) */
    resp[7]  = g_status.format.sample_rate & 0xFF;
    resp[8]  = (g_status.format.sample_rate >> 8) & 0xFF;
    resp[9]  = (g_status.format.sample_rate >> 16) & 0xFF;
    resp[10] = (g_status.format.sample_rate >> 24) & 0xFF;
    resp[11] = g_status.format.bit_depth;
    resp[12] = g_status.format.channels;
    resp[13] = g_status.format.protocol;
    resp[14] = 0;
    resp[15] = 0;

    /* Capture frames (4 bytes) */
    resp[16] = g_status.capture_frames & 0xFF;
    resp[17] = (g_status.capture_frames >> 8) & 0xFF;
    resp[18] = (g_status.capture_frames >> 16) & 0xFF;
    resp[19] = (g_status.capture_frames >> 24) & 0xFF;

    /* Capture bytes (4 bytes) */
    resp[20] = g_status.capture_bytes & 0xFF;
    resp[21] = (g_status.capture_bytes >> 8) & 0xFF;
    resp[22] = (g_status.capture_bytes >> 16) & 0xFF;
    resp[23] = (g_status.capture_bytes >> 24) & 0xFF;

    /* Buffer fill (4 bytes) */
    uint32_t fill = 0;
    if (g_status.capture_bytes > 0 && g_cap_dest == CAP_DEST_SDRAM) {
        fill = (g_status.capture_bytes * 100) / CAPTURE_BUF_SIZE_BYTES;
        if (fill > 100) fill = 100;
    }
    resp[24] = fill & 0xFF;
    resp[25] = 0;
    resp[26] = 0;
    resp[27] = 0;

    /* Battery voltage (2 bytes) */
    resp[28] = g_status.battery_mv & 0xFF;
    resp[29] = (g_status.battery_mv >> 8) & 0xFF;

    /* Battery percent (1 byte) */
    resp[30] = g_status.battery_pct;

    /* Flags (1 byte) */
    resp[31] = (g_status.charging ? 0x01 : 0x00) |
               (g_status.sd_present ? 0x02 : 0x00) |
               (g_status.ble_connected ? 0x04 : 0x00) |
               (g_status.usb_connected ? 0x08 : 0x00);

    /* Padding */
    for (int i = 32; i < 39; i++)
        resp[i] = 0;

    resp[39] = crc8(resp, 39);

    ble_c2_send(resp, 40);
    if (usb_cdc_connected())
        usb_cdc_send(resp, 40);
}

static void send_format_response(void)
{
    uint8_t resp[16];
    resp[0] = 0x01;
    resp[1] = 0x08;
    resp[2] = g_format.sample_rate & 0xFF;
    resp[3] = (g_format.sample_rate >> 8) & 0xFF;
    resp[4] = (g_format.sample_rate >> 16) & 0xFF;
    resp[5] = (g_format.sample_rate >> 24) & 0xFF;
    resp[6] = g_format.bit_depth;
    resp[7] = g_format.channels;
    resp[8] = g_format.protocol;
    resp[9] = 0;
    resp[10] = crc8(resp, 10);

    ble_c2_send(resp, 11);
    if (usb_cdc_connected())
        usb_cdc_send(resp, 11);
}

static void send_battery_response(void)
{
    uint8_t resp[8];
    resp[0] = 0x01;
    resp[1] = 0x04;
    resp[2] = g_status.battery_mv & 0xFF;
    resp[3] = (g_status.battery_mv >> 8) & 0xFF;
    resp[4] = g_status.battery_pct;
    resp[5] = g_status.charging ? 0x01 : 0x00;
    resp[6] = crc8(resp, 6);

    ble_c2_send(resp, 7);
    if (usb_cdc_connected())
        usb_cdc_send(resp, 7);
}

/* ========================================================================
 *  Update status periodically
 * ======================================================================== */

static void update_status(void)
{
    g_status.uptime_s = millis() / 1000;
    g_status.capture_frames = capture_get_frames();
    g_status.capture_bytes = capture_get_bytes();
    g_status.sd_present = sdcard_present();
    g_status.ble_connected = ble_c2_connected();
    g_status.usb_connected = usb_cdc_connected();

    if (g_status.ble_connected) {
        g_status.buffer_fill = 0;
        if (g_status.capture_bytes > 0 && g_cap_dest == CAP_DEST_SDRAM) {
            g_status.buffer_fill = (g_status.capture_bytes * 100) / CAPTURE_BUF_SIZE_BYTES;
            if (g_status.buffer_fill > 100)
                g_status.buffer_fill = 100;
        }
    }
}

/* ========================================================================
 *  System reset (called from factory reset)
 * ======================================================================== */

void NVIC_SystemReset(void)
{
    /* Write AIRCR to trigger system reset */
    REG32(0xE000ED0CU) = 0x05FA0004U;
    while (1);  /* Should never reach here */
}

/* ========================================================================
 *  CRC-8 (maxim/Dallas polynomial)
 * ======================================================================== */

uint8_t crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0x00;
    uint8_t poly = 0x31;

    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ poly;
            else
                crc = crc << 1;
        }
        crc &= 0xFF;
    }
    return crc;
}