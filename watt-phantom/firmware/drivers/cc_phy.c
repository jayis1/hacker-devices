/*
 * cc_phy.c — FUSB302B CC line transceiver driver
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Drives the FUSB302B BMC transceiver on each USB-C port's CC line.
 * Handles PD message framing, CRC, and GoodCRC auto-reply.
 * The MCU communicates with the FUSB302B over I²C at 1 MHz.
 *
 * The FUSB302B is a USB-C PD front-end IC that:
 *  - Detects CC line attach/detach and orientation
 *  - Encodes/decodes BMC (Biphase Mark Coding) at 600 kbps
 *  - Generates/checks CRC on PD messages
 *  - Auto-replies GoodCRC
 *  - Has a FIFO for TX/RX PD messages
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

/* ---- I²C bus mapping per port ---- */
static const uint32_t s_i2c_bus[PD_PORT_COUNT] = { I2C1_BUS, I2C2_BUS };
static const uint8_t s_i2c_addr[PD_PORT_COUNT] = { FUSB302B_SRC_ADDR, FUSB302B_SNK_ADDR };

/* ---- FUSB302B helper: write a register ---- */
static int fusb_write(pd_port_t port, uint8_t reg, uint8_t val) {
    return i2c_write_reg(s_i2c_bus[port], s_i2c_addr[port], reg, &val, 1);
}

/* ---- FUSB302B helper: read a register ---- */
static int fusb_read(pd_port_t port, uint8_t reg, uint8_t *val) {
    return i2c_read_reg(s_i2c_bus[port], s_i2c_addr[port], reg, val, 1);
}

/* ---- FUSB302B helper: read multiple registers ---- */
static int fusb_read_burst(pd_port_t port, uint8_t reg, uint8_t *buf, uint16_t len) {
    return i2c_read_reg(s_i2c_bus[port], s_i2c_addr[port], reg, buf, len);
}

/* ---- FUSB302B helper: write to FIFO ---- */
static int fusb_write_fifo(pd_port_t port, const uint8_t *data, uint16_t len) {
    return i2c_write_reg(s_i2c_bus[port], s_i2c_addr[port], FUSB302_FIFOS, data, len);
}

/* ---- FUSB302B helper: read from FIFO ---- */
static int fusb_read_fifo(pd_port_t port, uint8_t *data, uint16_t len) {
    return i2c_read_reg(s_i2c_bus[port], s_i2c_addr[port], FUSB302_FIFOS, data, len);
}

/* ---- CRC-32 for PD messages (CRC-32/ITU, polynomial 0x04C11DB7) ---- */
static uint32_t pd_crc32(const uint8_t *data, uint16_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint32_t)data[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 1u) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

/* ---- Initialize FUSB302B for a given port ---- */
int cc_phy_init(pd_port_t port) {
    uint8_t val;

    /* 1. Reset the FUSB302B */
    fusb_write(port, FUSB302_RESET, 0x01u);
    delay_ms(5);

    /* 2. Power up: enable everything */
    fusb_write(port, FUSB302_POWER, 0x0Fu); /* PWR1=011, PWR0=11 (all on) */

    /* 3. Set default role: source port = source, sink port = sink */
    cc_phy_set_role(port, (port == PD_PORT_SOURCE) ? 1 : 0);

    /* 4. Configure SWITCHES0: enable auto-CRC, CC measurement */
    val = FUSB302_SW0_AUTO_CRC;
    if (port == PD_PORT_SOURCE) {
        val |= FUSB302_SW0_PD_EN_CC1 | FUSB302_SW0_PD_EN_CC2;
    } else {
        val |= FUSB302_SW0_PU_EN_CC1 | FUSB302_SW0_PU_EN_CC2;
    }
    fusb_write(port, FUSB302_SWITCHES0, val);

    /* 5. Configure SWITCHES1: enable BMC on both CC lines (auto-detect orientation) */
    fusb_write(port, FUSB302_SWITCHES1, 0x25u); /* AUTO_CRC, TXCC1, TXCC2 */

    /* 6. Configure CONTROL0: flush FIFOs, set host current to default */
    fusb_write(port, FUSB302_CONTROL0,
               FUSB302_CTRL0_TX_FLUSH | FUSB302_CTRL0_RX_FLUSH);
    fusb_write(port, FUSB302_CONTROL0, FUSB302_CTRL0_INT_REC);

    /* 7. Configure CONTROL2: enable auto-retry, 4 retries */
    fusb_write(port, FUSB302_CONTROL2, 0x03u); /* 3 retries */

    /* 8. Configure CONTROL3: enable auto-hard-reset, disable packet generation */
    fusb_write(port, FUSB302_CONTROL3, 0x06u);

    /* 9. Clear interrupts */
    fusb_read(port, FUSB302_INTERRUPTA, &val);
    fusb_read(port, FUSB302_INTERRUPTB, &val);

    /* 10. Mask interrupts we don't care about */
    fusb_write(port, FUSB302_MASK, 0x00u);   /* Unmask all */
    fusb_write(port, FUSB302_MASKA, 0x00u);
    fusb_write(port, FUSB302_MASKB, 0x00u);

    return 0;
}

/* ---- Set the role (source or sink) ---- */
void cc_phy_set_role(pd_port_t port, uint8_t is_source) {
    uint8_t sw0;
    fusb_read(port, FUSB302_SWITCHES0, &sw0);

    if (is_source) {
        /* Source: pull-down on CC (advertise Rd) */
        sw0 &= ~(FUSB302_SW0_PU_EN_CC1 | FUSB302_SW0_PU_EN_CC2);
        sw0 |= FUSB302_SW0_PD_EN_CC1 | FUSB302_SW0_PD_EN_CC2;
    } else {
        /* Sink: pull-up on CC (advertise Rp) */
        sw0 &= ~(FUSB302_SW0_PD_EN_CC1 | FUSB302_SW0_PD_EN_CC2);
        sw0 |= FUSB302_SW0_PU_EN_CC1 | FUSB302_SW0_PU_EN_CC2;
    }
    sw0 |= FUSB302_SW0_AUTO_CRC;
    fusb_write(port, FUSB302_SWITCHES0, sw0);
}

/* ---- Get CC line status (which CC is active, voltage level) ---- */
uint8_t cc_phy_get_cc_status(pd_port_t port) {
    uint8_t status0;
    fusb_read(port, FUSB302_STATUS0, &status0);

    /* BC_LVL bits [1:0] indicate CC voltage level:
     * 00 = < 0.2V (nothing attached)
     * 01 = 0.2-0.66V (default USB current)
     * 10 = 0.66-1.25V (1.5A)
     * 11 = > 1.25V (3A)
     */
    return status0 & 0x03u;
}

/* ---- Flush RX FIFO ---- */
void cc_phy_flush_rx(pd_port_t port) {
    fusb_write(port, FUSB302_CONTROL0,
               FUSB302_CTRL0_RX_FLUSH | FUSB302_CTRL0_INT_REC);
}

/* ---- Send a PD message via the FUSB302B ---- */
int cc_phy_send_msg(pd_port_t port, const pd_msg_t *msg) {
    uint8_t tx_buf[40]; /* Header(2) + data(28) + CRC(4) = 34 max */
    uint16_t total_len;

    /* Pack header (2 bytes, little-endian) */
    tx_buf[0] = (uint8_t)(msg->header & 0xFFu);
    tx_buf[1] = (uint8_t)((msg->header >> 8) & 0xFFu);
    total_len = 2;

    /* Pack data objects (4 bytes each, little-endian) */
    for (uint8_t i = 0; i < msg->num_objs; i++) {
        tx_buf[total_len++] = (uint8_t)(msg->data_obj[i] & 0xFFu);
        tx_buf[total_len++] = (uint8_t)((msg->data_obj[i] >> 8) & 0xFFu);
        tx_buf[total_len++] = (uint8_t)((msg->data_obj[i] >> 16) & 0xFFu);
        tx_buf[total_len++] = (uint8_t)((msg->data_obj[i] >> 24) & 0xFFu);
    }

    /* Compute and append CRC-32 */
    uint32_t crc = pd_crc32(tx_buf, total_len);
    tx_buf[total_len++] = (uint8_t)(crc & 0xFFu);
    tx_buf[total_len++] = (uint8_t)((crc >> 8) & 0xFFu);
    tx_buf[total_len++] = (uint8_t)((crc >> 16) & 0xFFu);
    tx_buf[total_len++] = (uint8_t)((crc >> 24) & 0xFFu);

    /* Write to FUSB302B TX FIFO:
     * First byte: SOP* token (0x12 = SOP, 0x13 = SOP', 0x14 = SOP")
     * Then the message bytes
     */
    uint8_t fifo_buf[41];
    fifo_buf[0] = 0x12u; /* SOP token */
    memcpy(&fifo_buf[1], tx_buf, total_len);
    fusb_write_fifo(port, fifo_buf, total_len + 1);

    /* Trigger TX via CONTROL0 */
    fusb_write(port, FUSB302_CONTROL0, 0x44u); /* TX_START | INT_REC */

    /* Wait for TX complete (poll STATUS0 bit 0 = TXBUSY) */
    uint32_t timeout = millis() + 50;
    uint8_t status0;
    do {
        fusb_read(port, FUSB302_STATUS0, &status0);
        if (millis() > timeout) return -1;
    } while (status0 & 0x01u); /* TXBUSY */

    return 0;
}

/* ---- Receive a PD message from the FUSB302B ---- */
int cc_phy_recv_msg(pd_port_t port, pd_msg_t *msg, uint32_t timeout_ms) {
    uint8_t status1;
    uint32_t deadline = millis() + timeout_ms;

    /* Wait for RX message (STATUS1 bit 2 = RX_EMPTY, 0 = data available) */
    do {
        fusb_read(port, FUSB302_STATUS1, &status1);
        if (status1 & 0x04u) {
            /* RX empty */
            if (millis() > deadline) return -1;
            continue;
        }
        break;
    } while (1);

    /* Read the RX FIFO: first byte is SOP* token, then header, data, CRC */
    uint8_t rx_buf[40];
    fusb_read_fifo(port, rx_buf, 1); /* Read SOP token */

    /* Read header (2 bytes) */
    fusb_read_fifo(port, &rx_buf[0], 2);
    msg->header = (uint16_t)rx_buf[0] | ((uint16_t)rx_buf[1] << 8);
    msg->num_objs = (uint8_t)((msg->header >> PD_HDR_NUM_OBJ_SHIFT) & 0x07u);

    /* Read data objects + CRC (4 bytes each + 4 bytes CRC) */
    uint16_t payload_len = msg->num_objs * 4u + 4u; /* data + CRC */
    if (msg->num_objs > PD_MAX_DATA_OBJS) return -1;

    fusb_read_fifo(port, &rx_buf[2], payload_len);

    /* Extract data objects (skip CRC at the end) */
    for (uint8_t i = 0; i < msg->num_objs; i++) {
        uint16_t off = 2 + i * 4u;
        msg->data_obj[i] = (uint32_t)rx_buf[off]
                         | ((uint32_t)rx_buf[off + 1] << 8)
                         | ((uint32_t)rx_buf[off + 2] << 16)
                         | ((uint32_t)rx_buf[off + 3] << 24);
    }

    /* Verify CRC (over header + data, excluding the CRC itself) */
    uint16_t crc_len = 2u + msg->num_objs * 4u;
    uint32_t computed_crc = pd_crc32(rx_buf, crc_len);
    uint32_t received_crc = (uint32_t)rx_buf[crc_len]
                          | ((uint32_t)rx_buf[crc_len + 1] << 8)
                          | ((uint32_t)rx_buf[crc_len + 2] << 16)
                          | ((uint32_t)rx_buf[crc_len + 3] << 24);

    if (computed_crc != received_crc) {
        return -2; /* CRC mismatch */
    }

    /* Flush RX FIFO to prepare for next message */
    cc_phy_flush_rx(port);

    return 0;
}

/* ---- Build a PD header ---- */
static uint16_t build_pd_header(uint8_t msg_type, uint8_t num_objs,
                                uint8_t port_role, uint8_t data_role,
                                uint8_t msg_id, uint8_t rev) {
    uint16_t hdr = 0;
    hdr |= (uint16_t)(msg_type & 0x1Fu);
    hdr |= (uint16_t)(port_role & 1u) << PD_HDR_PORT_ROLE_SHIFT;
    hdr |= (uint16_t)(data_role & 1u) << PD_HDR_DATA_ROLE_SHIFT;
    hdr |= (uint16_t)(msg_id & 0x07u) << PD_HDR_MSG_ID_SHIFT;
    hdr |= (uint16_t)(num_objs & 0x07u) << PD_HDR_NUM_OBJ_SHIFT;
    hdr |= (uint16_t)(rev & 0x03u) << PD_HDR_REV_SHIFT;
    return hdr;
}

/* ---- Send a control message ---- */
static int send_control_msg(pd_port_t port, pd_ctrl_msg_t ctrl,
                            uint8_t port_role, uint8_t data_role,
                            uint8_t msg_id) {
    pd_msg_t msg;
    msg.header = build_pd_header((uint8_t)ctrl, 0, port_role, data_role, msg_id, 2u);
    msg.num_objs = 0;
    return cc_phy_send_msg(port, &msg);
}

/* ---- Convenience: send GoodCRC ---- */
int cc_phy_send_goodcrc(pd_port_t port, uint8_t msg_id,
                        uint8_t port_role, uint8_t data_role) {
    return send_control_msg(port, PD_CTRL_GOODCRC, port_role, data_role, msg_id);
}