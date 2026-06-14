/*
 * PN5180 NFC Frontend Controller Driver
 * Handles ISO 14443A/B, FeliCa, ISO 15693 protocols
 *
 * Copyright (c) 2026 Hacker Devices. Licensed under GPL-2.0.
 */

#ifndef PN5180_H
#define PN5180_H

#include <stdint.h>
#include <stdbool.h>

/* PN5180 Register Map */
#define PN5180_REG_SYSTEM_CONFIG       0x00U
#define PN5180_REG_IRQ_STATUS          0x04U
#define PN5180_REG_IRQ_ENABLE          0x08U
#define PN5180_REG_TX_DATA_CFG         0x14U
#define PN5180_REG_TX_MOD_DEPTH        0x18U
#define PN5180_REG_RX_CFG              0x1CU
#define PN5180_REG_RX_STATUS           0x20U
#define PN5180_REG_FIELD_STATUS        0x24U
#define PN5180_REG_FIELD_ON_TIME        0x28U
#define PN5180_REG_FIELD_OFF_TIME       0x2CU
#define PN5180_REG_CRC_CFG             0x30U
#define PN5180_REG_MISC_CFG            0x34U
#define PN5180_REG_ANAT_CONFIG         0x38U
#define PN5180_REG_RF_STATUS           0x3CU

/* PN5180 Commands */
#define PN5180_CMD_ACTIVATE_TX          0x01U
#define PN5180_CMD_DEACTIVATE_TX        0x02U
#define PN5180_CMD_ACTIVATE_RX          0x03U
#define PN5180_CMD_DEACTIVATE_RX        0x04U
#define PN5180_CMD_TRANSCEIVE           0x05U
#define PN5180_CMD_WRITE_DATA           0x08U
#define PN5180_CMD_READ_DATA            0x09U
#define PN5180_CMD_LOAD_RF_CONFIG       0x0AU

/* NFC Protocol Modes */
typedef enum {
    NFC_MODE_A_106 = 0,
    NFC_MODE_A_212 = 1,
    NFC_MODE_A_424 = 2,
    NFC_MODE_B_106 = 3,
    NFC_MODE_F_212 = 4,
    NFC_MODE_F_424 = 5,
    NFC_MODE_V     = 6,
} nfc_protocol_t;

/* Operation modes */
typedef enum {
    NFC_OPS_READER = 0,
    NFC_OPS_CARD_EMUL = 1,
    NFC_OPS_SNIFFER = 2,
} nfc_op_mode_t;

typedef struct {
    nfc_protocol_t protocol;
    nfc_op_mode_t mode;
    uint8_t uid[10];
    uint8_t uid_len;
    uint8_t atqa[2];
    uint8_t sak;
    bool field_on;
} nfc_context_t;

/* Function prototypes */
void pn5180_init(void);
void pn5180_reset(void);
void pn5180_load_protocol(nfc_protocol_t proto);
void pn5180_field_on(void);
void pn5180_field_off(void);
bool pn5180_detect_card(nfc_context_t *ctx);
bool pn5180_send_frame(const uint8_t *data, size_t len);
bool pn5180_recv_frame(uint8_t *buf, size_t *len, uint32_t timeout_ms);
void pn5180_set_card_emulation(const uint8_t *uid, size_t uid_len,
                                const uint8_t *atqa, uint8_t sak);
void pn5180_set_sniffer_mode(void);
uint32_t pn5180_read_irq(void);
void pn5180_clear_irq(uint32_t mask);

/* Internal helpers */
static void pn5180_write_register(uint32_t reg, uint32_t val);
static uint32_t pn5180_read_register(uint32_t reg);
static void pn5180_send_command(uint8_t cmd);
static void pn5180_write_data(const uint8_t *data, size_t len);
static void pn5180_read_data(uint8_t *buf, size_t len);

#endif /* PN5180_H */