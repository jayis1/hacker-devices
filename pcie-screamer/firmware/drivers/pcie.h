/**
 * drivers/pcie.h — PCIe Hard IP Driver Interface
 *
 * PCIe Screamer — NVMe Interposer
 * Author: Nous Research
 * Date: 2026-06-16
 */

#ifndef PCIE_H
#define PCIE_H

#include <stdint.h>
#include <stddef.h>

/*============================================================================
 * TLP TYPE CONSTANTS
 *============================================================================*/

/* TLP Format + Type encodings (Fmt[1:0] << 5 | Type[4:0]) */
#define PCIE_FMT_3DW_NO_DATA    0x00
#define PCIE_FMT_4DW_NO_DATA    0x01
#define PCIE_FMT_3DW_WITH_DATA  0x02
#define PCIE_FMT_4DW_WITH_DATA  0x03

#define PCIE_TYPE_MRD            0x00  /* Memory Read Request */
#define PCIE_TYPE_MRD_LOCK       0x01  /* Memory Read Request - Locked */
#define PCIE_TYPE_MWR            0x02  /* Memory Write Request */
#define PCIE_TYPE_IORD           0x03  /* I/O Read Request */
#define PCIE_TYPE_IOWR           0x04  /* I/O Write Request */
#define PCIE_TYPE_CFGRD0         0x05  /* Configuration Read Type 0 */
#define PCIE_TYPE_CFGWR0         0x06  /* Configuration Write Type 0 */
#define PCIE_TYPE_CFGRD1         0x07  /* Configuration Read Type 1 */
#define PCIE_TYPE_CFGWR1         0x08  /* Configuration Write Type 1 */
#define PCIE_TYPE_MSG            0x10  /* Message Request */
#define PCIE_TYPE_MSGD           0x11  /* Message Request with Data */
#define PCIE_TYPE_CPL            0x12  /* Completion */
#define PCIE_TYPE_CPLD           0x13  /* Completion with Data */
#define PCIE_TYPE_CPLLOCK        0x14  /* Completion for Locked Read */
#define PCIE_TYPE_CPLDLOCK       0x15  /* Completion with Data for Locked Read */
#define PCIE_TYPE_FETCHADD       0x16  /* Fetch and Add AtomicOp */
#define PCIE_TYPE_SWAP           0x17  /* Unconditional Swap AtomicOp */
#define PCIE_TYPE_CAS            0x18  /* Compare and Swap AtomicOp */

/*============================================================================
 * TLP HEADER STRUCTURES
 *============================================================================*/

/* 3-DW TLP Header (12 bytes) */
typedef struct __attribute__((packed)) {
    uint32_t dw0;  /* Fmt[1:0], Type[4:0], TC[2:0], Attr[2:0],
                      TH, TD, EP, Attr2, Length[9:0] */
    uint32_t dw1;  /* Requester ID[15:0], Tag[7:0],
                      Last DW BE[3:0], First DW BE[3:0] */
    uint32_t dw2;  /* Address[31:2] (32-bit address) or
                      Address[63:32] (64-bit address upper) */
} tlp_header_3dw_t;

/* 4-DW TLP Header (16 bytes) */
typedef struct __attribute__((packed)) {
    uint32_t dw0;  /* Same as 3-DW */
    uint32_t dw1;  /* Same as 3-DW */
    uint32_t dw2;  /* Address[31:2] (lower 32 bits of 64-bit address) */
    uint32_t dw3;  /* Address[63:32] (upper 32 bits) */
} tlp_header_4dw_t;

/*============================================================================
 * TLP RECORD STRUCTURE (for capture)
 *============================================================================*/

typedef struct __attribute__((packed)) {
    uint64_t timestamp;     /* 48-bit timestamp, 8 ns resolution */
    uint8_t  direction;     /* 0 = Host→Device, 1 = Device→Host */
    uint8_t  tlp_type;      /* Fmt[1:0] | Type[4:0] */
    uint16_t tlp_length;    /* Total TLP length in bytes */
    uint16_t requester_id;  /* Bus[7:0], Device[4:0], Function[2:0] */
    uint8_t  tag;           /* Transaction tag */
    uint8_t  traffic_class; /* TC[2:0] */
    uint8_t  attributes;    /* Attr[1:0], TH, TD, EP, Attr2 */
    uint8_t  byte_enables;  /* First DW BE[3:0], Last DW BE[3:0] */
    uint64_t address;       /* Target address (for memory/IO/CFG TLPs) */
    /* Raw TLP bytes follow this header */
} tlp_record_header_t;

/*============================================================================
 * PCIe CONFIGURATION SPACE (Type 0, for interposer's own function)
 *============================================================================*/

typedef struct __attribute__((packed)) {
    uint16_t vendor_id;         /* 0x00 */
    uint16_t device_id;         /* 0x02 */
    uint16_t command;           /* 0x04 */
    uint16_t status;            /* 0x06 */
    uint8_t  revision_id;       /* 0x08 */
    uint8_t  prog_if;           /* 0x09 */
    uint8_t  subclass;          /* 0x0A */
    uint8_t  class_code;        /* 0x0B */
    uint8_t  cache_line_size;   /* 0x0C */
    uint8_t  latency_timer;     /* 0x0D */
    uint8_t  header_type;       /* 0x0E */
    uint8_t  bist;              /* 0x0F */
    uint32_t bar0;              /* 0x10 */
    uint32_t bar1;              /* 0x14 */
    uint32_t bar2;              /* 0x18 */
    uint32_t bar3;              /* 0x1C */
    uint32_t bar4;              /* 0x20 */
    uint32_t bar5;              /* 0x24 */
    uint32_t cardbus_cis_ptr;   /* 0x28 */
    uint16_t subsystem_vendor;  /* 0x2C */
    uint16_t subsystem_id;      /* 0x2E */
    uint32_t expansion_rom;     /* 0x30 */
    uint8_t  capabilities_ptr;  /* 0x34 */
    uint8_t  reserved1[3];      /* 0x35 */
    uint32_t reserved2;         /* 0x38 */
    uint8_t  interrupt_line;    /* 0x3C */
    uint8_t  interrupt_pin;     /* 0x3D */
    uint8_t  min_grant;         /* 0x3E */
    uint8_t  max_latency;       /* 0x3F */
} pcie_config_space_t;

/*============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

/**
 * pcie_init — Initialize PCIe Hard IP and train link
 *
 * Configures the ECP5 PCIe Hard IP for x4 Gen2 operation,
 * releases PERST# to the downstream NVMe device, and waits
 * for link training to complete (LTSSM L0 state).
 *
 * Returns: 0 on success
 *          -1 if PIPE interface not ready
 *          -2 if link training timeout
 *          -3 if link fails to reach L0
 */
int pcie_init(void);

/**
 * pcie_get_link_status — Get current PCIe link status
 *
 * Returns LTSSM state, link speed, and link width through
 * output parameters.
 */
void pcie_get_link_status(uint8_t *ltssm_state, uint8_t *link_speed,
                          uint8_t *link_width);

/**
 * pcie_rx_tlp_read — Read a received TLP from the RX FIFO
 *
 * Reads one complete TLP (header + data + LCRC) from the
 * PCIe Hard IP receive FIFO.
 *
 * @param buf      Output buffer (must be at least 4096 bytes)
 * @param max_len  Maximum bytes to read
 * @return         Number of bytes read, 0 if no TLP available,
 *                 negative on error
 */
int pcie_rx_tlp_read(uint8_t *buf, uint16_t max_len);

/**
 * pcie_tx_tlp_inject — Inject a TLP onto the PCIe link
 *
 * Builds and transmits a TLP through the PCIe Hard IP TX FIFO.
 * The LCRC is automatically appended by the hardware.
 *
 * @param tlp_type      TLP type (MRd=0, MWr=1, CfgRd0=5, etc.)
 * @param requester_id  16-bit requester ID
 * @param tag           8-bit transaction tag
 * @param address       64-bit target address
 * @param data          Payload data (NULL for reads)
 * @param length_dw     Data length in DWORDs (0 for reads)
 * @return              0 on success, negative on error
 */
int pcie_tx_tlp_inject(uint8_t tlp_type, uint16_t requester_id,
                       uint8_t tag, uint64_t address,
                       const uint8_t *data, uint16_t length_dw);

/**
 * pcie_tlp_parse_header — Parse a raw TLP header into fields
 *
 * Extracts Fmt, Type, Length, Requester ID, Tag, Address, etc.
 * from a raw TLP header.
 *
 * @param raw_header  Pointer to raw TLP header bytes (12 or 16)
 * @param record      Output record header structure to fill
 * @return            Number of header DWORDs (3 or 4)
 */
int pcie_tlp_parse_header(const uint8_t *raw_header,
                          tlp_record_header_t *record);

/**
 * pcie_config_read — Read from interposer's own PCIe config space
 *
 * @param offset  Byte offset in config space (0-255)
 * @param value   Output 32-bit value
 * @return        0 on success
 */
int pcie_config_read(uint16_t offset, uint32_t *value);

/**
 * pcie_config_write — Write to interposer's own PCIe config space
 *
 * @param offset  Byte offset in config space (0-255)
 * @param value   32-bit value to write
 * @return        0 on success
 */
int pcie_config_write(uint16_t offset, uint32_t value);

/**
 * pcie_set_vendor_id — Set the interposer's PCIe vendor/device ID
 *
 * These IDs appear in the interposer's own Type 0 config space
 * if it exposes a function to the host (optional).
 */
void pcie_set_vendor_id(uint16_t vendor_id, uint16_t device_id);

#endif /* PCIE_H */
