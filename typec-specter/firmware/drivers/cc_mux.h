/*
 * Type-C Specter cable mux control
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#ifndef TYPEC_SPECTER_CC_MUX_H
#define TYPEC_SPECTER_CC_MUX_H

typedef enum {
    TS_USB2_PASSTHROUGH = 0,
    TS_USB2_ISOLATE,
    TS_USB2_MONITOR
} ts_usb2_mode_t;

typedef enum {
    TS_SBU_PASSTHROUGH = 0,
    TS_SBU_ISOLATE,
    TS_SBU_MONITOR
} ts_sbu_mode_t;

void cc_mux_init(void);
void cc_mux_set_usb2_mode(ts_usb2_mode_t mode);
void cc_mux_set_sbu_mode(ts_sbu_mode_t mode);
void cc_mux_force_safe_path(void);
const char *cc_mux_usb2_mode_name(void);
const char *cc_mux_sbu_mode_name(void);

#endif
