/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef CS_ENGINE_H
#define CS_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#define CS_CAPACITY 8u
#define CS_SETUP 5u
#define CS_QUALIFY 3u
#define CS_BASELINE 20u
enum {
    CS_MISMATCH = 1u,
    CS_UNDERFLOW = 2u,
    CS_OVERFLOW = 4u,
    CS_COLLISION = 8u,
    CS_UNSTABLE = 16u,
    CS_GAP = 32u,
    CS_EXHAUSTED = 64u,
    CS_WATCHDOG = 128u
};
typedef struct {
    uint32_t now;
    uint32_t levels;
    uint32_t valid;
    uint32_t ready;
    uint32_t halted;
    uint32_t faults;
    uint32_t depth;
    uint32_t top;
    uint32_t pushes;
    uint32_t pops;
    uint32_t matched;
    uint32_t abandoned;
    uint32_t mismatch;
    uint32_t underflow;
    uint32_t overflow;
    uint32_t collision;
    uint32_t unstable;
    uint32_t gaps;
    uint32_t missed;
    uint32_t peak;
    uint32_t tx_fail;
    uint8_t tags[CS_CAPACITY];
    uint8_t candidates;
    uint8_t runs[4];
    uint8_t baseline;
    uint8_t setup;
    bool started;
    bool lost;
    bool exhausted;
} Stack;
void stack_init(Stack *s);
void stack_sample(Stack *s, uint32_t now, uint8_t raw);
void stack_halt(Stack *s, uint32_t reason);
void stack_tx_failure(Stack *s);
bool stack_valid(const Stack *s);
#endif
