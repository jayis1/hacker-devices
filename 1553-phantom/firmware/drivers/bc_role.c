/*
 * bc_role.c — Bus Controller role: drives the 1553 bus with a schedule
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * A schedule is a list of "messages" each consisting of:
 *   - channel (0=A, 1=B)
 *   - command word (BC→RT or RT→BC or RT→RT or mode code)
 *   - up to 32 data words (BC→RT only)
 *   - inter-message gap in µs
 *
 * The minor frame is run at MAJOR_MS cadence. Fuzz primitives corrupt
 * selected messages to probe LRU parser robustness.
 */

#include "../board.h"
#include "../registers.h"
#include "bc_role.h"
#include "fpga_link.h"
#include "capture.h"
#include "usb_cdc.h"
#include <string.h>

/* ---- Schedule storage ---- */
#define MAX_MSGS 64
static bc_msg_t sched[MAX_MSGS];
static int      sched_len = 0;
static int      sched_idx = 0;
static uint32_t frame_count = 0;
static uint16_t fuzz_mask = 0;
static uint16_t gap_override_us = 0;
static int      running = 0;

/* ---- Fuzz state ---- */
static uint8_t fault_parity_next = 0;
static uint8_t fault_sync_next   = 0;

void bc_role_init(void) { bc_role_stop(); }
void bc_role_start(void) {
    if (!fpga_link_armed()) return;
    sched_idx = 0;
    frame_count = 0;
    running = 1;
}
void bc_role_stop(void) { running = 0; }

uint32_t bc_role_frame_count(void) { return frame_count; }

/* ---- Schedule loading ----
 * Accepts one CSV line at a time from CLI:
 *   ch,rt,tx,sa,wc,d0,d1,...,d31,gap_us
 * Multiple lines accumulate into sched[] until "bc run".
 */
static int parse_csv(const char *line, bc_msg_t *m) {
    /* Very small CSV parser: tokens separated by commas */
    int f = 0;
    const char *p = line;
    int vals[40];
    int nvals = 0;
    while (*p && nvals < 40) {
        int v = 0, sign = 1;
        if (*p == '-') { sign = -1; p++; }
        while (*p >= '0' && *p <= '9') { v = v*10 + (*p - '0'); p++; }
        vals[nvals++] = v * sign;
        if (*p == ',') p++;
        else break;
    }
    if (nvals < 6) return -1;
    m->ch  = vals[0];
    m->rt  = vals[1];
    m->tx  = vals[2] ? 1 : 0;
    m->sa  = vals[3];
    m->wc  = vals[4];
    int dstart = 5;
    int n_data = nvals - dstart - 1;   /* last is gap */
    if (n_data < 0) n_data = 0;
    if (n_data > 32) n_data = 32;
    m->n_data = n_data;
    for (int i = 0; i < n_data; i++) m->data[i] = (uint16_t)vals[dstart + i];
    m->gap_us = (uint16_t)(nvals > 0 ? vals[nvals-1] : 0);
    return 0;
}

/* ---- CLI ---- */
void bc_role_cli(int argc, char argv[8][32]) {
    if (argc < 2) { return; }
    if (!strcmp(argv[1], "load") && argc >= 4 && !strcmp(argv[2], "csv")) {
        /* Re-join the remaining tokens as a single CSV line */
        char line[160];
        int n = 0;
        for (int i = 3; i < argc && n < 150; i++) {
            int k = 0;
            while (argv[i][k] && n < 150) line[n++] = argv[i][k++];
            if (i + 1 < argc) line[n++] = ',';
        }
        line[n] = 0;
        if (sched_len < MAX_MSGS && parse_csv(line, &sched[sched_len]) == 0) {
            sched_len++;
            usb_cdc_puts("bc: msg loaded\r\n");
        } else {
            usb_cdc_puts("bc: parse error\r\n");
        }
        return;
    }
    if (!strcmp(argv[1], "run"))  { bc_role_start(); return; }
    if (!strcmp(argv[1], "stop")) { bc_role_stop();  return; }
    if (!strcmp(argv[1], "gap") && argc == 3) {
        gap_override_us = (uint16_t)atoi_lite(argv[2]);
        usb_cdc_puts("bc: gap override set\r\n");
        return;
    }
    if (!strcmp(argv[1], "fuzz") && argc == 3) {
        fuzz_mask = (uint16_t)hex32(argv[2]);
        fpga_link_set_fuzz(fuzz_mask);
        usb_cdc_puts("bc: fuzz mask set\r\n");
        return;
    }
    usb_cdc_puts("bc: bad subcmd\r\n");
}

/* ---- Per-tick runner ---- */
void bc_role_tick(uint32_t now_ms) {
    if (!running || sched_len == 0) return;

    static uint32_t next_msg_ms = 0;
    if (now_ms < next_msg_ms) return;

    bc_msg_t *m = &sched[sched_idx];
    uint32_t cmd = fpga_link_make_cmd(m->rt, m->tx, m->sa, m->wc);

    /* Fuzz: inject parity / sync / illegal-length faults on selected msgs */
    if (fuzz_mask & FUZZ_PARITY) cmd ^= 0x1;             /* flip parity bit */
    if (fuzz_mask & FUZZ_SYNC)   cmd ^= 0x60000;         /* corrupt sync head */
    if (fuzz_mask & FUZZ_BADLEN) {
        /* Force word-count field to illegal value via raw override */
        cmd = (cmd & ~(0x1Fu << 5)) | (0x1Fu << 5);
    }

    /* Send command word */
    fpga_link_tx_word(m->ch, cmd);

    /* Send data words if BC→RT */
    if (!m->tx) {
        for (int i = 0; i < m->n_data; i++) {
            uint32_t dw = fpga_link_make_data(m->data[i]);
            if (fuzz_mask & FUZZ_PARITY) dw ^= 0x1;   /* also corrupt data parity */
            fpga_link_tx_word(m->ch, dw);
        }
    }

    /* Advance schedule */
    sched_idx++;
    if (sched_idx >= sched_len) {
        sched_idx = 0;
        frame_count++;
    }

    /* Inter-message gap */
    uint16_t gap = gap_override_us ? gap_override_us : m->gap_us;
    next_msg_ms = now_ms + (gap / 1000) + 1;     /* ms granularity for tick */
    if (gap && gap < 1000) next_msg_ms = now_ms + 1;
}

/* Fuzz mode-code sweep: issue every mode code (0..31) against every RT.
 * Called once per tick when in "fuzz campaign" mode.
 */
void bc_role_fuzz_mode_sweep(uint8_t rt_start, uint8_t rt_end) {
    static uint8_t rt = 0, mc = 0;
    if (rt < rt_start) rt = rt_start;
    /* mode code command: SA=0 (or 31), wc = code */
    uint32_t cmd = fpga_link_make_cmd(rt, 1, 0, mc);   /* TX from RT */
    fpga_link_tx_word(0, cmd);
    mc++;
    if (mc > 31) { mc = 0; rt++; if (rt > rt_end) rt = rt_start; }
}