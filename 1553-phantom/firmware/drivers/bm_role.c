/*
 * bm_role.c — Bus Monitor (passive sniffer) role
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * In BM role the device never transmits. It continuously drains the
 * FPGA RX FIFO into the capture ring and runs a lightweight protocol
 * fault detector (gap violations, RT response timeouts, parity faults).
 */

#include "../board.h"
#include "../registers.h"
#include "bm_role.h"
#include "fpga_link.h"
#include "capture.h"
#include "rt_role.h"
#include "usb_cdc.h"
#include <string.h>

/* ---- Protocol fault counters ---- */
static uint32_t fault_gap;        /* gap < 4 µs between messages    */
static uint32_t fault_rt_timeout; /* RT did not respond in 14 µs    */
static uint32_t fault_parity;
static uint32_t fault_sync;

/* ---- Last activity timestamps (per channel) ---- */
static uint32_t last_word_ts[N_CHAN];

/* ---- Running? ---- */
static uint8_t running = 0;

void bm_role_init(void) { }
void bm_role_start(void) {
    running = 1;
    fault_gap = fault_rt_timeout = fault_parity = fault_sync = 0;
    memset(last_word_ts, 0, sizeof(last_word_ts));
}
void bm_role_stop(void) { running = 0; }

/* ---- Called by capture.c on every captured word ---- */
void bm_role_on_word(int ch, const decoded_word_t *w, uint32_t ts) {
    if (!running) return;

    if (!w->parity_ok) fault_parity++;

    /* Gap analysis: if this is a command word and we saw a previous
     * word on the same channel very recently, the gap may be too small. */
    if (w->type == WORD_CMD && last_word_ts[ch]) {
        uint32_t dt = ts - last_word_ts[ch];
        /* FPGA timestamp is 96 MHz → convert to µs: /96 */
        uint32_t dt_us = dt / 96;
        if (dt_us > 0 && dt_us < 4) {
            fault_gap++;
        }
    }
    last_word_ts[ch] = ts;

    /* If this is a command word, hand it to the RT role so it can
     * decide to respond (in RT role) — in pure BM this is a no-op. */
    if (w->type == WORD_CMD) {
        rt_role_on_command(ch, w);
    }
}

/* ---- Tick: nothing to do in BM (everything is event-driven) ---- */
void bm_role_tick(uint32_t now_ms) {
    (void)now_ms;
    /* Periodic protocol-health summary over USB CDC at low rate */
    static uint32_t last_report = 0;
    if (now_ms - last_report > 10000) {
        last_report = now_ms;
        /* Only verbose if running and there are faults */
        if (running && (fault_gap || fault_rt_timeout || fault_parity)) {
            char b[80];
            snprintf_lite(b, sizeof(b),
                "bm: gap=%u rto=%u par=%u syn=%u\r\n",
                fault_gap, fault_rt_timeout, fault_parity, fault_sync);
            usb_cdc_puts(b);
        }
    }
}

/* ---- CLI ---- */
void bm_role_cli(int argc, char argv[8][32]) {
    if (argc < 2) return;
    if (!strcmp(argv[1], "start")) { bm_role_start(); return; }
    if (!strcmp(argv[1], "stop"))  { bm_role_stop();  return; }
    if (!strcmp(argv[1], "flush")) {
        capture_flush_to_host();
        return;
    }
    if (!strcmp(argv[1], "export") && argc == 3) {
        if (!strcmp(argv[2], "csv"))          capture_export(CAP_FMT_CSV);
        else if (!strcmp(argv[2], "1553cap")) capture_export(CAP_FMT_1553CAP);
        else if (!strcmp(argv[2], "pcapng"))  capture_export(CAP_FMT_PCAPNG);
        else usb_cdc_puts("bm: bad fmt\r\n");
        return;
    }
    usb_cdc_puts("bm: bad subcmd\r\n");
}

uint32_t bm_role_faults(int kind) {
    switch (kind) {
        case 0: return fault_gap;
        case 1: return fault_rt_timeout;
        case 2: return fault_parity;
        case 3: return fault_sync;
        default: return 0;
    }
}