/*
 * EM4095 125 kHz RFID Reader Driver
 * Handles EM4100, HID Prox, and T5577 cloning
 *
 * Copyright (c) 2026 Hacker Devices. Licensed under GPL-2.0.
 */

#ifndef EM4095_H
#define EM4095_H

#include <stdint.h>
#include <stdbool.h>

#define EM4100_BIT_LEN    64
#define EM4100_BYTE_LEN   8
#define HID_PROX_MAX_BITS  200
#define RFID_125_MAX_BYTES  25

typedef enum {
    RFID_PROTO_EM4100 = 0,
    RFID_PROTO_HID_PROX = 1,
    RFID_PROTO_AWID = 2,
    RFID_PROTO_IO_PROX = 3,
} rfid_125_protocol_t;

typedef struct {
    rfid_125_protocol_t protocol;
    uint8_t data[RFID_125_MAX_BYTES];
    uint8_t bit_count;
    uint32_t facility_code;
    uint32_t card_number;
} rfid_125_context_t;

void em4095_init(void);
void em4095_power_on(void);
void em4095_power_off(void);
bool em4095_read_em4100(rfid_125_context_t *ctx);
bool em4095_read_hid_prox(rfid_125_context_t *ctx);
bool em4095_write_t5577(const rfid_125_context_t *ctx, uint8_t *key);
void em4095_set_modulation(bool on);
void em4095_start_continuous_read(void (*callback)(const rfid_125_context_t *));
void em4095_stop_continuous_read(void);

/* Manchester decoding */
int8_t em4095_manchester_decode(const uint8_t *raw, size_t raw_bits,
                                 uint8_t *decoded, size_t *decoded_bits);

#endif /* EM4095_H */