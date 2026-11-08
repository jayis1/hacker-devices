/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
/* Synthetic sampled operations; never represented as physical measurements. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Ledger state;
static uint32_t clock_ms;
static void emit(void)
{
    char out[768];
    size_t n = wire_status(out, sizeof out, &state);
    assert(n);
    fwrite(out, 1, n, stdout);
}
static void hold(unsigned raw, unsigned count)
{
    while (count--) ledger_sample(&state, ++clock_ms, raw);
}
static void op(unsigned tag, bool retain)
{
    hold(tag, 12);
    hold(tag | (retain ? 4u : 8u), 6);
    hold(tag, 6);
    emit();
}
int main(int argc, char **argv)
{
    char out[768];
    size_t n = wire_policy(out, sizeof out);
    fwrite(out, 1, n, stdout);
    ledger_init(&state);
    hold(0, 20);
    emit();
    op(0, true); op(0, true); op(2, true);
    op(0, false); op(2, false); op(0, true);
    op(0, false); op(0, false);
    if (argc > 1 && !strcmp(argv[1], "legal")) return 0;
    if (argc > 1 && !strcmp(argv[1], "underflow")) {
        op(2, false);
        return 0;
    }
    op(3, true); op(3, true); op(3, true); op(3, true);
    return 0;
}
