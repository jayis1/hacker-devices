/*
 * mitm_role.c — Inline Man-in-the-Middle: forward A<->B with match/replace
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * MITM mode requires the device to be wired inline between an LRU drop
 * and the rest of the bus (ch A = bus side, ch B = LRU side, or vice
 * versa). The device forwards every received word to the opposite
 * channel, except where a rule matches — then it replaces, drops, or
 * injects.
 */

#include "../board.h"
#include "../registers.h"
#include "mitm_role.h"
#include "fpga_link.h"
#include "capture.h"
#include "usb_cdc.h"
#include <string.h>

#define MAX_RULES 16

typedef enum { RULE_REPL = 0, RULE_DROP, RULE_INJECT } rule_kind_t;

typedef struct {
    uint8_t    active;
    rule_kind_t kind;
    uint8_t    ch;           /* channel to apply rule on (incoming) */
    uint32_t   match_mask;   /* bits set here are matched in the word */
    uint32_t   match_val;    /* word AND mask == match_val to match   */
    uint32_t   repl;         /* replacement word (RULE_REPL)          */
    uint32_t   inject_at_us; /* schedule offset (RULE_INJECT)        */
    uint32_t   inject_word;
    uint32_t   last_fire_ts;
} mitm_rule_t;

static mitm_rule_t rules[MAX_RULES];
static uint8_t     running = 0;

void mitm_role_init(void) { memset(rules, 0, sizeof(rules)); }

void mitm_role_start(void) {
    if (!fpga_link_armed()) return;
    running = 1;
}
void mitm_role_stop(void) { running = 0; }

int mitm_role_rule_count(void) {
    int n = 0;
    for (int i = 0; i < MAX_RULES; i++) if (rules[i].active) n++;
    return n;
}

/* ---- CLI ---- */
void mitm_role_cli(int argc, char argv[8][32]) {
    if (argc < 2) return;
    if (!strcmp(argv[1], "add") && argc == 5) {
        int ch = atoi_lite(argv[2]);
        uint32_t m = hex32(argv[3]);
        uint32_t r = hex32(argv[4]);
        for (int i = 0; i < MAX_RULES; i++) {
            if (!rules[i].active) {
                rules[i].active     = 1;
                rules[i].kind       = RULE_REPL;
                rules[i].ch         = (uint8_t)ch;
                rules[i].match_mask  = 0xFFFFF;   /* full 20-bit word */
                rules[i].match_val   = m;
                rules[i].repl        = r;
                usb_cdc_puts("mitm: rule added\r\n");
                return;
            }
        }
        usb_cdc_puts("mitm: table full\r\n");
        return;
    }
    if (!strcmp(argv[1], "drop") && argc == 4) {
        int ch = atoi_lite(argv[2]);
        uint32_t m = hex32(argv[3]);
        for (int i = 0; i < MAX_RULES; i++) {
            if (!rules[i].active) {
                rules[i].active    = 1;
                rules[i].kind      = RULE_DROP;
                rules[i].ch        = (uint8_t)ch;
                rules[i].match_mask = 0xFFFFF;
                rules[i].match_val  = m;
                usb_cdc_puts("mitm: drop rule added\r\n");
                return;
            }
        }
        return;
    }
    if (!strcmp(argv[1], "inject") && argc == 5) {
        int ch = atoi_lite(argv[2]);
        uint32_t at = hex32(argv[3]);
        uint32_t w  = hex32(argv[4]);
        for (int i = 0; i < MAX_RULES; i++) {
            if (!rules[i].active) {
                rules[i].active = 1;
                rules[i].kind   = RULE_INJECT;
                rules[i].ch     = (uint8_t)ch;
                rules[i].inject_at_us = at;
                rules[i].inject_word  = w;
                usb_cdc_puts("mitm: inject rule added\r\n");
                return;
            }
        }
        return;
    }
    if (!strcmp(argv[1], "clear")) {
        memset(rules, 0, sizeof(rules));
        usb_cdc_puts("mitm: rules cleared\r\n");
        return;
    }
    usb_cdc_puts("mitm: bad subcmd\r\n");
}

/* ---- Called by capture.c on every captured word ---- */
void mitm_role_on_word(int ch, const decoded_word_t *w, uint32_t ts) {
    if (!running) return;
    int dst = ch ^ 1;     /* forward to opposite channel */

    /* Apply rules in order; first match wins */
    for (int i = 0; i < MAX_RULES; i++) {
        if (!rules[i].active) continue;
        if (rules[i].ch != ch) continue;
        if (rules[i].kind == RULE_INJECT) continue;   /* not event-driven */
        if ((w->raw & rules[i].match_mask) == rules[i].match_val) {
            if (rules[i].kind == RULE_DROP) {
                return;     /* silently drop, do not forward */
            }
            if (rules[i].kind == RULE_REPL) {
                fpga_link_tx_word(dst, rules[i].repl);
                rules[i].last_fire_ts = ts;
                return;
            }
        }
    }
    /* No rule matched: forward unchanged */
    fpga_link_tx_word(dst, w->raw);
}

/* ---- Tick: handle time-based inject rules ---- */
void mitm_role_tick(uint32_t now_ms) {
    (void)now_ms;
    /* Time-based injects fire when their scheduled µs offset elapses
     * relative to the last capture. Implementation simplified: fire
     * once on first eligible tick. */
    static uint32_t last_ts[N_CHAN] = {0};
    for (int i = 0; i < MAX_RULES; i++) {
        if (!rules[i].active || rules[i].kind != RULE_INJECT) continue;
        uint32_t now = fpga_link_timestamp();
        if (last_ts[rules[i].ch] == 0) { last_ts[rules[i].ch] = now; continue; }
        if (now - last_ts[rules[i].ch] >= (rules[i].inject_at_us * 96)) {
            fpga_link_tx_word(rules[i].ch ^ 1, rules[i].inject_word);
            rules[i].last_fire_ts = now;
            last_ts[rules[i].ch] = now;
        }
    }
}