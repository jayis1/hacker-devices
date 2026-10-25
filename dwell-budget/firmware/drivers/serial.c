/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "board.h"
#include "registers.h"
#include <string.h>

static char buffer[SERIAL_BUFFER];
static size_t length;
static size_t position;
static uint32_t started;
static bool timeout;

bool serial_busy(void)
{
    return position < length;
}

bool serial_submit(const char *data, size_t size)
{
    if (!data || serial_busy() || !size || size > sizeof buffer)
        return false;
    memcpy(buffer, data, size);
    length = size;
    position = 0;
    started = board_millis();
    return true;
}

void serial_poll(void)
{
    if (!serial_busy())
        return;
    if (board_millis() - started >= 100u) {
        position = length;
        timeout = true;
        return;
    }
    /* Bounded one-byte service, allowing acquisition between characters. */
    if (USART2_SR & (1u << 7))
        USART2_DR = (uint8_t)buffer[position++];
}

bool serial_take_timeout(void)
{
    bool result = timeout;
    timeout = false;
    return result;
}
