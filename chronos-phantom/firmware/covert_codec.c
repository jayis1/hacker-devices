/*
 * covert_codec.c — Covert channel codec implementation
 *
 * Provides a byte-level queue for data exfiltrated over PTP correctionField
 * or NTP root_delay fields. Includes CRC-8 for message integrity.
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#include <string.h>
#include "covert_codec.h"

void covert_init(covert_state_t *st)
{
    memset(st, 0, sizeof(*st));
}

int covert_tx_queue(covert_state_t *st, const uint8_t *data, uint16_t len)
{
    if (st->tx_len + len > COVERT_MAX_MSG)
        return -1;  /* no room */
    memcpy(&st->tx_buf[st->tx_len], data, len);
    st->tx_len += len;
    return 0;
}

int covert_tx_next_byte(covert_state_t *st, uint8_t *out)
{
    if (st->tx_idx >= st->tx_len)
        return -1;  /* empty */
    *out = st->tx_buf[st->tx_idx++];
    return 0;
}

void covert_rx_add_byte(covert_state_t *st, uint8_t b)
{
    if (st->rx_len >= COVERT_MAX_MSG) {
        st->rx_overflow++;
        return;
    }
    st->rx_buf[st->rx_len++] = b;
}

uint16_t covert_rx_drain(covert_state_t *st, uint8_t *out, uint16_t maxlen)
{
    uint16_t n = st->rx_len;
    if (n > maxlen) n = maxlen;
    memcpy(out, st->rx_buf, n);
    /* Shift remaining */
    if (st->rx_len > n)
        memmove(st->rx_buf, &st->rx_buf[n], st->rx_len - n);
    st->rx_len -= n;
    return n;
}

uint8_t covert_crc8(const uint8_t *data, uint16_t len)
{
    /* CRC-8/MAXIM polynomial 0x31 */
    uint8_t crc = 0;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }
    return crc;
}