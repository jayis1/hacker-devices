/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef RW_RECEIPT_H
#define RW_RECEIPT_H
#include <stdbool.h>
#include <stdint.h>
#define CAPACITY 8u
#define APPEND_MS 50u
#define DURABLE_MS 200u
#define QUALIFY_MS 3u
#define BASELINE_MS 20u
#define ACTION 1u
#define APPEND 2u
#define DURABLE 4u
#define BLOCKED 8u
#define F_BLOCKED_ACTION 1u
#define F_ORPHAN_APPEND 2u
#define F_ORPHAN_DURABLE 4u
#define F_APPEND_LATE 8u
#define F_DURABLE_LATE 16u
#define F_OVERFLOW 32u
#define F_GAP 64u
#define F_STARTUP 128u
/* FIFO entries store elapsed ages, never vulnerable historical timestamps. */
typedef struct {
    uint32_t age;
    uint32_t append_age;
    bool appended;
} Pending;
typedef struct {
    uint32_t time;
    uint32_t faults;
    uint32_t actions;
    uint32_t appended;
    uint32_t receipts;
    uint32_t abandoned;
    uint32_t gaps;
    uint32_t missed;
    uint32_t segments;
    uint32_t max_append;
    uint32_t max_durable;
    uint32_t baseline;
    uint8_t levels;
    uint8_t candidate;
    uint8_t stable[4];
    uint8_t known;
    uint8_t head;
    uint8_t count;
    bool ready;
    Pending pending[CAPACITY];
} Receipt;
uint32_t receipt_add(uint32_t a, uint32_t b);
void receipt_init(Receipt *r, uint32_t now);
void receipt_tick(Receipt *r, uint32_t now, uint8_t raw);
uint32_t receipt_waiting(const Receipt *r);
#endif
