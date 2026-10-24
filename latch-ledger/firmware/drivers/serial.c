/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "registers.h"
#include <string.h>

/* Single foreground owner; no ISR accesses this buffer or its indices. */
static char pending[SERIAL_BUFFER];
static size_t length;
static size_t position;
static uint32_t submitted_at;
static bool timed_out;

bool serial_busy(void)
{
    return position < length;
}

bool serial_submit(const char *data, size_t size)
{
    if (serial_busy() || !size || size > sizeof pending)
        return false;
    memcpy(pending, data, size);
    length = size;
    position = 0;
    submitted_at = board_millis();
    return true;
}

void serial_poll(void)
{
    if (!serial_busy())
        return;
    if (board_millis() - submitted_at >= 100u) {
        position = length;
        timed_out = true;
        return;
    }
    /* One byte per visit keeps the sampling loop bounded. */
    if (USART2_SR & (1u << 7))
        USART2_DR = (uint8_t)pending[position++];
}

bool serial_take_timeout(void)
{
    bool result = timed_out;
    timed_out = false;
    return result;
}
