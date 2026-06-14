/*
 * atecc608a.h — ATECC608A Crypto Authentication Driver for Substation
 * Provides secure boot, key storage, and BLE authentication
 */

#ifndef ATECC608A_H
#define ATECC608A_H

#include <stdint.h>
#include <stdbool.h>

/* ATECC608A I2C Address (7-bit) */
#define ATECC_I2C_ADDR          0x60

/* ATECC608A Command Opcodes */
#define ATECC_OP_CHECKMAC       0x28
#define ATECC_OP_COUNTER         0x24
#define ATECC_OP_DERIVEKEY      0x1C
#define ATECC_OP_ECDH           0x43
#define ATECC_OP_GENDIG         0x15
#define ATECC_OP_GENKEY         0x40
#define ATECC_OP_HMAC           0x11
#define ATECC_OP_INFO           0x30
#define ATECC_OP_LOCK           0x17
#define ATECC_OP_MAC            0x08
#define ATECC_OP_NONCE          0x16
#define ATECC_OP_PRIVWRITE      0x46
#define ATECC_OP_RANDOM         0x1B
#define ATECC_OP_READ          0x02
#define ATECC_OP_SECUREBOOT     0x20
#define ATECC_OP_SHA            0x47
#define ATECC_OP_SIGN           0x41
#define ATECC_OP_UPDATEEXTRA    0x20
#define ATECC_OP_VERIFY         0x45
#define ATECC_OP_WRITE          0x12

/* ATECC608A Zones */
#define ATECC_ZONE_CONFIG       0x00
#define ATECC_ZONE_OTP          0x01
#define ATECC_ZONE_DATA         0x02

/* Key Slot Definitions */
#define ATECC_SLOT_SECURE_BOOT  0    /* Secure boot verification key */
#define ATECC_SLOT_ECDH_PRIV    1    /* ECDH private key for BLE */
#define ATECC_SLOT_ECDH_PUB     2    /* ECDH public key */
#define ATECC_SLOT_SIGN_PRIV    3    /* Signing private key */
#define ATECC_SLOT_SIGN_PUB     4    /* Signing public key */
#define ATECC_SLOT_HMAC_KEY     5    /* HMAC key */
#define ATECC_SLOT_TEMP         6    /* Temp key */

/* Status codes */
#define ATECC_SUCCESS           0x00
#define ATECC_ERR_CONFIG        0x01
#define ATECC_ERR_OPCODE        0x03
#define ATECC_ERR_CHECKMAC     0x0D
#define ATECC_ERR_TIMEOUT       0xFF

/* Function prototypes */
int  atecc_init(void);
int  atecc_wakeup(void);
int  atecc_sleep(void);
int  atecc_idle(void);

/* Random number generation */
int  atecc_random(uint8_t *rand_out);

/* SHA-256 hash */
int  atecc_sha_start(void);
int  atecc_sha_update(const uint8_t *data, uint16_t len);
int  atecc_sha_finish(uint8_t *digest);

/* ECDSA signature */
int  atecc_sign(uint8_t slot, const uint8_t *msg, uint16_t msg_len,
                uint8_t *signature);
int  atecc_verify(const uint8_t *msg, uint16_t msg_len,
                  const uint8_t *signature, const uint8_t *pub_key);

/* ECDH key agreement */
int  atecc_genkey(uint8_t slot, uint8_t *pub_key);
int  atecc_ecdh(uint8_t slot, const uint8_t *pub_key, uint8_t *shared_secret);

/* Secure boot */
int  atecc_secure_boot(const uint8_t *digest, uint8_t slot, uint8_t signature_id);

/* Key storage */
int  atecc_write_zone(uint8_t zone, uint16_t addr, const uint8_t *data, uint8_t len);
int  atecc_read_zone(uint8_t zone, uint16_t addr, uint8_t *data, uint8_t len);

/* Monotonic counter */
int  atecc_counter_increment(uint8_t counter_id, uint32_t *new_value);
int  atecc_counter_read(uint8_t counter_id, uint32_t *value);

/* CRC helper */
uint16_t atecc_crc16(const uint8_t *data, uint16_t len);

#endif /* ATECC608A_H */