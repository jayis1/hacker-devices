/*
 * main.c — CoilGrip firmware entry point and command dispatcher
 *
 * Author:  jayis1
 * Copyright (C) 2026 jayis1
 * SPDX-License-Identifier: GPL-2.0
 *
 * CoilGrip is a Wireless Power Transfer (Qi) Manipulation, MITM &
 * Covert Channel Platform. See README.md for the full design.
 *
 * The firmware is a cooperative state machine (no RTOS) driven by a
 * priority-ordered main loop. The high-resolution timer (HRTIM) and
 * ADC ISRs handle the time-critical Qi PWM and sampling; everything
 * else is polled. Commands arrive over BLE (encrypted) or USB-CDC.
 */

#include "board.h"
#include "registers.h"

/* Globals declared in board.h */
volatile coil_mode_t g_mode = MODE_IDLE;
volatile int8_t      g_coil_temp_c = 0;
volatile float       g_current_a = 0.0f;
volatile float       g_power_w = 0.0f;

/* ---- Forward decls for debug helpers (defined in board_init.c) --------- */
void debug_puts(const char *s);
void debug_put_u(uint32_t v);
void debug_put_x(uint32_t v);

/* ---- Command protocol -------------------------------------------------- */
/* Commands are short ASCII tokens for the USB CDC console and a binary
 * form for BLE. The console parser is line-oriented (\n terminated). */

#define CMD_LINE_MAX 80
static char g_cmd_line[CMD_LINE_MAX];
static uint8_t g_cmd_len = 0;

static const char *mode_name(coil_mode_t m)
{
    switch (m) {
    case MODE_IDLE:         return "IDLE";
    case MODE_SNIFFER:      return "SNIFFER";
    case MODE_MITM:         return "MITM";
    case MODE_PROFILE:      return "PROFILE";
    case MODE_GLITCH:       return "GLITCH";
    case MODE_EXFIL:        return "EXFIL";
    case MODE_FOD_RESEARCH: return "FOD";
    default:                 return "?";
    }
}

/* ---- Mode handlers (long-running, called from main loop) --------------- */

static void mode_sniffer_tick(void)
{
    /* Passive: read RX telemetry and log it. No TX. */
    uint16_t vrect = 0, iout = 0;
    int8_t temp = 0; uint8_t st = 0;
    if (qi_rx_read_telemetry(&vrect, &iout, &temp, &st) == 0) {
        g_current_a = (float)iout / 1000.0f;
        g_coil_temp_c = temp;
        /* Log to SD every ~100 ms (heuristic: call count) */
        uint8_t log[8];
        log[0] = (uint8_t)(vrect >> 8); log[1] = (uint8_t)vrect;
        log[2] = (uint8_t)(iout >> 8); log[3] = (uint8_t)iout;
        log[4] = (uint8_t)temp; log[5] = st;
        sdcard_write_log(log, 6);
    }
}

static void mode_mitm_tick(void)
{
    /* Inline MITM: TX is running (started on mode enter), we rewrite
     * negotiation packets from the target via load_modulator. */
    static uint8_t throttle = 0;
    if (++throttle < 10) return;
    throttle = 0;
    /* Read target's load-modulated packets and check for Control Error
     * (type 1) — if it asks for more power than our cap, clamp it. */
    uint8_t pkt[16]; uint16_t len = 0;
    if (load_mod_recv(pkt, sizeof(pkt), &len) == 0 && len >= 2) {
        /* pkt[0] = header, pkt[1..] = data. If header type == CTRL_ERROR
         * and the signed value > our limit, rewrite it. */
        uint8_t hdr = pkt[0];
        uint8_t type = (hdr >> 4) & 0x7U;
        if (type == 1 && len >= 3) {
            int8_t val = (int8_t)pkt[2];
            if (val > 50) {
                pkt[2] = (uint8_t)50;  /* clamp demand */
                /* Re-send modified packet toward upstream charger */
                qi_rx_send_packet(pkt, (uint8_t)len);
            }
        }
    }
    fod_thermal_interlock_check();
}

static void mode_profile_tick(void)
{
    uint8_t state = 0; float conf = 0.0f;
    if (profiler_classify_state(&state, &conf) == 0) {
        debug_puts("profile: state=");
        debug_put_u(state);
        debug_puts(" conf=");
        debug_put_u((uint32_t)(conf * 1000.0f));
        debug_puts("\n");
    }
}

static void mode_glitch_tick(void)
{
    /* Glitch is fire-on-command; the main loop just monitors. */
    fod_thermal_interlock_check();
    if (fod_thermal_shutdown_active()) {
        debug_puts("glitch: thermal shutdown!\n");
        g_mode = MODE_IDLE;
    }
}

static void mode_exfil_tick(void)
{
    /* Pull any covert-channel bytes received from the target and log them. */
    uint8_t buf[16]; uint16_t len = 0;
    if (load_mod_recv(buf, sizeof(buf), &len) == 0) {
        sdcard_write_log(buf, len);
        debug_puts("exfil: got ");
        debug_put_u(len);
        debug_puts(" bytes\n");
    }
}

static void mode_fod_tick(void)
{
    if (!fod_safety_unlocked()) {
        debug_puts("fod: safety-locked, refusing\n");
        g_mode = MODE_IDLE;
        return;
    }
    int32_t off = 0; int8_t t = 0;
    fod_get_status(&off, &t);
    debug_puts("fod: offset_mW=");
    debug_put_u((uint32_t)off);
    debug_puts(" temp=");
    debug_put_u((uint32_t)t);
    debug_puts("\n");
    fod_thermal_interlock_check();
}

/* ---- Mode enter/leave -------------------------------------------------- */

static void enter_mode(coil_mode_t m)
{
    /* Leave current mode */
    switch (g_mode) {
    case MODE_SNIFFER:
    case MODE_MITM:
        qi_tx_stop();
        break;
    case MODE_PROFILE:
        profiler_stop();
        break;
    case MODE_GLITCH:
        glitch_engine_disarm();
        qi_tx_stop();
        break;
    case MODE_EXFIL:
        qi_rx_set_load_modulation(false);
        break;
    default: break;
    }

    g_mode = m;
    debug_puts("mode -> ");
    debug_puts(mode_name(m));
    debug_puts("\n");

    switch (m) {
    case MODE_IDLE:
        break;
    case MODE_SNIFFER:
        qi_rx_init();
        sdcard_open_log("sniffer.log");
        break;
    case MODE_MITM:
        qi_rx_init();
        qi_tx_start(140000, 50);
        sdcard_open_log("mitm.log");
        break;
    case MODE_PROFILE:
        profiler_start(PROFILER_FS_HZ);
        sdcard_open_log("profile.log");
        break;
    case MODE_GLITCH:
        qi_tx_start(140000, 50);
        break;
    case MODE_EXFIL:
        qi_rx_init();
        qi_rx_set_load_modulation(true);
        load_mod_init();
        sdcard_open_log("exfil.log");
        break;
    case MODE_FOD_RESEARCH:
        fod_calibrate();
        break;
    }
}

/* ---- Console command parser ------------------------------------------- */

static int parse_uint(const char *s, uint32_t *out)
{
    uint32_t v = 0;
    if (*s == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        while (*s) {
            char c = *s++;
            uint8_t d;
            if (c >= '0' && c <= '9') d = (uint8_t)(c - '0');
            else if (c >= 'a' && c <= 'f') d = (uint8_t)(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') d = (uint8_t)(c - 'A' + 10);
            else return -1;
            v = (v << 4) | d;
        }
    } else {
        while (*s >= '0' && *s <= '9') {
            v = v * 10 + (uint32_t)(*s - '0');
            s++;
        }
    }
    *out = v;
    return 0;
}

/* Tiny strcmp replacement so we don't pull in libc */
static bool strcmp_eq(const char *a, const char *b)
{
    while (*a && *b) { if (*a != *b) return false; a++; b++; }
    return *a == *b;
}

static void cmd_help(void)
{
    debug_puts("CoilGrip v1.0 (jayis1)\n");
    debug_puts("Commands:\n");
    debug_puts("  help                 - this help\n");
    debug_puts("  mode <m>             - set mode: idle|sniffer|mitm|profile|glitch|exfil|fod\n");
    debug_puts("  tx start <freq_hz> <duty>  - start Qi TX\n");
    debug_puts("  tx stop              - stop Qi TX\n");
    debug_puts("  tx power <pct>       - set TX power percentage\n");
    debug_puts("  rx telemetry         - print RX telemetry\n");
    debug_puts("  glitch arm <delay_us> <width_us> <repeat> <ramp_us>\n");
    debug_puts("  glitch fire           - fire armed glitch\n");
    debug_puts("  glitch disarm        - disarm glitch engine\n");
    debug_puts("  profiler class       - run one classification\n");
    debug_puts("  fod unlock <code>    - unlock FOD safety research\n");
    debug_puts("  fod status           - print FOD status\n");
    debug_puts("  status               - print device status\n");
    debug_puts("  log close             - close SD log file\n");
}

static void cmd_status(void)
{
    int32_t off = 0; int8_t t = 0;
    fod_get_status(&off, &t);
    g_coil_temp_c = t;
    debug_puts("=== CoilGrip status ===\n");
    debug_puts("Author: jayis1  Version: 1.0\n");
    debug_puts("Mode: "); debug_puts(mode_name(g_mode)); debug_puts("\n");
    debug_puts("Coil temp: "); debug_put_u((uint32_t)t); debug_puts(" C\n");
    debug_puts("FOD offset: "); debug_put_u((uint32_t)off); debug_puts(" mW\n");
    debug_puts("FOD safety: "); debug_puts(fod_safety_unlocked() ? "UNLOCKED\n" : "locked\n");
    debug_puts("TX running: "); debug_puts(qi_tx_is_running() ? "yes\n" : "no\n");
    debug_puts("Glitch armed: "); debug_puts(glitch_engine_is_armed() ? "yes\n" : "no\n");
    debug_puts("Glitch fires: "); debug_put_u(glitch_engine_get_count()); debug_puts("\n");
    debug_puts("SD present: "); debug_puts(sdcard_present() ? "yes\n" : "no\n");
    debug_puts("BLE connected: "); debug_puts(ble_is_connected() ? "yes\n" : "no\n");
}

static void handle_line(const char *line)
{
    /* Tokenize on spaces */
    char tok[CMD_LINE_MAX];
    uint8_t ti = 0;
    while (*line && *line == ' ') line++;
    while (*line && *line != ' ' && ti < sizeof(tok) - 1) tok[ti++] = *line++;
    tok[ti] = 0;
    while (*line == ' ') line++;

    if (ti == 0) return;

    if (strcmp_eq(tok, "help")) { cmd_help(); return; }
    if (strcmp_eq(tok, "status")) { cmd_status(); return; }
    if (strcmp_eq(tok, "log")) {
        char sub[16]; uint8_t si = 0;
        while (*line && *line != ' ' && si < sizeof(sub) - 1) sub[si++] = *line++;
        sub[si] = 0;
        if (strcmp_eq(sub, "close")) sdcard_close_log();
        return;
    }
    if (strcmp_eq(tok, "mode")) {
        char sub[16]; uint8_t si = 0;
        while (*line && *line != ' ' && si < sizeof(sub) - 1) sub[si++] = *line++;
        sub[si] = 0;
        if (strcmp_eq(sub, "idle"))         enter_mode(MODE_IDLE);
        else if (strcmp_eq(sub, "sniffer"))  enter_mode(MODE_SNIFFER);
        else if (strcmp_eq(sub, "mitm"))     enter_mode(MODE_MITM);
        else if (strcmp_eq(sub, "profile"))  enter_mode(MODE_PROFILE);
        else if (strcmp_eq(sub, "glitch"))   enter_mode(MODE_GLITCH);
        else if (strcmp_eq(sub, "exfil"))    enter_mode(MODE_EXFIL);
        else if (strcmp_eq(sub, "fod"))      enter_mode(MODE_FOD_RESEARCH);
        else debug_puts("unknown mode\n");
        return;
    }
    if (strcmp_eq(tok, "tx")) {
        char sub[16]; uint8_t si = 0;
        while (*line && *line != ' ' && si < sizeof(sub) - 1) sub[si++] = *line++;
        sub[si] = 0;
        while (*line == ' ') line++;
        if (strcmp_eq(sub, "start")) {
            uint32_t freq = 140000, duty = 50;
            parse_uint(line, &freq);
            while (*line && *line != ' ') line++;
            while (*line == ' ') line++;
            parse_uint(line, &duty);
            debug_puts(qi_tx_start(freq, (uint8_t)duty) ? "tx start FAIL\n" : "tx start OK\n");
        } else if (strcmp_eq(sub, "stop")) {
            qi_tx_stop(); debug_puts("tx stop OK\n");
        } else if (strcmp_eq(sub, "power")) {
            uint32_t pct = 50;
            parse_uint(line, &pct);
            qi_tx_set_power_pct((uint8_t)pct);
            debug_puts("tx power set\n");
        }
        return;
    }
    if (strcmp_eq(tok, "rx")) {
        char sub[16]; uint8_t si = 0;
        while (*line && *line != ' ' && si < sizeof(sub) - 1) sub[si++] = *line++;
        sub[si] = 0;
        if (strcmp_eq(sub, "telemetry")) {
            uint16_t v = 0, i = 0; int8_t t = 0; uint8_t st = 0;
            if (qi_rx_read_telemetry(&v, &i, &t, &st) == 0) {
                debug_puts("VRECT="); debug_put_u(v); debug_puts(" mV\n");
                debug_puts("IOUT=");  debug_put_u(i); debug_puts(" mA\n");
                debug_puts("TEMP=");  debug_put_u((uint32_t)t); debug_puts(" C\n");
                debug_puts("STATE="); debug_put_x(st); debug_puts("\n");
            } else debug_puts("rx read FAIL\n");
        }
        return;
    }
    if (strcmp_eq(tok, "glitch")) {
        char sub[16]; uint8_t si = 0;
        while (*line && *line != ' ' && si < sizeof(sub) - 1) sub[si++] = *line++;
        sub[si] = 0;
        while (*line == ' ') line++;
        if (strcmp_eq(sub, "arm")) {
            glitch_params_t p = {0};
            p.trigger = TRIG_TIMER;
            parse_uint(line, &p.delay_us);
            while (*line && *line != ' ') line++;
            while (*line == ' ') line++;
            parse_uint(line, &p.width_us);
            while (*line && *line != ' ') line++;
            while (*line == ' ') line++;
            parse_uint(line, &p.repeat);
            while (*line && *line != ' ') line++;
            while (*line == ' ') line++;
            int32_t ramp = 0;
            parse_uint(line, (uint32_t *)&ramp);
            p.ramp_us = ramp;
            debug_puts(glitch_engine_arm(&p) ? "arm FAIL\n" : "arm OK\n");
        } else if (strcmp_eq(sub, "fire")) {
            debug_puts(glitch_engine_fire() ? "fire FAIL\n" : "fire OK\n");
        } else if (strcmp_eq(sub, "disarm")) {
            glitch_engine_disarm();
            debug_puts("disarm OK\n");
        }
        return;
    }
    if (strcmp_eq(tok, "profiler")) {
        char sub[16]; uint8_t si = 0;
        while (*line && *line != ' ' && si < sizeof(sub) - 1) sub[si++] = *line++;
        sub[si] = 0;
        if (strcmp_eq(sub, "class")) {
            uint8_t st = 0; float c = 0.0f;
            if (profiler_classify_state(&st, &c) == 0) {
                debug_puts("state="); debug_put_u(st);
                debug_puts(" conf="); debug_put_u((uint32_t)(c * 1000)); debug_puts("\n");
            } else debug_puts("no match\n");
        }
        return;
    }
    if (strcmp_eq(tok, "fod")) {
        char sub[16]; uint8_t si = 0;
        while (*line && *line != ' ' && si < sizeof(sub) - 1) sub[si++] = *line++;
        sub[si] = 0;
        while (*line == ' ') line++;
        if (strcmp_eq(sub, "unlock")) {
            uint32_t code = 0;
            parse_uint(line, &code);
            debug_puts(fod_set_safety_unlock(code) ? "unlock FAIL\n" : "unlock OK\n");
        } else if (strcmp_eq(sub, "status")) {
            int32_t off = 0; int8_t t = 0;
            fod_get_status(&off, &t);
            debug_puts("offset="); debug_put_u((uint32_t)off);
            debug_puts(" temp="); debug_put_u((uint32_t)t); debug_puts("\n");
        }
        return;
    }
    debug_puts("unknown command (try 'help')\n");
}

/* ---- OLED status refresh ---------------------------------------------- */
static void refresh_oled(void)
{
    static uint32_t tick = 0;
    if (++tick < 200000) return;  /* throttle ~ every 0.5 s */
    tick = 0;
    oled_draw_status(mode_name(g_mode),
                     ble_is_connected() ? "BLE" : "USB",
                     g_coil_temp_c, g_current_a, g_power_w);
}

/* ---- Main ------------------------------------------------------------- */

int main(void)
{
    board_init();
    debug_puts("\nCoilGrip v1.0  author: jayis1\n");
    debug_puts("Wireless Power Transfer Manipulation Platform\n");

    oled_init();
    oled_draw_status("BOOT", "init", 25, 0.0f, 0.0f);

    qi_rx_init();
    load_mod_init();
    ble_init();
    usb_cdc_init();
    sdcard_init();
    config_load();
    fod_calibrate();

    debug_puts("init done.\n");
    oled_draw_status("IDLE", "ready", 25, 0.0f, 0.0f);

    for (;;) {
        /* 1. Service USB CDC console */
        usb_cdc_poll();
        uint8_t usb_rx[64];
        int n = usb_cdc_read(usb_rx, sizeof(usb_rx));
        for (int i = 0; i < n; i++) {
            char c = (char)usb_rx[i];
            if (c == '\r') continue;
            if (c == '\n') {
                g_cmd_line[g_cmd_len] = 0;
                handle_line(g_cmd_line);
                g_cmd_len = 0;
            } else if (g_cmd_len < sizeof(g_cmd_line) - 1) {
                g_cmd_line[g_cmd_len++] = c;
            }
        }

        /* 2. Service BLE commands (encrypted) */
        uint8_t ble_buf[64]; uint16_t ble_len = 0;
        if (ble_recv(ble_buf, sizeof(ble_buf), &ble_len, 50) == 0) {
            /* BLE commands use the same text protocol; echo to console. */
            for (uint16_t i = 0; i < ble_len; i++) {
                char c = (char)ble_buf[i];
                if (c == '\n') {
                    g_cmd_line[g_cmd_len] = 0;
                    handle_line(g_cmd_line);
                    g_cmd_len = 0;
                } else if (g_cmd_len < sizeof(g_cmd_line) - 1) {
                    g_cmd_line[g_cmd_len++] = c;
                }
            }
        }

        /* 3. Thermal interlock always runs */
        fod_thermal_interlock_check();

        /* 4. Mode tick */
        switch (g_mode) {
        case MODE_SNIFFER:      mode_sniffer_tick(); break;
        case MODE_MITM:         mode_mitm_tick();    break;
        case MODE_PROFILE:      mode_profile_tick(); break;
        case MODE_GLITCH:       mode_glitch_tick();  break;
        case MODE_EXFIL:        mode_exfil_tick();   break;
        case MODE_FOD_RESEARCH: mode_fod_tick();     break;
        default: break;
        }

        /* 5. Update OLED */
        refresh_oled();
    }
    return 0;
}

/* ---- Interrupt vectors (stubbed; the real startup file would define these) ----
 * On real hardware the vector table at 0x08000000 points to these handlers.
 * We provide weak defaults so the linker is happy. */

void NMI_Handler(void)        { for (;;) {} }
void HardFault_Handler(void)   { debug_puts("HARDFAULT\n"); for (;;) {} }
void MemManage_Handler(void)   { for (;;) {} }
void BusFault_Handler(void)    { debug_puts("BUSFAULT\n"); for (;;) {} }
void UsageFault_Handler(void)  { for (;;) {} }
void SVC_Handler(void)         { }
void DebugMon_Handler(void)    { }
void PendSV_Handler(void)      { }

void SysTick_Handler(void)
{
    /* 1 kHz tick; could drive a global uptime counter. */
}

/* HRTIM master IRQ — drives the Qi PWM; nothing to do in firmware since
 * the hardware generates the waveform autonomously. */
void HRTIM1_Master_IRQHandler(void)
{
    HRTIM1->ICR = 0xFFFFFFFFU;  /* clear all flags */
}

/* TIM2 IRQ — used by the software glitch path when no FPGA is present. */
void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR = ~TIM_SR_UIF;
        /* Software glitch tick could fire here; for now we poll in main. */
    }
}

/* USART2/3 IRQs — unused (we poll). Provided to satisfy the vector table. */
void USART2_IRQHandler(void) { }
void USART3_IRQHandler(void) { }
void I2C1_IRQHandler(void)   { }
void SPI1_IRQHandler(void)   { }
void SPI2_IRQHandler(void)   { }