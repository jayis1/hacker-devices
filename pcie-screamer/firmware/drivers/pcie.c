/**
 * drivers/pcie.c — PCIe Hard IP Driver Implementation
 *
 * Complete driver for the Lattice ECP5 PCIe Hard IP. Manages
 * initialization, link training, TLP reception, and TLP injection.
 *
 * The ECP5 PCIe Hard IP provides:
 *   - x1/x2/x4 lane support at Gen1 (2.5 GT/s) or Gen2 (5.0 GT/s)
 *   - PIPE interface to the SERDES PMA
 *   - Data Link Layer (DLLP) handling
 *   - Transaction Layer (TLP) RX/TX FIFOs
 *   - Configuration space (Type 0)
 *   - MSI/MSI-X interrupt generation
 *
 * Author: Nous Research
 * Date: 2026-06-16
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "../board.h"
#include "../registers.h"
#include "pcie.h"

/*============================================================================
 * INTERNAL CONSTANTS
 *============================================================================*/

#define PCIE_LINK_TIMEOUT_US     100000UL  /* 100 ms for link training */
#define PCIE_PIPE_TIMEOUT_US     10000UL   /* 10 ms for PIPE ready */
#define PCIE_TX_FIFO_TIMEOUT_US  1000UL    /* 1 ms for TX FIFO space */
#define PCIE_RX_FIFO_TIMEOUT_US  1000UL    /* 1 ms for RX FIFO data */

/* PCIe Maximum Payload Size encodings */
#define PCIE_MPS_128              0x0
#define PCIE_MPS_256              0x1
#define PCIE_MPS_512              0x2
#define PCIE_MPS_1024             0x3

/* PCIe RX Equalization modes */
#define PCIE_RX_EQ_OFF            0x0
#define PCIE_RX_EQ_ADAPTIVE       0x1
#define PCIE_RX_EQ_MANUAL         0x2

/* PCIe TX De-emphasis values (in dB) */
#define PCIE_TX_DEEMPH_0DB        0x0
#define PCIE_TX_DEEMPH_MINUS3_5DB 0x1
#define PCIE_TX_DEEMPH_MINUS6DB   0x2

/*============================================================================
 * PCIE INITIALIZATION
 *============================================================================*/

int pcie_init(void) {
    uint32_t reg;
    int ret;

    /* 1. Assert PCIe Hard IP reset */
    REG_WRITE(PCIE_CTRL_REG, PCIE_CTRL_RESET);
    delay_us(10);

    /* 2. Configure link parameters */
    REG_WRITE(PCIE_LINK_WIDTH_CFG_REG, PCIE_WIDTH_X4);
    REG_WRITE(PCIE_MAX_SPEED_REG, PCIE_SPEED_GEN2);
    REG_WRITE(PCIE_MAX_PAYLOAD_REG, PCIE_MPS_256);

    /* 3. Configure vendor/device IDs for interposer's own config space */
    REG_WRITE(PCIE_VENDOR_ID_REG, 0x1D6B);  /* Default vendor ID */
    REG_WRITE(PCIE_DEVICE_ID_REG, 0x0001);  /* Default device ID */

    /* 4. Configure BAR0 — 4 KB MMIO space for interposer registers */
    REG_WRITE(PCIE_BAR0_REG, 0x00000000);       /* BAR0 base (programmable) */
    REG_WRITE(PCIE_BAR0_MASK_REG, 0xFFFFF000);  /* 4 KB aperture */

    /* 5. Configure RX equalization */
    REG_WRITE(PCIE_RX_EQ_REG, PCIE_RX_EQ_ADAPTIVE);

    /* 6. Configure TX de-emphasis */
    REG_WRITE(PCIE_TX_DEEMPH_REG, PCIE_TX_DEEMPH_MINUS3_5DB);

    /* 7. Release PCIe Hard IP reset */
    REG_WRITE(PCIE_CTRL_REG, 0);

    /* 8. Wait for PIPE interface ready */
    ret = REG_POLL_UNTIL(PCIE_STATUS_REG, PCIE_STATUS_PIPE_RDY,
                         PCIE_STATUS_PIPE_RDY, PCIE_PIPE_TIMEOUT_US);
    if (ret != 0) {
        return -1;  /* PIPE not ready */
    }

    /* 9. Release PERST# to downstream NVMe device */
    DEASSERT_LOW(PCIE_PERST_PIN);
    delay_us(100);  /* Tperst-clk = 100 µs minimum */

    /* 10. Enable link training */
    REG_SET_BITS(PCIE_CTRL_REG, PCIE_CTRL_LINK_TRAIN_EN);

    /* 11. Wait for link to reach L0 */
    ret = REG_POLL_UNTIL(PCIE_LTSSM_STATE_REG, 0x3F,
                         LTSSM_L0, PCIE_LINK_TIMEOUT_US);
    if (ret != 0) {
        /* Check if we reached any active state */
        reg = REG_READ(PCIE_LTSSM_STATE_REG) & 0x3F;
        if (reg == LTSSM_DETECT_QUIET) {
            /* No device detected — try Gen1 fallback */
            REG_WRITE(PCIE_MAX_SPEED_REG, PCIE_SPEED_GEN1);
            REG_SET_BITS(PCIE_CTRL_REG, PCIE_CTRL_LINK_TRAIN_EN);
            ret = REG_POLL_UNTIL(PCIE_LTSSM_STATE_REG, 0x3F,
                                 LTSSM_L0, PCIE_LINK_TIMEOUT_US);
            if (ret != 0) {
                return -3;  /* Link failed even at Gen1 */
            }
        } else {
            return -2;  /* Link training timeout */
        }
    }

    /* 12. Read back negotiated parameters */
    reg = REG_READ(PCIE_LINK_SPEED_REG);
    uint8_t speed = reg & 0xFF;
    reg = REG_READ(PCIE_LINK_WIDTH_REG);
    uint8_t width = reg & 0xFF;

    /* 13. Enable TLP reception and transmission */
    REG_WRITE(PCIE_RX_TLP_ENABLE_REG, 1);
    REG_WRITE(PCIE_TX_TLP_ENABLE_REG, 1);

    /* 14. Verify TLP interfaces are ready */
    ret = REG_POLL_UNTIL(PCIE_STATUS_REG,
                         PCIE_STATUS_TLP_RX_RDY | PCIE_STATUS_TLP_TX_RDY,
                         PCIE_STATUS_TLP_RX_RDY | PCIE_STATUS_TLP_TX_RDY,
                         1000);
    if (ret != 0) {
        return -1;
    }

    return 0;
}

/*============================================================================
 * LINK STATUS
 *============================================================================*/

void pcie_get_link_status(uint8_t *ltssm_state, uint8_t *link_speed,
                          uint8_t *link_width) {
    if (ltssm_state) {
        *ltssm_state = REG_READ(PCIE_LTSSM_STATE_REG) & 0x3F;
    }
    if (link_speed) {
        *link_speed = REG_READ(PCIE_LINK_SPEED_REG) & 0xFF;
    }
    if (link_width) {
        *link_width = REG_READ(PCIE_LINK_WIDTH_REG) & 0xFF;
    }
}

/*============================================================================
 * TLP RECEPTION
 *============================================================================*/

int pcie_rx_tlp_read(uint8_t *buf, uint16_t max_len) {
    uint32_t header[4];
    uint16_t length_dw;
    uint16_t total_bytes;
    uint8_t fmt, type;
    uint8_t header_dw;

    /* Check if TLP available in RX FIFO */
    uint32_t fifo_status = REG_READ(PCIE_RX_FIFO_STATUS_REG);
    if (!(fifo_status & PCIE_RX_FIFO_NOT_EMPTY)) {
        return 0;  /* No TLP available */
    }

    /* Check for overflow */
    if (fifo_status & PCIE_RX_FIFO_OVERFLOW) {
        /* Clear overflow by reading all available data */
        while (REG_READ(PCIE_RX_FIFO_STATUS_REG) & PCIE_RX_FIFO_NOT_EMPTY) {
            REG_READ(PCIE_RX_FIFO_DATA_REG);
        }
        return -1;  /* Overflow occurred, data lost */
    }

    /* Read first header DWORD to determine TLP size */
    header[0] = REG_READ(PCIE_RX_FIFO_DATA_REG);

    /* Parse Fmt and Type */
    fmt  = (header[0] >> 29) & 0x3;
    type = (header[0] >> 24) & 0x1F;
    length_dw = header[0] & 0x3FF;  /* 10-bit length field */

    /* Determine header size */
    header_dw = (fmt == PCIE_FMT_4DW_NO_DATA || fmt == PCIE_FMT_4DW_WITH_DATA) ? 4 : 3;

    /* Calculate total TLP size (header + data + 1 DW LCRC) */
    total_bytes = (header_dw + length_dw + 1) * 4;

    if (total_bytes > max_len) {
        /* Drain the TLP from FIFO to prevent stall */
        for (int i = 1; i < header_dw + length_dw + 1; i++) {
            REG_READ(PCIE_RX_FIFO_DATA_REG);
        }
        return -2;  /* Buffer too small */
    }

    /* Read remaining header DWORDs */
    for (int i = 1; i < header_dw; i++) {
        header[i] = REG_READ(PCIE_RX_FIFO_DATA_REG);
    }

    /* Copy header to output buffer */
    memcpy(buf, header, header_dw * 4);

    /* Read data payload (if any) */
    for (int i = 0; i < length_dw; i++) {
        uint32_t dw = REG_READ(PCIE_RX_FIFO_DATA_REG);
        memcpy(buf + (header_dw + i) * 4, &dw, 4);
    }

    /* Read LCRC (1 DW) */
    uint32_t lcrc = REG_READ(PCIE_RX_FIFO_DATA_REG);
    memcpy(buf + (header_dw + length_dw) * 4, &lcrc, 4);

    return total_bytes;
}

/*============================================================================
 * TLP INJECTION
 *============================================================================*/

int pcie_tx_tlp_inject(uint8_t tlp_type, uint16_t requester_id,
                       uint8_t tag, uint64_t address,
                       const uint8_t *data, uint16_t length_dw) {
    uint32_t header[4];
    uint8_t fmt, type;
    int header_dw;
    int ret;

    /* Map TLP type to Fmt and Type fields */
    switch (tlp_type) {
    case PCIE_TYPE_MRD:  /* Memory Read Request */
        if (address > 0xFFFFFFFFULL) {
            fmt = PCIE_FMT_4DW_NO_DATA;
        } else {
            fmt = PCIE_FMT_3DW_NO_DATA;
        }
        type = PCIE_TYPE_MRD;
        break;

    case PCIE_TYPE_MWR:  /* Memory Write Request */
        if (address > 0xFFFFFFFFULL) {
            fmt = PCIE_FMT_4DW_WITH_DATA;
        } else {
            fmt = PCIE_FMT_3DW_WITH_DATA;
        }
        type = PCIE_TYPE_MWR;
        break;

    case PCIE_TYPE_CFGRD0:  /* Configuration Read Type 0 */
        fmt = PCIE_FMT_3DW_NO_DATA;
        type = PCIE_TYPE_CFGRD0;
        break;

    case PCIE_TYPE_CFGWR0:  /* Configuration Write Type 0 */
        fmt = PCIE_FMT_3DW_WITH_DATA;
        type = PCIE_TYPE_CFGWR0;
        break;

    case PCIE_TYPE_CFGRD1:  /* Configuration Read Type 1 */
        fmt = PCIE_FMT_3DW_NO_DATA;
        type = PCIE_TYPE_CFGRD1;
        break;

    case PCIE_TYPE_CFGWR1:  /* Configuration Write Type 1 */
        fmt = PCIE_FMT_3DW_WITH_DATA;
        type = PCIE_TYPE_CFGWR1;
        break;

    case PCIE_TYPE_MSG:  /* Message (no data) */
        fmt = PCIE_FMT_3DW_NO_DATA;
        type = PCIE_TYPE_MSG;
        break;

    case PCIE_TYPE_MSGD:  /* Message with Data */
        fmt = PCIE_FMT_3DW_WITH_DATA;
        type = PCIE_TYPE_MSGD;
        break;

    default:
        return -1;  /* Unsupported TLP type */
    }

    header_dw = (fmt == PCIE_FMT_4DW_NO_DATA || fmt == PCIE_FMT_4DW_WITH_DATA) ? 4 : 3;

    /* Build header DW0: Fmt, Type, TC=0, Attr=0, TH=0, TD=0, EP=0, Attr2=0, Length */
    header[0] = ((uint32_t)fmt << 29) |
                ((uint32_t)type << 24) |
                (0 << 22) |  /* TC = 0 */
                (0 << 18) |  /* Attr = 0 */
                (0 << 16) |  /* TH = 0 */
                (0 << 15) |  /* TD = 0 (no TLP digest) */
                (0 << 14) |  /* EP = 0 */
                (0 << 12) |  /* Attr2 = 0 */
                (length_dw & 0x3FF);

    /* Build header DW1: Requester ID, Tag, Byte Enables */
    header[1] = ((uint32_t)requester_id << 16) |
                ((uint32_t)tag << 8) |
                0x0F;  /* First DW BE = all bytes enabled, Last DW BE = all bytes */

    /* Build header DW2: Address[31:2] */
    if (fmt == PCIE_FMT_4DW_NO_DATA || fmt == PCIE_FMT_4DW_WITH_DATA) {
        header[2] = (uint32_t)(address & 0xFFFFFFFF);
        header[3] = (uint32_t)((address >> 32) & 0xFFFFFFFF);
    } else {
        header[2] = (uint32_t)(address & 0xFFFFFFFC);  /* Address[31:2] */
    }

    /* Wait for TX FIFO space */
    ret = REG_POLL_UNTIL(PCIE_TX_FIFO_STATUS_REG, PCIE_TX_FIFO_NOT_FULL,
                         PCIE_TX_FIFO_NOT_FULL, PCIE_TX_FIFO_TIMEOUT_US);
    if (ret != 0) {
        return -2;  /* TX FIFO full timeout */
    }

    /* Write header to TX FIFO */
    for (int i = 0; i < header_dw; i++) {
        REG_WRITE(PCIE_TX_FIFO_DATA_REG, header[i]);
    }

    /* Write data payload (if any) */
    if (fmt == PCIE_FMT_3DW_WITH_DATA || fmt == PCIE_FMT_4DW_WITH_DATA) {
        if (data == NULL && length_dw > 0) {
            return -3;  /* Data required but not provided */
        }

        for (uint16_t i = 0; i < length_dw; i++) {
            /* Wait for FIFO space before each DWORD */
            ret = REG_POLL_UNTIL(PCIE_TX_FIFO_STATUS_REG, PCIE_TX_FIFO_NOT_FULL,
                                 PCIE_TX_FIFO_NOT_FULL, PCIE_TX_FIFO_TIMEOUT_US);
            if (ret != 0) {
                return -2;
            }

            uint32_t dw;
            if (data) {
                memcpy(&dw, data + i * 4, 4);
            } else {
                dw = 0;
            }
            REG_WRITE(PCIE_TX_FIFO_DATA_REG, dw);
        }
    }

    /* LCRC is automatically appended by the PCIe Hard IP */

    return 0;
}

/*============================================================================
 * TLP HEADER PARSING
 *============================================================================*/

int pcie_tlp_parse_header(const uint8_t *raw_header,
                          tlp_record_header_t *record) {
    uint32_t dw0, dw1, dw2, dw3;
    uint8_t fmt, type;
    int header_dw;

    if (!raw_header || !record) {
        return -1;
    }

    /* Read header DWORDs */
    memcpy(&dw0, raw_header + 0, 4);
    memcpy(&dw1, raw_header + 4, 4);
    memcpy(&dw2, raw_header + 8, 4);

    /* Parse DW0 */
    fmt  = (dw0 >> 29) & 0x3;
    type = (dw0 >> 24) & 0x1F;

    record->tlp_type      = (fmt << 5) | type;
    record->tlp_length    = dw0 & 0x3FF;  /* Length in DW */
    record->traffic_class = (dw0 >> 22) & 0x7;
    record->attributes    = (dw0 >> 18) & 0x3F;

    /* Parse DW1 */
    record->requester_id  = (dw1 >> 16) & 0xFFFF;
    record->tag           = (dw1 >> 8) & 0xFF;
    record->byte_enables  = dw1 & 0xFF;

    /* Determine header size */
    header_dw = (fmt == PCIE_FMT_4DW_NO_DATA || fmt == PCIE_FMT_4DW_WITH_DATA) ? 4 : 3;

    /* Parse address */
    if (header_dw == 4) {
        memcpy(&dw3, raw_header + 12, 4);
        record->address = ((uint64_t)dw3 << 32) | (dw2 & 0xFFFFFFFC);
    } else {
        record->address = dw2 & 0xFFFFFFFC;
    }

    /* Convert length from DW to bytes */
    record->tlp_length = (header_dw + record->tlp_length + 1) * 4;

    return header_dw;
}

/*============================================================================
 * CONFIGURATION SPACE ACCESS
 *============================================================================*/

int pcie_config_read(uint16_t offset, uint32_t *value) {
    if (offset > 0xFF || (offset & 0x3)) {
        return -1;  /* Invalid offset or unaligned */
    }

    /* The ECP5 PCIe Hard IP maps config space to a register block.
     * Offset 0x00-0x3F are the Type 0 header registers. */
    uint32_t config_base = PCIE_BASE + 0x0100;  /* Config space base */
    *value = REG_READ(config_base + offset);

    return 0;
}

int pcie_config_write(uint16_t offset, uint32_t value) {
    if (offset > 0xFF || (offset & 0x3)) {
        return -1;
    }

    uint32_t config_base = PCIE_BASE + 0x0100;
    REG_WRITE(config_base + offset, value);

    return 0;
}

void pcie_set_vendor_id(uint16_t vendor_id, uint16_t device_id) {
    REG_WRITE(PCIE_VENDOR_ID_REG, vendor_id);
    REG_WRITE(PCIE_DEVICE_ID_REG, device_id);
}

/*============================================================================
 * LCRC CALCULATION (for verification, not required for injection)
 *============================================================================*/

/**
 * pcie_lcrc_calculate — Calculate PCIe LCRC over TLP bytes
 *
 * PCIe uses the same CRC-32 polynomial as Ethernet (0x04C11DB7).
 * The LCRC covers the TLP header and data payload, but NOT the
 * framing symbols (STP, SDP, END) or the LCRC itself.
 *
 * This is a software implementation for verification purposes.
 * The hardware automatically appends LCRC on TX and checks on RX.
 *
 * @param data       TLP header + data bytes
 * @param len_bytes  Number of bytes
 * @return           32-bit LCRC value
 */
uint32_t pcie_lcrc_calculate(const uint8_t *data, uint16_t len_bytes) {
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t polynomial = 0x04C11DB7;

    for (uint16_t i = 0; i < len_bytes; i++) {
        crc ^= (uint32_t)data[i] << 24;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc = (crc << 1);
            }
        }
    }

    return ~crc;  /* Final XOR */
}
