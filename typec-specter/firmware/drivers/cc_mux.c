/*
 * Type-C Specter cable mux control
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include "cc_mux.h"

static ts_usb2_mode_t g_usb2_mode = TS_USB2_PASSTHROUGH;
static ts_sbu_mode_t g_sbu_mode = TS_SBU_PASSTHROUGH;

void cc_mux_init(void) {
    g_usb2_mode = TS_USB2_PASSTHROUGH;
    g_sbu_mode = TS_SBU_PASSTHROUGH;
}

void cc_mux_set_usb2_mode(ts_usb2_mode_t mode) {
    g_usb2_mode = mode;
}

void cc_mux_set_sbu_mode(ts_sbu_mode_t mode) {
    g_sbu_mode = mode;
}

void cc_mux_force_safe_path(void) {
    g_usb2_mode = TS_USB2_PASSTHROUGH;
    g_sbu_mode = TS_SBU_PASSTHROUGH;
}

const char *cc_mux_usb2_mode_name(void) {
    switch (g_usb2_mode) {
        case TS_USB2_ISOLATE: return "isolate";
        case TS_USB2_MONITOR: return "monitor";
        default: return "passthrough";
    }
}

const char *cc_mux_sbu_mode_name(void) {
    switch (g_sbu_mode) {
        case TS_SBU_ISOLATE: return "isolate";
        case TS_SBU_MONITOR: return "monitor";
        default: return "passthrough";
    }
}
