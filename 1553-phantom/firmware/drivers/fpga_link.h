/*
 * fpga_link.h — SPI link + 1553 word codec helpers
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef FPGA_LINK_H
#define FPGA_LINK_H

#include <stdint.h>
#include "../board.h"

/* ---- Decoded 1553 word types ---- */
typedef enum {
    WORD_CMD = 0,
    WORD_DATA,
    WORD_STATUS,    /* set by capture.c from context */
    WORD_MODE       /* command with SA=0 or SA=31 */
} word_type_t;

typedef struct {
    uint32_t    raw;        /* 20-bit raw word (parity in bit 0) */
    word_type_t type;
    uint8_t     rt;         /* RT address (0..30) */
    uint8_t     sa;         /* subaddress (0..31) */
    uint8_t     wc;         /* word count (0..31, 0 = 32) */
    uint8_t     tx;         /* TX (1) / RX (0) bit */
    uint16_t    data;       /* data word payload (WORD_DATA) */
    uint8_t     parity_ok;
} decoded_word_t;

/* ---- API ---- */
void     fpga_link_init(void);
uint16_t fpga_link_status(void);
uint32_t fpga_link_timestamp(void);
uint32_t fpga_link_chan_errs(int ch);

int      fpga_link_tx_word(int ch, uint32_t word20);
void     fpga_link_pump(void);
void     fpga_link_on_irq(void);

void     fpga_link_arm(void);
void     fpga_link_disarm(void);
int      fpga_link_armed(void);

void     fpga_link_set_fuzz(uint16_t mask);
void     fpga_link_set_gap(uint16_t us);

/* 1553 word encode/decode helpers (parity odd-over-16-data) */
uint32_t fpga_link_make_cmd    (uint8_t rt, int tx, uint8_t sa, uint8_t wc);
uint32_t fpga_link_make_status (uint8_t rt, uint16_t fault_mask);
uint32_t fpga_link_make_data   (uint16_t data);
void     fpga_link_decode      (uint32_t w20, decoded_word_t *out);

#endif /* FPGA_LINK_H */