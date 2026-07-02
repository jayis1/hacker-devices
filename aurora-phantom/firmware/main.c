/*
 * main.c — Aurora-Phantom application-core firmware entry point
 *
 * Device:  Aurora-Phantom — Optical Compromising-Emanations Reconstruction
 * Author:  jayis1
 * License: GPL-2.0
 *
 * Target:  Espressif ESP32-S3 (application core)
 *
 * main.c owns the top-level state machine that orchestrates the signal-core
 * FPGA and all drivers. It is responsible for:
 *   - Board bring-up (PMIC, clocks, FPGA config check, SD mount, USB CDC)
 *   - Mission lifecycle: load -> arm -> capture -> rendezvous
 *   - Mode enforcement, especially RF silence during capture
 *   - Dispatching commands from BLE (rendezvous) and USB CDC (config)
 *
 * The firmware is structured so that the heavy signal processing is in the
 * FPGA; the ESP32-S3 never sees raw photon events during normal capture,
 * only compressed IQ / magnitude frames and occasional raw-event snippets
 * for offline analysis. This keeps the application core responsive and lets
 * it sleep the RF rail for covert operation.
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "registers.h"
#include "drivers/spad_driver.h"
#include "drivers/optics_driver.h"
#include "drivers/lockin.h"
#include "drivers/refresh_estimator.h"
#include "drivers/reconstruct.h"
#include "drivers/storage.h"
#include "drivers/ble_link.h"
#include "drivers/usb_cdc.h"
#include "drivers/power.h"

/* ---- Author / version banner ------------------------------------------ */
static const char kBanner[] =
    "\r\n"
    "=========================================\r\n"
    " aurora-phantom  v" AURORA_VERSION "\r\n"
    " optical compromising-emanations recon\r\n"
    " author: " AURORA_AUTHOR "\r\n"
    "=========================================\r\n";

/* ---- Top-level modes --------------------------------------------------- */
typedef enum {
    MODE_STANDBY = 0,   /* rails off, lowest power, await mission or button */
    MODE_ARMED,         /* mission loaded, awaiting start trigger (button) */
    MODE_CAPTURE,       /* RF silent, SPAD + FPGA + lock-in + SD active */
    MODE_RENDEZVOUS,    /* capture done, RF rail on, BLE up, exfil to app */
    MODE_USB_CONFIG,    /* USB CDC console active (config / raw dump) */
    MODE_FAULT
} aurora_mode_t;

static volatile aurora_mode_t g_mode = MODE_STANDBY;
static volatile bool g_btn_pressed  = false;
static volatile bool g_mission_done = false;
static volatile uint32_t g_boot_ms  = 0;

/* ---- Mission descriptor ------------------------------------------------ */
typedef struct {
    uint32_t lo_freq_hz;      /* lock-in LO seed (0 = use estimator) */
    uint32_t int_window_us;   /* integration window per frame */
    uint32_t frame_period_us; /* frame cadence */
    uint32_t duration_s;      /* total capture duration */
    uint16_t wavelength_nm;   /* LC bandpass center */
    uint16_t bias_trim;       /* SPAD bias trim */
    uint8_t  tdc_res;         /* TDC resolution select */
    uint8_t  exfil_policy;    /* 0 = buffer+rendezvous, 1 = live BLE */
    uint8_t  stream_events;   /* 1 = also stream raw events to SD */
    char     name[32];
} mission_t;

static mission_t g_mission;

/* ---- Forward declarations --------------------------------------------- */
static void   on_button(void);
static void   mode_enter(aurora_mode_t m);
static int    mission_load_default(void);
static int    mission_load_from_sd(const char *path);
static void   capture_run(const mission_t *m);
static void   rendezvous_run(void);
static void   usb_console_task(void);
static void   status_led_set(uint8_t r, uint8_t g, uint8_t b);
static void   log_event(const char *tag, const char *msg);
static void   usb_console_handle(const char *line);

/* ---- HAL forward declarations (weak stubs defined at bottom) ---------- */
uint32_t system_ms(void);
void     system_sleep_ms(uint32_t ms);
int      fpga_wait_cdone(uint32_t ms);
int      clocks_init(void);
void     btn_init(void (*cb)(void));

/* ===================================================================== */
int app_main(void)
{
    /* -- 1. Low-level bring-up ------------------------------------------ */
    power_init();                /* nPM1300: enable ESP + FPGA IO rails only */
    usb_cdc_init();
    usb_cdc_write((const uint8_t *)kBanner, sizeof(kBanner) - 1);

    /* -- 2. Clocks & FPGA handshake ------------------------------------ */
    /* Si5351C feeds both the FPGA sample clock and the SPAD gate. */
    if (clocks_init() != 0) {
        log_event("BOOT", "si5351 init failed");
        mode_enter(MODE_FAULT);
    }
    /* Wait for FPGA configuration done (CDONE) before touching reg file. */
    if (fpga_wait_cdone(1000) != 0) {
        log_event("BOOT", "FPGA CDONE timeout");
        mode_enter(MODE_FAULT);
    }
    if (fpga_read(REG_MAGIC) != 0xA51D) {
        log_event("BOOT", "FPGA magic mismatch");
        mode_enter(MODE_FAULT);
    }
    log_event("BOOT", "FPGA signal core alive");

    /* -- 3. Peripherals ------------------------------------------------- */
    spad_init();
    optics_init();
    lockin_init();
    refresh_estimator_init();
    reconstruct_init();

    if (storage_init() != 0) {
        log_event("BOOT", "SD mount failed; capture disabled");
    } else {
        log_event("BOOT", "SD mounted");
    }

    /* Button GPIO + interrupt */
    btn_init(on_button);

    /* BLE is intentionally NOT started here; RF rail is off at boot to
     * preserve silence. BLE only comes up in MODE_RENDEZVOUS. */
    g_mode = MODE_STANDBY;
    status_led_set(0, 0, 1);   /* solid blue = standby */
    log_event("BOOT", "standby");

    /* -- 4. Main loop --------------------------------------------------- */
    uint32_t last_status_ms = 0;
    while (1) {
        const uint32_t now = system_ms();

        /* Button edge handling (mode transitions) */
        if (g_btn_pressed) {
            g_btn_pressed = false;
            switch (g_mode) {
            case MODE_STANDBY:
                /* No mission loaded -> try default from SD. */
                if (mission_load_default() == 0) {
                    mode_enter(MODE_ARMED);
                } else {
                    log_event("BTN", "no mission; enter USB config");
                    mode_enter(MODE_USB_CONFIG);
                }
                break;
            case MODE_ARMED:
                mode_enter(MODE_CAPTURE);
                capture_run(&g_mission);
                g_mission_done = true;
                mode_enter(MODE_RENDEZVOUS);
                break;
            case MODE_CAPTURE:
                /* Forced abort by operator */
                log_event("BTN", "capture aborted by operator");
                g_mission_done = true;
                mode_enter(MODE_RENDEZVOUS);
                break;
            case MODE_RENDEZVOUS:
                mode_enter(MODE_STANDBY);
                break;
            case MODE_USB_CONFIG:
                mode_enter(MODE_STANDBY);
                break;
            default:
                mode_enter(MODE_STANDBY);
                break;
            }
        }

        /* Mode-specific background work */
        if (g_mode == MODE_RENDEZVOUS) {
            rendezvous_run();
        } else if (g_mode == MODE_USB_CONFIG) {
            usb_console_task();
        }

        /* Heartbeat status every 5 s */
        if (now - last_status_ms > 5000) {
            last_status_ms = now;
            char buf[96];
            snprintf(buf, sizeof(buf),
                     "STAT mode=%d sd=%d fpga=%d rate=%u kHz",
                     (int)g_mode, storage_ready(),
                     (fpga_read(REG_STATUS) & STAT_RUNNING) ? 1 : 0,
                     fpga_read(REG_SPAD_RATE_SUM));
            log_event("STAT", buf);
        }

        system_sleep_ms(10);   /* cooperative yield */
    }
}

/* ===================================================================== */
/* Mode transitions                                                       */
static void mode_enter(aurora_mode_t m)
{
    log_event("MODE", "enter");
    g_mode = m;
    switch (m) {
    case MODE_STANDBY:
        power_rail_clear(RAIL_RF | RAIL_SPAD_ANALOG);
        status_led_set(0, 0, 1);
        break;
    case MODE_ARMED:
        power_rail_set(RAIL_SPAD_ANALOG | RAIL_FPGA_CORE | RAIL_FPGA_IO);
        power_rail_clear(RAIL_RF);
        status_led_set(0, 1, 0);   /* solid green = armed */
        break;
    case MODE_CAPTURE:
        /* Critical: RF rail OFF for covert optical-only capture. */
        power_rail_clear(RAIL_RF);
        power_rail_set(RAIL_SPAD_ANALOG | RAIL_FPGA_CORE | RAIL_FPGA_IO);
        status_led_set(1, 0, 0);   /* dim red = capturing (cover) */
        break;
    case MODE_RENDEZVOUS:
        /* Safe to enable RF now; capture is over. */
        power_rail_set(RAIL_RF);
        ble_link_start();
        status_led_set(0, 1, 1);   /* cyan = rendezvous */
        break;
    case MODE_USB_CONFIG:
        power_rail_set(RAIL_RF | RAIL_SPAD_ANALOG | RAIL_FPGA_CORE);
        status_led_set(1, 0, 1);   /* magenta = USB config */
        break;
    case MODE_FAULT:
        power_rail_clear(RAIL_SPAD_ANALOG | RAIL_FPGA_CORE);
        status_led_set(1, 0, 0);
        break;
    }
}

/* ===================================================================== */
/* Mission loading                                                        */
static int mission_load_default(void)
{
    /* Try /aurora/missions/active.json on SD; fall back to a safe builtin. */
    if (storage_ready() && mission_load_from_sd("/aurora/missions/active.json") == 0)
        return 0;

    /* Builtin safe default: 60 Hz-ish hunt, 50 ms integration, 60 s. */
    memset(&g_mission, 0, sizeof(g_mission));
    g_mission.lo_freq_hz    = 0;          /* use refresh estimator */
    g_mission.int_window_us = 50000;      /* 50 ms */
    g_mission.frame_period_us = 100000;   /* 10 fps */
    g_mission.duration_s   = 60;
    g_mission.wavelength_nm = 550;        /* green-centered */
    g_mission.bias_trim    = 2048;        /* midrange */
    g_mission.tdc_res      = 1;           /* 0.5 ns */
    g_mission.exfil_policy = 0;           /* buffer + rendezvous */
    g_mission.stream_events = 1;
    strncpy(g_mission.name, "builtin-default", sizeof(g_mission.name));
    log_event("MISS", "loaded builtin default");
    return 0;
}

/* Minimal JSON-ish parser: enough for the mission schema. Not a full parser;
 * the host app emits a known small schema. */
static int mission_load_from_sd(const char *path)
{
    uint8_t buf[512];
    int n = storage_read_file(path, buf, sizeof(buf) - 1);
    if (n <= 0) return -1;
    buf[n] = 0;

    /* Very small key scanner. */
    memset(&g_mission, 0, sizeof(g_mission));
    const char *p = (const char *)buf;
    const char *q;
#define KEYINT(name, field, dflt) \
    do { q = strstr(p, "\"" name "\""); \
         g_mission.field = q ? (uint32_t)strtoul(q + strlen(name) + 4, NULL, 10) : (dflt); } while (0)

    KEYINT("lo_freq_hz",     lo_freq_hz,      0);
    KEYINT("int_window_us",  int_window_us,   50000);
    KEYINT("frame_period_us",frame_period_us, 100000);
    KEYINT("duration_s",     duration_s,      60);
    KEYINT("wavelength_nm",  wavelength_nm,   550);
    KEYINT("bias_trim",      bias_trim,       2048);
    KEYINT("tdc_res",        tdc_res,         1);
    KEYINT("exfil_policy",   exfil_policy,    0);
    KEYINT("stream_events",  stream_events,   1);
#undef KEYINT

    strncpy(g_mission.name, "sd-mission", sizeof(g_mission.name));
    log_event("MISS", "loaded from SD");
    return 0;
}

/* ===================================================================== */
/* Capture run                                                            */
static void capture_run(const mission_t *m)
{
    char buf[80];
    snprintf(buf, sizeof(buf), "capture start '%s' dur=%us", m->name, m->duration_s);
    log_event("CAP", buf);

    /* 1. Optics: tune LC bandpass to requested wavelength. */
    optics_set_wavelength(m->wavelength_nm);

    /* 2. SPAD: set bias trim + TDC resolution. */
    spad_set_bias(m->bias_trim);
    spad_set_tdc_resolution(m->tdc_res);
    spad_enable_all_pixels(true);

    /* 3. Lock-in: if LO seed provided, set it; else let estimator drive. */
    if (m->lo_freq_hz != 0) {
        lockin_set_lo_freq(m->lo_freq_hz);
    } else {
        /* First, run the refresh estimator for up to 2 s to find a line. */
        refresh_estimator_run(2000);
        uint32_t f = refresh_estimator_get_freq();
        if (f == 0) {
            log_event("CAP", "no refresh line; hunting at 60 Hz");
            lockin_set_lo_freq(60);
        } else {
            lockin_set_lo_freq(f);
            log_event("CAP", "locked refresh line");
        }
    }
    lockin_set_integration_window(m->int_window_us);
    lockin_set_frame_period(m->frame_period_us);
    lockin_enable(true);

    /* 4. Open a raw-event + frame storage session. */
    char session[48];
    snprintf(session, sizeof(session), "%s/%lu",
             SD_FRAME_DIR, (unsigned long)system_ms());
    storage_session_open(session, m->stream_events);

    /* 5. Main capture loop: poll FPGA for frames, log them, check abort. */
    const uint32_t end_ms = system_ms() + (uint32_t)m->duration_s * 1000;
    uint32_t frames_saved = 0;
    while (system_ms() < end_ms && !g_mission_done) {
        uint16_t st = fpga_read(REG_STATUS);
        if (st & STAT_FIFO_OVFL) {
            log_event("CAP", "FIFO overflow; draining");
            fpga_write(REG_FIFO_FLUSH, 1);
        }
        /* Pull any completed frame magnitudes. */
        uint16_t fc = fpga_read(REG_FRAME_COUNT);
        if (fc > 0) {
            reconstruct_pull_frame();
            storage_session_write_frame(reconstruct_frame_buffer(),
                                        reconstruct_frame_words());
            frames_saved++;
            if (frames_saved % 50 == 0) {
                snprintf(buf, sizeof(buf), "frame %u saved", frames_saved);
                log_event("CAP", buf);
            }
        }
        system_sleep_ms(5);
    }

    /* 6. Teardown: stop lock-in, flush storage, disable SPAD. */
    lockin_enable(false);
    spad_enable_all_pixels(false);
    storage_session_close();
    snprintf(buf, sizeof(buf), "capture done frames=%u", frames_saved);
    log_event("CAP", buf);
}

/* ===================================================================== */
/* Rendezvous: push buffered frames / events to the companion app via BLE */
static void rendezvous_run(void)
{
    /* BLE GATT notifications are driven by ble_link; here we feed it. */
    static enum { R_INIT, R_LIST, R_PUSH, R_DONE } rstate = R_INIT;
    static uint16_t idx = 0;
    static uint16_t total = 0;

    switch (rstate) {
    case R_INIT:
        if (!ble_link_is_connected()) return;
        total = storage_list_frames();
        idx = 0;
        rstate = R_LIST;
        break;
    case R_LIST:
        ble_link_send_header(total);
        rstate = R_PUSH;
        break;
    case R_PUSH: {
        uint16_t words[128];
        int n = storage_read_frame(idx, words, 128);
        if (n > 0) {
            ble_link_send_frame(words, (uint32_t)n);
            idx++;
        } else {
            rstate = R_DONE;
        }
        break;
    }
    case R_DONE:
        ble_link_send_trailer();
        log_event("RND", "exfil complete");
        rstate = R_INIT;
        /* Stay in rendezvous until operator button returns to standby. */
        break;
    }
}

/* ===================================================================== */
/* USB CDC console (config / raw dump / mission upload)                   */
static void usb_console_task(void)
{
    uint8_t rx[64];
    int n = usb_cdc_read(rx, sizeof(rx));
    if (n <= 0) return;

    /* Tiny line-based command parser. */
    static char line[128];
    static int llen = 0;
    for (int i = 0; i < n; i++) {
        char c = (char)rx[i];
        if (c == '\r' || c == '\n') {
            line[llen] = 0;
            if (llen > 0) usb_console_handle(line);
            llen = 0;
        } else if (llen < (int)sizeof(line) - 1) {
            line[llen++] = c;
        }
    }
}

/* Command handlers live in main.c; usb_cdc.c just provides line I/O. */
static void usb_console_handle(const char *line)
{
    char resp[128];
    if (strncmp(line, "help", 4) == 0) {
        usb_cdc_puts("aurora-phantom console (author: jayis1)\r\n");
        usb_cdc_puts("  info            - show device info\r\n");
        usb_cdc_puts("  regs            - dump FPGA register file\r\n");
        usb_cdc_puts("  rate            - show aggregate SPAD rate\r\n");
        usb_cdc_puts("  fft             - run 1s FFT + report peak\r\n");
        usb_cdc_puts("  lo <hz>         - set lock-in LO frequency\r\n");
        usb_cdc_puts("  bandpass <nm>   - set LC bandpass\r\n");
        usb_cdc_puts("  mission <json>  - load mission from string\r\n");
        usb_cdc_puts("  capture         - start capture now (builtin)\r\n");
        usb_cdc_puts("  standby         - return to standby\r\n");
    } else if (strncmp(line, "info", 4) == 0) {
        snprintf(resp, sizeof(resp),
                 "aurora-phantom v%s author=%s mode=%d sd=%d\r\n",
                 AURORA_VERSION, AURORA_AUTHOR, (int)g_mode, storage_ready());
        usb_cdc_puts(resp);
    } else if (strncmp(line, "regs", 4) == 0) {
        usb_cdc_dump_regs();
    } else if (strncmp(line, "rate", 4) == 0) {
        snprintf(resp, sizeof(resp), "aggregate rate = %u kHz\r\n",
                 fpga_read(REG_SPAD_RATE_SUM));
        usb_cdc_puts(resp);
    } else if (strncmp(line, "fft", 3) == 0) {
        refresh_estimator_run(1000);
        uint32_t f = refresh_estimator_get_freq();
        snprintf(resp, sizeof(resp), "peak = %u Hz (mag=%u)\r\n",
                 f, fpga_read(REG_FFT_PEAK_MAG));
        usb_cdc_puts(resp);
    } else if (strncmp(line, "lo ", 3) == 0) {
        uint32_t hz = (uint32_t)strtoul(line + 3, NULL, 10);
        lockin_set_lo_freq(hz);
        snprintf(resp, sizeof(resp), "LO = %u Hz\r\n", hz);
        usb_cdc_puts(resp);
    } else if (strncmp(line, "bandpass ", 9) == 0) {
        uint32_t nm = (uint32_t)strtoul(line + 9, NULL, 10);
        optics_set_wavelength((uint16_t)nm);
        snprintf(resp, sizeof(resp), "bandpass = %u nm\r\n", nm);
        usb_cdc_puts(resp);
    } else if (strncmp(line, "capture", 7) == 0) {
        mission_load_default();
        mode_enter(MODE_CAPTURE);
        capture_run(&g_mission);
        g_mission_done = true;
        mode_enter(MODE_RENDEZVOUS);
    } else if (strncmp(line, "standby", 7) == 0) {
        mode_enter(MODE_STANDBY);
    } else {
        usb_cdc_puts("unknown command (try 'help')\r\n");
    }
}

/* ===================================================================== */
static void on_button(void)
{
    g_btn_pressed = true;
}

static void status_led_set(uint8_t r, uint8_t g, uint8_t b)
{
    /* The single status LED is driven via the RMT peripheral; the driver
     * lives in power.c alongside PMIC control. */
    power_status_led(r, g, b);
}

static void log_event(const char *tag, const char *msg)
{
    char line[160];
    int n = snprintf(line, sizeof(line), "[%lu] %s: %s\r\n",
                     (unsigned long)system_ms(), tag, msg);
    usb_cdc_write((const uint8_t *)line, n);
    if (storage_ready())
        storage_append_log(line, (uint32_t)n);
}

/* ---- Weak stubs for platform HAL (host test builds override these) ---- */
__attribute__((weak)) uint32_t system_ms(void) { return g_boot_ms++; }
__attribute__((weak)) void     system_sleep_ms(uint32_t ms) { (void)ms; }
__attribute__((weak)) int      fpga_wait_cdone(uint32_t ms) { (void)ms; return 0; }
__attribute__((weak)) int      clocks_init(void) { return 0; }
__attribute__((weak)) void     btn_init(void (*cb)(void)) { (void)cb; }