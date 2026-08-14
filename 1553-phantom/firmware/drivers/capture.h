/*
 * capture.h — 1553 capture ring + export formats
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdint.h>

typedef enum {
    CAP_FMT_CSV = 0,
    CAP_FMT_1553CAP,
    CAP_FMT_PCAPNG
} cap_fmt_t;

void     capture_init(void);
void     capture_push(int ch, uint16_t raw16, uint32_t ts, uint16_t fpga_status);
uint32_t capture_count(void);
void     capture_flush_to_host(void);
void     capture_export(cap_fmt_t fmt);

#endif /* CAPTURE_H */