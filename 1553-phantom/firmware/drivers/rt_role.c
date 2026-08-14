/*
 * rt_role.c — Remote Terminal emulator (spoof up to 8 RT addresses)
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Listens for command words addressed to any of the configured emulated
 * RT addresses and responds with the appropriate status + data words.
 * Supports mode codes and deliberate status-word fault injection.
 */

#include "../board.h"
#include "../registers.h"
#include "rt_role.h"
#include "fpga_link.h"
#include "capture.h"
#include "usb_cdc.h"
#include <string.h>

/* ---- Per-RT substore ---- */
typedef struct {
    uint8_t  active;                /* 1 if this RT slot is emulated */
    uint8_t  addr;                  /* 0..30 */
    uint16_t tx_substore[32][32];   /* [subaddress][word]  */
    uint16_t rx_substore[32][32];   /* receive buffer      */
    uint16_t fault_mask;            /* status-word faults to inject */
    uint16_t last_status;           /* last status word sent */
} rt_slot_t;

static rt_slot_t slots[MAX_RT_EMUL];
static uint8_t   panic = 0;

/* ---- Pending command (captured by capture.c, replayed here) ---- */
static decoded_word_t pending_cmd;
static uint8_t        have_cmd = 0;
static uint32_t       cmd_chan = 0;

void rt_role_init(void) {
    memset(slots, 0, sizeof(slots));
}

void rt_role_start(void) {
    if (!fpga_link_armed()) return;
    /* RT role: we respond to commands. The capture ring is still
     * running (we use it to see incoming commands). bm_role_start()
     * is called implicitly by leaving the RX pump enabled.
     */
}

void rt_role_stop(void) {
    /* On stop, clear all active slots */
    for (int i = 0; i < MAX_RT_EMUL; i++) slots[i].active = 0;
}

int rt_role_emulated_count(void) {
    int n = 0;
    for (int i = 0; i < MAX_RT_EMUL; i++) if (slots[i].active) n++;
    return n;
}

/* ---- Find a slot for a given RT address ---- */
static rt_slot_t *find_slot(uint8_t rt) {
    for (int i = 0; i < MAX_RT_EMUL; i++) {
        if (slots[i].active && slots[i].addr == rt) return &slots[i];
    }
    return NULL;
}

static rt_slot_t *find_free(void) {
    for (int i = 0; i < MAX_RT_EMUL; i++) {
        if (!slots[i].active) return &slots[i];
    }
    return NULL;
}

/* ---- CLI ---- */
void rt_role_cli(int argc, char argv[8][32]) {
    if (argc < 2) return;
    if (!strcmp(argv[1], "set") && argc == 3) {
        uint8_t addr = (uint8_t)atoi_lite(argv[2]);
        if (addr > 30) { usb_cdc_puts("rt: addr 0..30\r\n"); return; }
        rt_slot_t *s = find_slot(addr);
        if (!s) s = find_free();
        if (!s) { usb_cdc_puts("rt: no free slot\r\n"); return; }
        s->active = 1;
        s->addr = addr;
        s->fault_mask = 0;
        usb_cdc_puts("rt: emulating\r\n");
        return;
    }
    if (!strcmp(argv[1], "tx") && argc >= 4) {
        uint8_t sa = (uint8_t)atoi_lite(argv[2]);
        rt_slot_t *s = slots[0].active ? &slots[0] : NULL;
        /* Use the first active slot if no specific RT given */
        if (!s) { usb_cdc_puts("rt: set an RT first\r\n"); return; }
        for (int i = 3; i < argc && i-3 < 32; i++) {
            s->tx_substore[sa & 0x1F][i-3] = (uint16_t)hex32(argv[i]);
        }
        usb_cdc_puts("rt: substore loaded\r\n");
        return;
    }
    if (!strcmp(argv[1], "fault") && argc == 3) {
        rt_slot_t *s = slots[0].active ? &slots[0] : NULL;
        if (!s) { usb_cdc_puts("rt: set an RT first\r\n"); return; }
        s->fault_mask = (uint16_t)hex32(argv[2]);
        usb_cdc_puts("rt: fault mask set\r\n");
        return;
    }
    if (!strcmp(argv[1], "mode") && argc >= 3) {
        /* respond to a mode code with optional data */
        rt_slot_t *s = slots[0].active ? &slots[0] : NULL;
        if (!s) return;
        uint8_t code = (uint8_t)atoi_lite(argv[2]);
        s->tx_substore[0][0] = (argc >= 4) ? (uint16_t)hex32(argv[3]) : 0;
        (void)code;
        usb_cdc_puts("rt: mode response set\r\n");
        return;
    }
    usb_cdc_puts("rt: bad subcmd\r\n");
}

/* ---- Called by capture.c when a command word arrives ---- */
void rt_role_on_command(int ch, const decoded_word_t *cmd) {
    if (panic) return;
    rt_slot_t *s = find_slot(cmd->rt);
    if (!s) return;     /* not our address */
    /* Stash for the tick to handle (avoid sending in ISR-adjacent context) */
    pending_cmd = *cmd;
    have_cmd = 1;
    cmd_chan = ch;
}

/* ---- Tick: send response if we have a pending command ---- */
void rt_role_tick(uint32_t now_ms) {
    (void)now_ms;
    if (!have_cmd) return;
    have_cmd = 0;

    rt_slot_t *s = find_slot(pending_cmd.rt);
    if (!s) return;

    /* Mode code? SA == 0 or SA == 31 */
    int is_mode = (pending_cmd.sa == 0 || pending_cmd.sa == 31);
    uint8_t wc = pending_cmd.wc ? pending_cmd.wc : 32;

    /* Status word first (with injected faults) */
    uint32_t status = fpga_link_make_status(s->addr, s->fault_mask);
    fpga_link_tx_word((int)cmd_chan, status);
    s->last_status = (uint16_t)(status >> 1);

    /* If RT→BC (tx=1), send data words from substore */
    if (pending_cmd.tx && !is_mode) {
        for (int i = 0; i < wc && i < 32; i++) {
            uint32_t dw = fpga_link_make_data(s->tx_substore[pending_cmd.sa][i]);
            fpga_link_tx_word((int)cmd_chan, dw);
        }
    }
    /* If mode code with data word requested, send one word */
    if (is_mode && pending_cmd.tx) {
        uint32_t dw = fpga_link_make_data(s->tx_substore[0][0]);
        fpga_link_tx_word((int)cmd_chan, dw);
    }
}

void rt_role_panic(void) { panic = 1; rt_role_stop(); }