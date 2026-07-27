/*
 * covert_codec.h — Covert channel codec over PTP/NTP fields
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef CHRONOS_PHANTOM_COVERT_CODEC_H
#define CHRONOS_PHANTOM_COVERT_CODEC_H

#include <stdint.h>

#define COVERT_MAX_MSG  128

/* Frame for buffered covert data */
typedef struct {
    uint8_t  tx_buf[COVERT_MAX_MSG];
    uint16_t tx_len;
    uint16_t tx_idx;
    uint8_t  rx_buf[COVERT_MAX_MSG];
    uint16_t rx_len;
    uint16_t rx_overflow;
} covert_state_t;

void covert_init(covert_state_t *st);
int  covert_tx_queue(covert_state_t *st, const uint8_t *data, uint16_t len);
int  covert_tx_next_byte(covert_state_t *st, uint8_t *out);
void covert_rx_add_byte(covert_state_t *st, uint8_t b);
uint16_t covert_rx_drain(covert_state_t *st, uint8_t *out, uint16_t maxlen);

/* CRC-8 for message integrity */
uint8_t covert_crc8(const uint8_t *data, uint16_t len);

#endif /* CHRONOS_PHANTOM_COVERT_CODEC_H */