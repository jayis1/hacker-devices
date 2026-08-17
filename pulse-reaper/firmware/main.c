/*
 * main.c — Pulse-Reaper firmware main loop
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * This is the top-level application. It implements a cooperative state
 * machine that dispatches between the operational modes (TDR, sniff,
 * inject, covert, cable-map) and routes captured frames to the BLE C2
 * backhaul and the microSD capture sink.
 *
 * The firmware is bare-metal (no RTOS) for determinism and auditability.
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

#include "drivers/board_init.h"
#include "drivers/clamp_afc.h"
#include "drivers/fpga_dsp.h"
#include "drivers/tdr_engine.h"
#include "drivers/protocol_detect.h"
#include "drivers/modbus_rtu.h"
#include "drivers/profibus_dp.h"
#include "drivers/hart_fsk.h"
#include "drivers/can_native.h"
#include "drivers/pots_dtmf.h"
#include "drivers/sd_capture.h"
#include "drivers/ble_c2.h"
#include "drivers/crypto.h"
#include "drivers/oled.h"
#include "drivers/power.h"

/* ----------------------------------------------------------------------- */
/*  Globals                                                                */
/* ----------------------------------------------------------------------- */

volatile uint32_t g_dwt_cycles_per_us = 0u;   /* filled by board_init */

static pr_mode_t  g_mode = PR_MODE_IDLE;
static uint32_t   g_mode_entered_tick = 0u;
static uint32_t   g_frames_captured = 0u;
static uint32_t   g_bytes_captured = 0u;
static uint32_t   g_uptime_s = 0u;           /* incremented by SysTick */
static int        g_low_battery = 0;
static int        g_ble_connected = 0;

/* Active protocol parser — set by protocol_detect */
static const pr_protocol_t *g_active_protocol = NULL;

/* BLE C2 receive buffer */
static uint8_t  g_ble_rx_buf[256];
static uint16_t g_ble_rx_len = 0u;

/* ----------------------------------------------------------------------- */
/*  SysTick handler — 1 kHz                                                 */
/* ----------------------------------------------------------------------- */

void SysTick_Handler(void) {
    static uint32_t sub_sec = 0u;
    sub_sec++;
    if (sub_sec >= 1000u) {
        sub_sec = 0u;
        g_uptime_s++;
    }
}

/* ----------------------------------------------------------------------- */
/*  Mode transition helpers                                                 */
/* ----------------------------------------------------------------------- */

static void enter_idle(void) {
    g_mode = PR_MODE_IDLE;
    g_mode_entered_tick = g_uptime_s;
    clamp_inject_enable(0);
    led_set(0, 1);                  /* green = idle */
    oled_show_status("IDLE", NULL);
}

static void enter_tdr(void) {
    g_mode = PR_MODE_TDR;
    g_mode_entered_tick = g_uptime_s;
    led_set(1, 1);                  /* yellow = TDR arming */
    oled_show_status("TDR", "Clamp closed...");
    /* Safety: refuse if jaw is open */
    if (!clamp_jaw_closed()) {
        oled_show_status("TDR ABORT", "Jaw open");
        enter_idle();
        return;
    }
    clamp_set_coupling(PR_COUPLING_BOTH);
    clamp_set_gain(PR_GAIN_LOW);
    clamp_tdr_discharge();
}

static void enter_sniff(void) {
    g_mode = PR_MODE_SNIFF;
    g_mode_entered_tick = g_uptime_s;
    g_frames_captured = 0u;
    g_bytes_captured = 0u;
    led_set(0, 1);                  /* green = capturing */
    oled_show_status("SNIFF", "Auto-detect...");
    clamp_inject_enable(0);
    clamp_set_coupling(PR_COUPLING_BOTH);
    clamp_set_gain(PR_GAIN_MID);
    g_active_protocol = NULL;       /* will be auto-detected */
    sd_capture_open("pulse.pcapng");
}

static void enter_inject(void) {
    /* Inject requires unpowered bus — TDR first to verify */
    g_mode = PR_MODE_INJECT;
    g_mode_entered_tick = g_uptime_s;
    led_set(1, 0);                  /* red = armed inject */
    oled_show_status("INJECT", "Verify unpowered");
    clamp_set_coupling(PR_COUPLING_CAP);
    clamp_set_gain(PR_GAIN_MID);
    /* The inject driver is enabled only when a frame is actually sent. */
    clamp_inject_enable(0);
}

static void enter_covert(void) {
    g_mode = PR_MODE_COVERT;
    g_mode_entered_tick = g_uptime_s;
    led_set(1, 1);
    oled_show_status("COVERT", "Pair mode");
    clamp_set_coupling(PR_COUPLING_CAP);
    clamp_set_gain(PR_GAIN_HIGH);
}

static void enter_cablemap(void) {
    g_mode = PR_MODE_CABLEMAP;
    g_mode_entered_tick = g_uptime_s;
    led_set(0, 1);
    oled_show_status("CABLEMAP", "Walk the tray");
}

/* ----------------------------------------------------------------------- */
/*  Button handling                                                         */
/* ----------------------------------------------------------------------- */

static void handle_buttons(void) {
    static uint32_t last_mode_btn = 0u;
    static uint32_t last_act_btn  = 0u;
    static uint8_t  mode_btn_state = 1u;
    static uint8_t  act_btn_state  = 1u;

    uint8_t mode_now = (uint8_t)((GPIOE->IDR >> BTN_MODE_PIN) & 1u);
    uint8_t act_now  = (uint8_t)((GPIOE->IDR >> BTN_ACTION_PIN) & 1u);

    /* Falling-edge detection (active low) */
    if (mode_now == 0u && mode_btn_state == 1u &&
        (g_uptime_s - last_mode_btn) > 0u) {
        last_mode_btn = g_uptime_s;
        /* Cycle mode: IDLE -> TDR -> SNIFF -> INJECT -> COVERT -> CABLEMAP -> IDLE */
        switch (g_mode) {
        case PR_MODE_IDLE:    enter_tdr();     break;
        case PR_MODE_TDR:     enter_sniff();   break;
        case PR_MODE_SNIFF:   enter_inject();  break;
        case PR_MODE_INJECT:  enter_covert();  break;
        case PR_MODE_COVERT:  enter_cablemap();break;
        case PR_MODE_CABLEMAP:enter_idle();    break;
        }
    }
    mode_btn_state = mode_now;

    if (act_now == 0u && act_btn_state == 1u &&
        (g_uptime_s - last_act_btn) > 0u) {
        last_act_btn = g_uptime_s;
        /* Action: depends on current mode */
        if (g_mode == PR_MODE_TDR) {
            /* Fire TDR */
            tdr_engine_acquire_and_classify();
            tdr_engine_show_result();
        } else if (g_mode == PR_MODE_SNIFF) {
            /* Stop sniff and close capture */
            sd_capture_close();
            oled_show_status("SNIFF STOP", "Capture saved");
            enter_idle();
        } else if (g_mode == PR_MODE_INJECT) {
            /* Default: inject one test frame (Modbus read coils) */
            modbus_inject_test_frame();
            oled_show_status("INJECT", "Sent test frame");
        } else if (g_mode == PR_MODE_COVERT) {
            /* Toggle covert TX/RX role */
            ble_c2_send_status("covert role toggled");
        }
    }
    act_btn_state = act_now;
}

/* ----------------------------------------------------------------------- */
/*  BLE C2 dispatch                                                        */
/* ----------------------------------------------------------------------- */

static void dispatch_ble_command(const uint8_t *buf, uint16_t len) {
    if (len < 1u) return;
    uint8_t cmd = buf[0];

    switch (cmd) {
    case BLE_CMD_PING:
        ble_c2_send_status("pong");
        break;
    case BLE_CMD_GET_STATUS: {
        char line[64];
        /* Minimal snprintf replacement to avoid pulling in full libc */
        uint32_t uptime = g_uptime_s;
        int n = 0;
        const char *p = "uptime=";
        while (*p) line[n++] = *p++;
        /* cheap decimal formatter */
        char tmp[12];
        int ti = 0;
        if (uptime == 0u) tmp[ti++] = '0';
        while (uptime) { tmp[ti++] = (char)('0' + (uptime % 10u)); uptime /= 10u; }
        while (ti) line[n++] = tmp[--ti];
        line[n] = '\0';
        ble_c2_send_status(line);
        break;
    }
    case BLE_CMD_ENTER_MODE: {
        if (len < 2u) break;
        uint8_t m = buf[1];
        switch (m) {
        case 0u: enter_idle();    break;
        case 1u: enter_tdr();     break;
        case 2u: enter_sniff();   break;
        case 3u: enter_inject();  break;
        case 4u: enter_covert();  break;
        case 5u: enter_cablemap();break;
        }
        break;
    }
    case BLE_CMD_FIRE_TDR:
        tdr_engine_acquire_and_classify();
        tdr_engine_send_reflectogram_over_ble();
        break;
    case BLE_CMD_SET_GAIN:
        if (len < 2u) break;
        clamp_set_gain((clamp_gain_t)buf[1]);
        break;
    case BLE_CMD_SET_COUPLING:
        if (len < 2u) break;
        clamp_set_coupling((clamp_coupling_t)buf[1]);
        break;
    case BLE_CMD_INJECT_FRAME:
        if (len < 2u) break;
        /* For now, route to the active protocol's injector if set */
        if (g_active_protocol && g_active_protocol->inject) {
            g_active_protocol->inject(buf + 1, len - 1);
        }
        break;
    case BLE_CMD_STOP_SNIFF:
        sd_capture_close();
        ble_c2_send_status("sniff stopped");
        break;
    case BLE_CMD_FIRMWARE_VERSION:
        ble_c2_send_status("Pulse-Reaper 1.0 (c) jayis1");
        break;
    default:
        ble_c2_send_status("unknown cmd");
        break;
    }
}

/* ----------------------------------------------------------------------- */
/*  Sniff-mode processing                                                    */
/* ----------------------------------------------------------------------- */

static void process_sniff(void) {
    uint8_t frame[260];
    uint16_t flen;

    /* Ask the FPGA DSP for a recovered frame (zero-copy path) */
    int got = fpga_dsp_read_frame(frame, &flen, sizeof(frame));
    if (got <= 0) return;

    /* Auto-detect the protocol on the first frame */
    if (g_active_protocol == NULL) {
        g_active_protocol = protocol_detect(frame, flen);
        if (g_active_protocol) {
            char label[24];
            const char *name = g_active_protocol->name;
            int i = 0;
            const char *prefix = "PROTO:";
            while (prefix[i]) label[i] = prefix[i], i++;
            int j = 0;
            while (name[j] && i < 22) label[i++] = name[j++];
            label[i] = '\0';
            oled_show_status("SNIFF", label);
        }
    }

    /* Parse with the active protocol and format for capture */
    if (g_active_protocol && g_active_protocol->parse) {
        pr_parsed_t parsed;
        if (g_active_protocol->parse(frame, flen, &parsed) == 0) {
            /* Write to microSD as pcapng */
            uint8_t  pcap_buf[280];
            uint16_t pcap_len = sd_capture_format_pcapng(&parsed, pcap_buf, sizeof(pcap_buf));
            if (pcap_len > 0u) {
                sd_capture_write(pcap_buf, pcap_len);
                g_bytes_captured += pcap_len;
            }
            /* Stream over BLE (encrypted) if connected */
            if (g_ble_connected) {
                ble_c2_send_frame(&parsed);
            }
            g_frames_captured++;
        }
    } else {
        /* No parser — store raw bytes */
        sd_capture_write(frame, flen);
        g_bytes_captured += flen;
        g_frames_captured++;
    }

    /* Periodically refresh the OLED stats */
    if ((g_frames_captured & 0x3Fu) == 0u) {
        oled_show_capture_stats(g_frames_captured, g_bytes_captured);
    }
}

/* ----------------------------------------------------------------------- */
/*  Main                                                                    */
/* ----------------------------------------------------------------------- */

int main(void) {
    /* 1. Board bring-up */
    board_init();
    oled_init();
    oled_show_status("PULSE-REAPER", "boot  (c) jayis1");
    board_delay_ms(400);

    /* 2. Initialise drivers */
    clamp_afc_init();
    fpga_dsp_init();
    tdr_engine_init();
    protocol_detect_init();
    modbus_rtu_init();
    profibus_dp_init();
    hart_fsk_init();
    can_native_init();
    pots_dtmf_init();
    sd_capture_init();
    crypto_init();
    power_init();

    /* 3. Bring up BLE C2 */
    ble_c2_init();
    ble_c2_start_advertising("PR-1A2B");
    oled_show_status("BLE", "Advertising");

    /* 4. Enter idle mode */
    enter_idle();

    /* 5. Main loop — cooperative state machine */
    uint32_t last_heartbeat = g_uptime_s;
    while (1) {
        handle_buttons();

        /* Poll BLE C2 */
        uint16_t avail = ble_c2_poll_rx(g_ble_rx_buf, sizeof(g_ble_rx_buf), &g_ble_rx_len);
        if (avail > 0u && g_ble_rx_len > 0u) {
            dispatch_ble_command(g_ble_rx_buf, g_ble_rx_len);
        }
        g_ble_connected = ble_c2_is_connected();

        /* Mode dispatch */
        switch (g_mode) {
        case PR_MODE_SNIFF:
            process_sniff();
            break;
        case PR_MODE_TDR:
            /* TDR is event-driven by the action button / BLE cmd */
            break;
        case PR_MODE_INJECT:
            /* Inject is event-driven; nothing to do in the idle poll */
            break;
        case PR_MODE_COVERT:
            /* Covert channel: pump a low-rate beacon if TX role */
            fpga_dsp_covert_pump();
            break;
        case PR_MODE_CABLEMAP:
            /* Cable map aggregates TDR results — nothing to poll */
            break;
        case PR_MODE_IDLE:
        default:
            break;
        }

        /* Heartbeat: battery check + OLED tick once per second */
        if (g_uptime_s != last_heartbeat) {
            last_heartbeat = g_uptime_s;
            power_poll();
            g_low_battery = power_is_low();
            if (g_low_battery) {
                oled_show_status("LOW BATT", "Charge soon");
                led_set(1, 0);
            }
            /* Refresh status line in idle */
            if (g_mode == PR_MODE_IDLE && !g_low_battery) {
                if (g_ble_connected) {
                    oled_show_status("IDLE", "BLE linked");
                } else {
                    oled_show_status("IDLE", "BLE idle");
                }
            }
        }

        /* WFI between polls to save power (interrupts wake us) */
        __asm volatile ("wfi" ::: "memory");
    }

    return 0;
}