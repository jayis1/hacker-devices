/*
 * TRACE-REAPER — main entry point
 * Cooperative scheduler, command dispatch, and the CPA session state machine.
 *
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0
 */

#include "registers.h"
#include "board.h"
#include "drivers/adc_frontend.h"
#include "drivers/fpga_corr.h"
#include "drivers/trace_buf.h"
#include "drivers/ble_bridge.h"
#include "drivers/crypto_model.h"
#include "drivers/storage.h"
#include "drivers/tamper.h"
#include "drivers/ui.h"
#include <string.h>

/* =========================================================================
 * Version
 * ========================================================================= */
#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    0

/* =========================================================================
 * Global state
 * ========================================================================= */
static volatile device_mode_t g_mode = MODE_IDLE;
static volatile uint32_t g_systick_ms = 0;
static volatile uint32_t g_uptime_s = 0;
static volatile uint8_t  g_battery_pct = 100; /* read from charger in real build */

static session_cfg_t   g_cfg;
static session_result_t g_result;
static corr_byte_t      g_corr[KEY_BYTES_AES256];  /* 16 or 32 used */
static uint8_t          g_hyp[KEY_BYTES_AES256][256];
static uint16_t         g_poi[KEY_BYTES_AES256];
static uint16_t         g_key_bytes = KEY_BYTES_AES128;

/* Passkey for session encryption; in real build entered on OLED, stored in
 * backup-domain registers. Here a fixed dev value. */
static const uint8_t g_passkey[16] = {
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
    0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,
};

/* Live trace scratch for BLE notify */
static int16_t g_live_trace[TRACE_MAX_SAMPLES];

/* Snapshot cadence */
#define SNAPSHOT_PERIOD_MS  200
static volatile uint32_t g_last_snapshot_ms = 0;

/* =========================================================================
 * Board API (board.h)
 * ========================================================================= */
void board_init(void);
void board_set_mode(device_mode_t m) { g_mode = m; }
device_mode_t board_get_mode(void) { return g_mode; }
uint32_t board_uptime_s(void) { return g_uptime_s; }
uint32_t board_millis(void) { return g_systick_ms; }

/* =========================================================================
 * SysTick
 * ========================================================================= */
static void systick_init(void)
{
    SysTick_LOAD = (HCLK_HZ / SYSTICK_HZ) - 1;
    SysTick_VAL = 0;
    SysTick_CTRL = SysTick_CTRL_ENABLE | SysTick_CTRL_TICKINT | SysTick_CTRL_CLKSOURCE;
}

void SysTick_Handler(void)
{
    g_systick_ms++;
    if ((g_systick_ms % 1000) == 0) g_uptime_s++;
}

/* =========================================================================
 * RCC / GPIO clock enables (subset for the pins we use)
 * ========================================================================= */
static void clock_init(void)
{
    /* Enable GPIO A,B,C clocks in AHB4 */
    RCC->AHB4ENR |= (1U << 0) | (1U << 1) | (1U << 2);
    /* Enable USART1 in APB2 */
    RCC->APB2ENR |= (1U << 4);
    /* Enable I2C1 in APB1L */
    RCC->APB1LENR |= (1U << 21);
    /* Enable SPI1 in APB2 */
    RCC->APB2ENR |= (1U << 12);
    /* QSPI in AHB3 */
    RCC->AHB3ENR |= (1U << 8);
    /* HSEM in AHB3 */
    RCC->AHB3ENR |= (1U << 10);
}

void board_init(void)
{
    clock_init();
    systick_init();
    /* Subsystem init */
    ui_init();
    ble_bridge_init();
    fpga_corr_init();
    adc_frontend_init();
    storage_init(g_passkey);
}

/* =========================================================================
 * Tamper callback — zeroize everything
 * ========================================================================= */
static void on_tamper(void)
{
    /* Wipe secrets */
    trace_buf_wipe();
    storage_wipe_all();
    tamper_zeroize(g_corr, sizeof(g_corr));
    tamper_zeroize(g_hyp, sizeof(g_hyp));
    tamper_zeroize(&g_result, sizeof(g_result));
    ble_bridge_disconnect();
    fpga_corr_stop();
    ui_clear();
    ui_text(0, "TAMPER");
    ui_text(2, "zeroized");
    board_set_mode(MODE_TAMPERED);
    ble_bridge_notify_tamper();
}

/* =========================================================================
 * Session helpers
 * ========================================================================= */
static uint16_t nbytes_for_cipher(cipher_id_t c)
{
    switch (c) {
    case CIPHER_AES256: return 32;
    case CIPHER_AES128:
    case CIPHER_DES:
    case CIPHER_PRESENT:
    default: return 16;
    }
}

static void prepare_hypotheses(void)
{
    g_key_bytes = nbytes_for_cipher(g_cfg.cipher);
    /* For AES-128 standard CPA, we use HW(S-box output) on the first round. */
    for (uint16_t i = 0; i < g_key_bytes; i++) {
        uint8_t pt = g_cfg.pt_random ? g_cfg.known_pt[i] : g_cfg.known_pt[i];
        crypto_model_hyp_vector(g_cfg.model, (uint8_t)i, pt, g_hyp[i]);
    }
    /* POI: default evenly spaced across the window. A real run scans the
     * variance trace to find leakage points; here we place one per byte. */
    uint16_t stride = g_cfg.window_samples / g_key_bytes;
    if (stride == 0) stride = 1;
    for (uint16_t i = 0; i < g_key_bytes; i++) g_poi[i] = (uint16_t)(i * stride + stride/2);
}

static void reset_result(void)
{
    memset(&g_result, 0, sizeof(g_result));
    crypto_model_reset(g_corr, g_key_bytes);
}

static void finalize_result(void)
{
    g_result.finished_ms = board_millis();
    g_result.traces_used = fpga_corr_trace_count();
    g_result.recovered_bytes = 0;
    for (uint16_t i = 0; i < g_key_bytes; i++) {
        uint8_t bk = 0;
        float r = crypto_model_best(&g_corr[i], &bk);
        g_result.best_key[i] = bk;
        g_result.best_hyp[i] = (int16_t)bk;
        g_result.rho[i] = r;
        if (r >= RHO_THRESHOLD) g_result.recovered_bytes++;
    }
    g_result.confidence_ok = (g_result.recovered_bytes == g_key_bytes) ? 1 : 0;
}

/* =========================================================================
 * Command dispatch
 * ========================================================================= */
static void dispatch_cmd(uint8_t cmd, const uint8_t *p, uint8_t len)
{
    switch (cmd) {
    case CMD_HELLO:
        ble_bridge_notify_status(board_uptime_s(), g_battery_pct, g_mode, 0);
        break;

    case CMD_AUTH_PASSKEY:
        /* The app sends the passkey; we require link-encryption first. */
        if (len >= 6 && ble_bridge_is_secure()) {
            /* Re-authenticate with the provided passkey (echoed from the
             * device OLED confirmation). */
            ble_bridge_authenticate(p);
            ble_bridge_notify_status(board_uptime_s(), g_battery_pct, g_mode, 0);
        } else {
            ble_bridge_notify_fault("auth: link not encrypted");
        }
        break;

    case CMD_GET_STATUS:
        ble_bridge_notify_status(board_uptime_s(), g_battery_pct, g_mode,
                                  fpga_corr_trace_count());
        break;

    case CMD_CONFIGURE:
        if (len >= sizeof(session_cfg_t) && ble_bridge_is_secure()) {
            memcpy(&g_cfg, p, sizeof(session_cfg_t));
            prepare_hypotheses();
            /* Push config to the FPGA */
            fpga_corr_configure(&g_cfg, g_poi, g_hyp);
            /* Configure the analog front-end from cfg */
            adc_frontend_set_input(g_cfg.input);
            adc_frontend_set_gain(g_cfg.gain);
            adc_frontend_set_trigger(g_cfg.trig_src, g_cfg.trig_threshold);
            /* Init trace ring with session id */
            trace_buf_init(g_cfg.session_id, g_passkey);
            reset_result();
            board_set_mode(MODE_CONFIGURED);
            ui_text(2, "configured");
        } else if (!ble_bridge_is_secure()) {
            ble_bridge_notify_fault("cfg: not authenticated");
        }
        break;

    case CMD_ARM:
        if (g_mode == MODE_CONFIGURED && ble_bridge_is_secure()) {
            g_result.started_ms = board_millis();
            fpga_corr_arm();
            board_set_mode(MODE_ACQUIRING);
            ui_led_arm(1);
            ui_text(2, "acquiring");
        }
        break;

    case CMD_STOP:
        fpga_corr_stop();
        if (g_mode == MODE_ACQUIRING) {
            board_set_mode(MODE_PROCESSING);
            fpga_corr_snapshot(g_corr, (uint8_t)g_key_bytes);
            finalize_result();
            storage_save(&g_result, &g_cfg, board_millis());
            ble_bridge_notify_result(&g_result, (uint8_t)g_key_bytes);
            board_set_mode(MODE_DONE);
            ui_text(2, "done");
            ui_led_arm(0);
        }
        break;

    case CMD_GET_RESULT:
        if (g_mode == MODE_DONE && ble_bridge_is_secure()) {
            ble_bridge_notify_result(&g_result, (uint8_t)g_key_bytes);
        }
        break;

    case CMD_DUMP_TRACE:
        if (g_mode == MODE_DONE && ble_bridge_is_secure()) {
            uint32_t idx = 0;
            if (len >= 4) {
                idx = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                      ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
            }
            uint16_t ns = g_cfg.window_samples;
            fpga_corr_fetch_trace(idx, g_live_trace, ns);
            ble_bridge_notify_live_trace(g_live_trace, ns);
        }
        break;

    case CMD_LIST_SESSIONS:
        if (ble_bridge_is_secure()) {
            uint8_t cnt = storage_count();
            ble_bridge_notify_status(board_uptime_s(), g_battery_pct, g_mode, cnt);
        }
        break;

    case CMD_OPEN_SESSION:
        if (ble_bridge_is_secure() && len >= 4) {
            uint32_t idx = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
            session_result_t r; session_cfg_t c; char lbl[32];
            if (storage_load(idx, &r, &c, lbl) == 0) {
                ble_bridge_notify_result(&r, (uint8_t)nbytes_for_cipher(c.cipher));
            }
        }
        break;

    case CMD_SET_INPUT:
        if (len >= 1) adc_frontend_set_input((input_src_t)p[0]);
        break;

    case CMD_SET_GAIN:
        if (len >= 1) adc_frontend_set_gain((gain_t)p[0]);
        break;

    case CMD_SET_TRIG:
        if (len >= 3) {
            int16_t thr = (int16_t)((uint16_t)p[1] | ((uint16_t)p[2] << 8));
            adc_frontend_set_trigger((trigger_src_t)p[0], thr);
        }
        break;

    case CMD_WIPE:
        if (ble_bridge_is_secure()) {
            trace_buf_wipe();
            storage_wipe_all();
            tamper_zeroize(g_corr, sizeof(g_corr));
            ble_bridge_notify_status(board_uptime_s(), g_battery_pct, g_mode, 0);
        }
        break;

    default:
        ble_bridge_notify_fault("unknown cmd");
        break;
    }
}

/* =========================================================================
 * Main loop / scheduler
 * ========================================================================= */
static void handle_buttons(void)
{
    if (ui_btn_mode_take()) {
        /* Cycle status display */
        ui_clear();
        ui_text(0, "TRACE-REAPER");
        switch (g_mode) {
        case MODE_IDLE:      ui_text(2, "idle"); break;
        case MODE_CONFIGURED: ui_text(2, "configured"); break;
        case MODE_ACQUIRING:  ui_text(2, "acquiring"); break;
        case MODE_DONE:      ui_text(2, "done"); break;
        case MODE_TAMPERED:  ui_text(2, "tampered"); break;
        default:             ui_text(2, "fault"); break;
        }
    }
    if (ui_btn_arm_take()) {
        /* Local arm toggle: arm if configured, stop if acquiring. */
        if (g_mode == MODE_CONFIGURED && ble_bridge_is_secure()) {
            g_result.started_ms = board_millis();
            fpga_corr_arm();
            board_set_mode(MODE_ACQUIRING);
            ui_led_arm(1);
            ui_text(2, "acquiring");
        } else if (g_mode == MODE_ACQUIRING) {
            fpga_corr_stop();
            board_set_mode(MODE_PROCESSING);
            fpga_corr_snapshot(g_corr, (uint8_t)g_key_bytes);
            finalize_result();
            storage_save(&g_result, &g_cfg, board_millis());
            ble_bridge_notify_result(&g_result, (uint8_t)g_key_bytes);
            board_set_mode(MODE_DONE);
            ui_text(2, "done");
            ui_led_arm(0);
        }
    }
    if (ui_btn_dump_take()) {
        /* Dump the latest trace to BLE (for live view). */
        if (g_mode == MODE_ACQUIRING || g_mode == MODE_DONE) {
            fpga_corr_fetch_trace(fpga_corr_trace_count() - 1,
                                  g_live_trace, g_cfg.window_samples);
            ble_bridge_notify_live_trace(g_live_trace, g_cfg.window_samples);
        }
    }
}

static void run_acquiring(void)
{
    uint32_t now = board_millis();
    uint32_t traces = fpga_corr_trace_count();

    /* Periodic live trace + correlation snapshot */
    if (now - g_last_snapshot_ms >= SNAPSHOT_PERIOD_MS) {
        g_last_snapshot_ms = now;
        /* Push latest trace sample (for the live view) */
        if (traces > 0) {
            fpga_corr_fetch_trace(traces - 1, g_live_trace, g_cfg.window_samples);
            ble_bridge_notify_live_trace(g_live_trace, g_cfg.window_samples);
            /* Snapshot the running correlations and pick best per byte */
            fpga_corr_snapshot(g_corr, (uint8_t)g_key_bytes);
            uint8_t best[KEY_BYTES_AES256];
            float rho[KEY_BYTES_AES256];
            for (uint16_t i = 0; i < g_key_bytes; i++) {
                uint8_t bk = 0;
                rho[i] = crypto_model_best(&g_corr[i], &bk);
                best[i] = bk;
            }
            ble_bridge_notify_corr_best(best, rho, (uint8_t)g_key_bytes);
        }
        /* Periodic status */
        ble_bridge_notify_status(board_uptime_s(), g_battery_pct, g_mode, traces);
    }

    /* Auto-stop when target traces reached */
    if (g_cfg.target_traces && traces >= g_cfg.target_traces) {
        fpga_corr_stop();
        board_set_mode(MODE_PROCESSING);
        fpga_corr_snapshot(g_corr, (uint8_t)g_key_bytes);
        finalize_result();
        storage_save(&g_result, &g_cfg, board_millis());
        ble_bridge_notify_result(&g_result, (uint8_t)g_key_bytes);
        board_set_mode(MODE_DONE);
        ui_text(2, "done");
        ui_led_arm(0);
    }
}

int main(void)
{
    board_init();
    tamper_init(on_tamper);

    /* Default config: AES-128 HW(S-box out), 5000 traces, 2000 samples,
     * external trigger, shunt input, 0 dB gain, random PT. */
    memset(&g_cfg, 0, sizeof(g_cfg));
    g_cfg.cipher = CIPHER_AES128;
    g_cfg.model = LEAK_HW_SBOX_OUT;
    g_cfg.target_traces = 5000;
    g_cfg.window_samples = 2000;
    g_cfg.trig_src = TRIG_EXTERNAL;
    g_cfg.input = INPUT_SHUNT;
    g_cfg.gain = GAIN_0DB;
    g_cfg.pt_random = 1;
    for (int i = 0; i < SESSION_ID_LEN; i++)
        g_cfg.session_id[i] = (uint8_t)(0x10 + i);
    memcpy(g_cfg.known_pt, "TRACE-REAPER-PT0", 16);
    strncpy(g_cfg.label, "default", sizeof(g_cfg.label));

    board_set_mode(MODE_IDLE);
    ui_led_status(1);

    /* Main cooperative loop */
    for (;;) {
        /* 1. Tamper check (highest priority) */
        if (tamper_check()) {
            /* on_tamper already ran via callback; just spin */
            for (;;) { __asm volatile ("wfi"); }
        }

        /* 2. BLE command polling */
        uint8_t cmd, payload[62], plen;
        if (ble_bridge_poll(&cmd, payload, &plen)) {
            dispatch_cmd(cmd, payload, plen);
        }

        /* 3. Button polling */
        ui_poll(board_millis());
        handle_buttons();

        /* 4. Mode-specific work */
        switch (g_mode) {
        case MODE_ACQUIRING:
            run_acquiring();
            break;
        case MODE_DONE:
            /* Idle in done state; operator can request result/dump */
            break;
        case MODE_TAMPERED:
        case MODE_FAULT:
            for (;;) { __asm volatile ("wfi"); }
        default:
            break;
        }

        /* 5. Low-power wait until next interrupt (SysTick / BLE / button) */
        if (g_mode != MODE_ACQUIRING) {
            __asm volatile ("wfi");
        }
    }
}