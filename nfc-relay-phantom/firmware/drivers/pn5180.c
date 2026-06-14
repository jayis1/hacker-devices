/*
 * PN5180 NFC Frontend Controller Driver
 * Handles ISO 14443A/B, FeliCa, ISO 15693 protocols
 *
 * Copyright (c) 2026 Hacker Devices. Licensed under GPL-2.0.
 */

#include "pn5180.h"
#include "../board.h"
#include "../registers.h"
#include "spi_dma.h"
#include <string.h>

/* SPI frame format for PN5180:
 * Write register: [0x01] [reg_addr_24bit] [data_32bit]
 * Read register:  [0x02] [reg_addr_24bit] → [data_32bit]
 * Send command:   [0x00] [cmd_byte]
 * Write data:     [0x03] [data...]
 * Read data:      [0x04] [data...]
 */

#define PN5180_SPI_WRITE_REG    0x01U
#define PN5180_SPI_READ_REG     0x02U
#define PN5180_SPI_CMD          0x00U
#define PN5180_SPI_WRITE_DATA   0x03U
#define PN5180_SPI_READ_DATA    0x04U

/* ISO 14443A commands */
#define ISO14443A_CMD_WUPA      0x52U
#define ISO14443A_CMD_ANTICOLL  0x93U
#define ISO14443A_CMD_SELECT     0x93U
#define ISO14443A_CMD_RATS       0xE0U

/* ======================================================================
 * Internal SPI Helpers
 * ====================================================================== */
static void pn5180_write_register(uint32_t reg, uint32_t val) {
    uint8_t tx[8];
    uint8_t rx[8];

    tx[0] = PN5180_SPI_WRITE_REG;
    tx[1] = (reg >> 16) & 0xFF;
    tx[2] = (reg >> 8) & 0xFF;
    tx[3] = reg & 0xFF;
    tx[4] = val & 0xFF;
    tx[5] = (val >> 8) & 0xFF;
    tx[6] = (val >> 16) & 0xFF;
    tx[7] = (val >> 24) & 0xFF;

    spi_transfer_blocking(SPI_BUS_1, tx, rx, 8);
}

static uint32_t pn5180_read_register(uint32_t reg) {
    uint8_t tx[8] = {0};
    uint8_t rx[8] = {0};

    tx[0] = PN5180_SPI_READ_REG;
    tx[1] = (reg >> 16) & 0xFF;
    tx[2] = (reg >> 8) & 0xFF;
    tx[3] = reg & 0xFF;

    spi_transfer_blocking(SPI_BUS_1, tx, rx, 8);

    return ((uint32_t)rx[7] << 24) | ((uint32_t)rx[6] << 16) |
           ((uint32_t)rx[5] << 8) | rx[4];
}

static void pn5180_send_command(uint8_t cmd) {
    uint8_t tx[2] = {PN5180_SPI_CMD, cmd};
    spi_transfer_blocking(SPI_BUS_1, tx, NULL, 2);
}

static void pn5180_write_data(const uint8_t *data, size_t len) {
    uint8_t tx[1] = {PN5180_SPI_WRITE_DATA};
    spi_transfer_blocking(SPI_BUS_1, tx, NULL, 1);
    spi_transfer_blocking(SPI_BUS_1, data, NULL, len);
}

static void pn5180_read_data(uint8_t *buf, size_t len) {
    uint8_t tx[1] = {PN5180_SPI_READ_DATA};
    spi_transfer_blocking(SPI_BUS_1, tx, NULL, 1);
    spi_transfer_blocking(SPI_BUS_1, NULL, buf, len);
}

/* ======================================================================
 * Public Functions
 * ====================================================================== */
void pn5180_reset(void) {
    PN5180_RST_LOW();
    /* Wait 1ms for reset */
    for (volatile int i = 0; i < 120000; i++);
    PN5180_RST_HIGH();
    /* Wait 10ms for boot */
    for (volatile int i = 0; i < 1200000; i++);
}

void pn5180_init(void) {
    pn5180_reset();

    /* Clear all IRQ flags */
    pn5180_clear_irq(0xFFFFFFFF);

    /* Enable IRQ sources */
    pn5180_write_register(PN5180_REG_IRQ_ENABLE,
        PN5180_IRQ_TX_DONE | PN5180_IRQ_RX_DONE |
        PN5180_IRQ_COLLISION | PN5180_IRQ_CRC_ERROR |
        PN5180_IRQ_FIELD_DET | PN5180_IRQ_RF_DET);

    /* Configure system: use internal LDO, auto-sleep off */
    pn5180_write_register(PN5180_REG_SYSTEM_CONFIG, 0x00000001);

    /* Set field on/off timers (in carrier cycles) */
    /* 5ms at 13.56 MHz ≈ 67800 cycles */
    pn5180_write_register(PN5180_REG_FIELD_ON_TIME, 67800);
    pn5180_write_register(PN5180_REG_FIELD_OFF_TIME, 67800);

    /* Default: load NFC-A 106 kbps config */
    pn5180_load_protocol(NFC_MODE_A_106);
}

void pn5180_load_protocol(nfc_protocol_t proto) {
    /* Each protocol has specific register settings for PN5180.
     * The PN5180 has internal RF configurations that can be loaded
     * via the LOAD_RF_CONFIG command. */

    switch (proto) {
        case NFC_MODE_A_106:
            /* ISO 14443A @ 106 kbps */
            pn5180_send_command(PN5180_CMD_LOAD_RF_CONFIG);
            /* TX config: 100% ASK, Miller coding */
            pn5180_write_register(PN5180_REG_TX_DATA_CFG, 0x00018007);
            /* RX config: ISO 14443A, Miller */
            pn5180_write_register(PN5180_REG_RX_CFG, 0x00180F14);
            /* CRC config: CRC-A, 16-bit */
            pn5180_write_register(PN5180_REG_CRC_CFG, 0x00101840);
            break;

        case NFC_MODE_B_106:
            /* ISO 14443B @ 106 kbps */
            pn5180_send_command(PN5180_CMD_LOAD_RF_CONFIG);
            pn5180_write_register(PN5180_REG_TX_DATA_CFG, 0x00028007);
            pn5180_write_register(PN5180_REG_RX_CFG, 0x00180F14);
            pn5180_write_register(PN5180_REG_CRC_CFG, 0x00101840);
            break;

        case NFC_MODE_F_212:
        case NFC_MODE_F_424:
            /* FeliCa @ 212/424 kbps */
            pn5180_send_command(PN5180_CMD_LOAD_RF_CONFIG);
            pn5180_write_register(PN5180_REG_TX_DATA_CFG, 0x00038007);
            pn5180_write_register(PN5180_REG_RX_CFG, 0x00180F14);
            pn5180_write_register(PN5180_REG_CRC_CFG, 0x00101840);
            break;

        case NFC_MODE_V:
            /* ISO 15693 */
            pn5180_send_command(PN5180_CMD_LOAD_RF_CONFIG);
            pn5180_write_register(PN5180_REG_TX_DATA_CFG, 0x00048007);
            pn5180_write_register(PN5180_REG_RX_CFG, 0x00180F14);
            pn5180_write_register(PN5180_REG_CRC_CFG, 0x00101840);
            break;

        default:
            break;
    }
}

void pn5180_field_on(void) {
    pn5180_send_command(PN5180_CMD_ACTIVATE_TX);
    /* Wait for field to stabilize */
    for (volatile int i = 0; i < 600000; i++);  /* ~5ms */
}

void pn5180_field_off(void) {
    pn5180_send_command(PN5180_CMD_DEACTIVATE_TX);
}

bool pn5180_detect_card(nfc_context_t *ctx) {
    uint8_t wupa_cmd[1] = {ISO14443A_CMD_WUPA};
    uint8_t rx_buf[10];
    size_t rx_len = 0;

    pn5180_load_protocol(NFC_MODE_A_106);
    pn5180_field_on();

    /* Send WUPA (Wakeup All) */
    if (!pn5180_send_frame(wupa_cmd, 1)) {
        pn5180_field_off();
        return false;
    }

    /* Receive ATQA */
    if (!pn5180_recv_frame(rx_buf, &rx_len, 10)) {
        pn5180_field_off();
        return false;
    }

    if (rx_len >= 2) {
        ctx->atqa[0] = rx_buf[0];
        ctx->atqa[1] = rx_buf[1];
    }

    /* Send ANTICOLL (Select Cascade Level 1) */
    uint8_t anticoll_cmd[2] = {ISO14443A_CMD_ANTICOLL, 0x20};
    if (!pn5180_send_frame(anticoll_cmd, 2)) {
        pn5180_field_off();
        return false;
    }

    /* Receive UID + BCC */
    if (!pn5180_recv_frame(rx_buf, &rx_len, 10)) {
        pn5180_field_off();
        return false;
    }

    if (rx_len >= 5) {
        memcpy(ctx->uid, rx_buf, 4);
        ctx->uid_len = 4;
        ctx->sak = 0x08;  /* Default: Mifare Classic */
    }

    pn5180_field_off();
    ctx->protocol = NFC_MODE_A_106;
    ctx->field_on = false;
    return true;
}

bool pn5180_send_frame(const uint8_t *data, size_t len) {
    /* Clear IRQ flags */
    pn5180_clear_irq(PN5180_IRQ_TX_DONE | PN5180_IRQ_CRC_ERROR | PN5180_IRQ_GENERAL_ERR);

    /* Load TX data */
    pn5180_write_data(data, len);

    /* Enable CRC calculation */
    pn5180_write_register(PN5180_REG_CRC_CFG,
        pn5180_read_register(PN5180_REG_CRC_CFG) | 0x01);

    /* Send TRANSCEIVE command */
    pn5180_send_command(PN5180_CMD_TRANSCEIVE);

    /* Wait for TX done (polling) */
    uint32_t timeout = 1000000;
    while (timeout--) {
        uint32_t irq = pn5180_read_irq();
        if (irq & PN5180_IRQ_TX_DONE) {
            pn5180_clear_irq(PN5180_IRQ_TX_DONE);
            return true;
        }
        if (irq & (PN5180_IRQ_CRC_ERROR | PN5180_IRQ_GENERAL_ERR)) {
            pn5180_clear_irq(0xFFFFFFFF);
            return false;
        }
    }

    return false;
}

bool pn5180_recv_frame(uint8_t *buf, size_t *len, uint32_t timeout_ms) {
    /* Wait for RX done or timeout */
    uint32_t timeout = timeout_ms * 120000;  /* Approximate ms to cycles */

    while (timeout--) {
        uint32_t irq = pn5180_read_irq();
        if (irq & PN5180_IRQ_RX_DONE) {
            pn5180_clear_irq(PN5180_IRQ_RX_DONE);

            /* Read received data length from RX status */
            uint32_t rx_status = pn5180_read_register(PN5180_REG_RX_STATUS);
            *len = (rx_status >> 4) & 0x1FF;  /* Number of received bits / 8 */

            if (*len > 0 && *len <= NFC_MAX_FRAME_SIZE) {
                pn5180_read_data(buf, *len);
                return true;
            }
            return false;
        }
        if (irq & (PN5180_IRQ_CRC_ERROR | PN5180_IRQ_COLLISION | PN5180_IRQ_GENERAL_ERR)) {
            pn5180_clear_irq(0xFFFFFFFF);
            return false;
        }
    }

    return false;
}

void pn5180_set_card_emulation(const uint8_t *uid, size_t uid_len,
                                const uint8_t *atqa, uint8_t sak) {
    /* Card emulation mode:
     * - Disable field generation
     * - Configure RX for passive mode
     * - Load UID, ATQA, SAK into response buffers
     * This requires detailed PN5180 passive mode configuration
     * which is protocol-specific. */

    pn5180_field_off();

    /* Enable passive mode (card emulation) */
    uint32_t sys_cfg = pn5180_read_register(PN5180_REG_SYSTEM_CONFIG);
    sys_cfg |= (1 << 1);  /* PASSIVE_MODE bit */
    pn5180_write_register(PN5180_REG_SYSTEM_CONFIG, sys_cfg);

    /* Load ATQA response */
    pn5180_write_register(PN5180_REG_TX_DATA_CFG, (atqa[1] << 8) | atqa[0]);

    /* Load SAK */
    pn5180_write_register(PN5180_REG_TX_MOD_DEPTH, sak);

    /* Enable RX for card mode */
    pn5180_send_command(PN5180_CMD_ACTIVATE_RX);
}

void pn5180_set_sniffer_mode(void) {
    /* Sniffer mode: listen for all NFC frames on the carrier
     * without actively transmitting. Requires configuring
     * PN5180 in receive-only mode with passive detection. */

    pn5180_field_off();

    /* Enable RX with auto-detect */
    uint32_t rx_cfg = pn5180_read_register(PN5180_REG_RX_CFG);
    rx_cfg |= (1 << 31);  /* AUTO_DETECT bit */
    rx_cfg |= (1 << 30);  /* AUTO_COLL bit */
    pn5180_write_register(PN5180_REG_RX_CFG, rx_cfg);

    /* Enable field detection */
    pn5180_write_register(PN5180_REG_IRQ_ENABLE,
        PN5180_IRQ_RX_DONE | PN5180_IRQ_FIELD_DET |
        PN5180_IRQ_COLLISION | PN5180_IRQ_CRC_ERROR);

    /* Start continuous RX */
    pn5180_send_command(PN5180_CMD_ACTIVATE_RX);
}

uint32_t pn5180_read_irq(void) {
    return pn5180_read_register(PN5180_REG_IRQ_STATUS);
}

void pn5180_clear_irq(uint32_t mask) {
    pn5180_write_register(PN5180_REG_IRQ_STATUS, mask);
}