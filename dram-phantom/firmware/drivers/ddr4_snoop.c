/*
 * ddr4_snoop.c — decode FPGA snoop records into a row-access trace
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * The FPGA pushes compact 64-byte records (only the first 12 bytes used):
 *   [0]     bank_group
 *   [1]     bank
 *   [2..4]  row (24-bit)
 *   [5..6]  column (16-bit)
 *   [7]     op code (0=ACT,1=READ,2=WRITE,3=PRE,4=REF,5=REFSB)
 *   [8..11] timestamp (32-bit, in FPGA clock ticks)
 * This module decodes them into a human-readable struct for the app.
 */

#include "board.h"
#include <string.h>

typedef struct {
    uint8_t  bg;
    uint8_t  bank;
    uint32_t row;
    uint16_t col;
    uint8_t  op;
    uint32_t t;
} snoop_rec_t;

static const char *op_name(uint8_t op) {
    switch (op) {
        case 0: return "ACT";
        case 1: return "RD ";
        case 2: return "WR ";
        case 3: return "PRE";
        case 4: return "REF";
        case 5: return "RFSB";
        default: return "???";
    }
}

int ddr4_snoop_decode(const void *rec, void *out, uint16_t n) {
    const uint8_t *in = (const uint8_t *)rec;
    snoop_rec_t *o = (snoop_rec_t *)out;
    for (uint16_t i = 0; i < n; i++) {
        const uint8_t *r = in + i * 64;
        o[i].bg   = r[0];
        o[i].bank = r[1];
        o[i].row  = (uint32_t)r[2] | ((uint32_t)r[3]<<8) | ((uint32_t)r[4]<<16);
        o[i].col  = (uint16_t)r[5] | ((uint16_t)r[6]<<8);
        o[i].op   = r[7];
        o[i].t    = (uint32_t)r[8] | ((uint32_t)r[9]<<8)
                  | ((uint32_t)r[10]<<16) | ((uint32_t)r[11]<<24);
    }
    return 0;
}

/* Count row activations per (bg,bank,row) tuple — a quick heuristic for
 * detecting Rowhammer-like activity in the host's own traffic. */
typedef struct {
    uint8_t  bg, bank;
    uint32_t row;
    uint32_t acts;
} row_stat_t;

#define ROW_STAT_CAP 128
static row_stat_t g_stats[ROW_STAT_CAP];
static uint16_t g_stats_n = 0;

static row_stat_t *stat_find(uint8_t bg, uint8_t bank, uint32_t row) {
    for (uint16_t i = 0; i < g_stats_n; i++) {
        if (g_stats[i].bg == bg && g_stats[i].bank == bank && g_stats[i].row == row)
            return &g_stats[i];
    }
    if (g_stats_n >= ROW_STAT_CAP) return NULL;
    row_stat_t *s = &g_stats[g_stats_n++];
    s->bg = bg; s->bank = bank; s->row = row; s->acts = 0;
    return s;
}

uint16_t ddr4_snoop_analyze(const void *rec, uint16_t n) {
    g_stats_n = 0;
    snoop_rec_t dec[32];
    for (uint16_t i = 0; i < n; ) {
        uint16_t chunk = n - i; if (chunk > 32) chunk = 32;
        ddr4_snoop_decode((const uint8_t *)rec + i*64, dec, chunk);
        for (uint16_t j = 0; j < chunk; j++) {
            if (dec[j].op == 0) { /* ACT */
                row_stat_t *s = stat_find(dec[j].bg, dec[j].bank, dec[j].row);
                if (s) s->acts++;
            }
        }
        i += chunk;
    }
    /* Return the count of rows that exceeded a 1000-activation threshold
     * in this window — a Rowhammer indicator. */
    uint16_t hot = 0;
    for (uint16_t i = 0; i < g_stats_n; i++) {
        if (g_stats[i].acts > 1000) hot++;
    }
    return hot;
}