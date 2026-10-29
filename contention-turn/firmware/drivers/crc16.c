/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "crc16.h"
/* CCITT-FALSE: polynomial 0x1021, initial FFFF, no reflection or final xor.
   This is accidental-corruption detection, not a message authenticator. */
uint16_t crc16_byte(uint16_t state, uint8_t value)
{
    state ^= (uint16_t)value << 8;
    for (unsigned i = 0; i < 8; ++i) {
        bool carry = (state & 0x8000u) != 0;
        state = (uint16_t)(state << 1);
        if (carry)
            state ^= 0x1021u;
    }
    return state;
}
/* Null with zero length denotes the empty message; output is left untouched
   on invalid arguments, so callers cannot accidentally use partial results. */
bool crc16_buffer(const void *data, size_t length, uint16_t *result)
{
    if (!result)
        return false;
    if (!data && length)
        return false;
    const uint8_t *bytes = data;
    uint16_t state = 0xffffu;
    for (size_t i = 0; i < length; ++i)
        state = crc16_byte(state, bytes[i]);
    *result = state;
    return true;
}
bool crc16_verify(const void *data, size_t length, uint16_t expected)
{
    uint16_t actual;
    if (!crc16_buffer(data, length, &actual))
        return false;
    return actual == expected;
}
