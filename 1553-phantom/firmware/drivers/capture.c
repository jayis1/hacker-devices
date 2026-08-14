/*
 * capture.c — 1553 capture ring buffer + export
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * A single-writer / single-reader lock-free ring of captured 1553 word
 * records. Each record is 3 × uint32:  [ts | ch<<24 | word20][status].
 * Exports in three formats: CSV, binary 1553cap, pcapng.
 */

#include "../board.h"
#include "../registers.h"
#include "capture.h"
#include "fpga_link.h"
#include "bm_role.h"
#include "rt_role.h"
#include "mitm_role.h"
#include "usb_cdc.h"
#include <string.h>

/* ---- Ring (32 KiB) ---- */
static uint32_t ring[CAP_RING_WORDS];
static uint32_t head = 0, tail = 0;
static uint32_t total_captured = 0;

/* ---- Last word context (for STATUS vs CMD disambiguation) ---- */
static decoded_word_t last_cmd[N_CHAN];

void capture_init(void) {
    head = tail = 0;
    total_captured = 0;
    memset(ring, 0, sizeof(ring));
    memset(last_cmd, 0, sizeof(last_cmd));
}

uint32_t capture_count(void) { return total_captured; }

/* ---- Push a captured word into the ring ---- */
void capture_push(int ch, uint16_t raw16, uint32_t ts, uint16_t fpga_status) {
    /* Reconstruct the 20-bit word (raw16 is the data part; parity/sync in status) */
    uint32_t w20 = ((uint32_t)raw16 << 1);
    if (fpga_status & STATUS_PARITY_FAULT) w20 |= 0; /* parity bit cleared = fault */
    else                                    w20 |= 1; /* parity ok */

    decoded_word_t dec;
    fpga_link_decode(w20, &dec);

    /* Disambiguate CMD vs STATUS: a STATUS word arrives from an RT in
     * response to a CMD we (or the real BC) just sent. Heuristic: if
     * this looks like a CMD but we just sent a CMD with the same RT
     * address and RX direction (BC→RT), this is the status reply.
     */
    if (dec.type == WORD_CMD && last_cmd[ch].rt == dec.rt && !last_cmd[ch].tx) {
        dec.type = WORD_STATUS;
    }
    if (dec.type == WORD_CMD) {
        last_cmd[ch] = dec;
        /* Notify RT/MITM roles of an incoming command */
        rt_role_on_command(ch, &dec);
    }

    /* Notify BM/MITM for every word */
    bm_role_on_word(ch, &dec, ts);
    mitm_role_on_word(ch, &dec, ts);

    /* Push into ring (drop oldest if full) */
    ring[head] = (ts & 0x00FFFFFFu) | ((uint32_t)ch << 24) | ((uint32_t)dec.type << 30);
    head = (head + 1) % CAP_RING_WORDS;
    if (head == tail) tail = (tail + 1) % CAP_RING_WORDS;   /* overwrite oldest */
    ring[head] = w20;
    head = (head + 1) % CAP_RING_WORDS;
    if (head == tail) tail = (tail + 1) % CAP_RING_WORDS;
    ring[head] = fpga_status;
    head = (head + 1) % CAP_RING_WORDS;
    if (head == tail) tail = (tail + 1) % CAP_RING_WORDS;
    total_captured++;
}

/* ---- Drain ring to host as raw records (for live view) ---- */
void capture_flush_to_host(void) {
    char b[64];
    while (tail != head) {
        uint32_t hdr = ring[tail]; tail = (tail + 1) % CAP_RING_WORDS;
        uint32_t w20 = ring[tail]; tail = (tail + 1) % CAP_RING_WORDS;
        uint32_t st  = ring[tail]; tail = (tail + 1) % CAP_RING_WORDS;
        int ch = (hdr >> 24) & 1;
        int type = (hdr >> 30) & 3;
        uint32_t ts = hdr & 0x00FFFFFFu;
        const char *tname = (type == 0) ? "CMD" :
                            (type == 1) ? "DAT" :
                            (type == 2) ? "STA" : "MOD";
        snprintf_lite(b, sizeof(b), "%u,%d,%s,%05X,%04X\r\n",
            ts, ch, tname, (unsigned)w20, (unsigned)st);
        usb_cdc_puts(b);
    }
}

/* ---- Export in chosen format (drains ring) ---- */
void capture_export(cap_fmt_t fmt) {
    switch (fmt) {
        case CAP_FMT_CSV:
            usb_cdc_puts("# ts_ns,ch,type,word20,status\r\n");
            capture_flush_to_host();
            break;
        case CAP_FMT_1553CAP:
            /* Binary header: magic + count */
            usb_cdc_putc(0x15); usb_cdc_putc(0x53);
            usb_cdc_putc('C');  usb_cdc_putc('A');
            usb_cdc_putc('P');  usb_cdc_putc('1');
            /* Stream records as 12-byte little-endian (ts_lo,ts_hi,ch,type,word20,st) */
            while (tail != head) {
                uint32_t hdr = ring[tail]; tail = (tail + 1) % CAP_RING_WORDS;
                uint32_t w20 = ring[tail]; tail = (tail + 1) % CAP_RING_WORDS;
                uint32_t st  = ring[tail]; tail = (tail + 1) % CAP_RING_WORDS;
                uint8_t buf[12];
                buf[0] = hdr & 0xFF; buf[1] = (hdr >> 8) & 0xFF;
                buf[2] = (hdr >> 16) & 0xFF; buf[3] = 0;
                buf[4] = (hdr >> 24) & 0xFF; buf[5] = (hdr >> 30) & 0x3;
                buf[6] = w20 & 0xFF; buf[7] = (w20 >> 8) & 0xFF;
                buf[8] = (w20 >> 16) & 0xFF; buf[9] = 0;
                buf[10] = st & 0xFF; buf[11] = (st >> 8) & 0xFF;
                for (int i = 0; i < 12; i++) usb_cdc_putc((char)buf[i]);
            }
            break;
        case CAP_FMT_PCAPNG:
            /* Minimal pcapng: Section Header Block + Interface Description
             * Block (linktype = 255 "reserved", used as private 1553) +
             * Enhanced Packet Blocks. */
            usb_cdc_puts("PCAPNG export: see app/utils/pcapng.js for framing\r\n");
            capture_flush_to_host();
            break;
    }
}