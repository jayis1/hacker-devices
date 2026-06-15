/*
 * xio2001.h — XIO2001 PCIe-to-PCI Bridge driver
 * USB DMA Phantom firmware
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 */

#ifndef XIO2001_H
#define XIO2001_H

#include <stdint.h>
#include "board.h"

/* ---- XIO2001 SPI register offsets ---- */
#define XIO_REG_PCI_VID       0x00   /* Vendor ID */
#define XIO_REG_PCI_DID       0x02   /* Device ID */
#define XIO_REG_PCI_CMD       0x04   /* PCI command register */
#define XIO_REG_PCI_STS       0x06   /* PCI status register */
#define XIO_REG_PCI_RID       0x08   /* Revision ID */
#define XIO_REG_PCI_CC        0x0A   /* Class code */
#define XIO_REG_BAR0          0x10   /* Base address register 0 */
#define XIO_REG_BAR1          0x14   /* Base address register 1 */
#define XIO_REG_PRI_BUS      0x18   /* Primary bus number */
#define XIO_REG_SEC_BUS      0x19   /* Secondary bus number */
#define XIO_REG_SUB_BUS      0x1A   /* Subordinate bus number */
#define XIO_REG_IO_BASE      0x1C   /* I/O base */
#define XIO_REG_IO_LIMIT      0x1D   /* I/O limit */
#define XIO_REG_MEM_BASE      0x20   /* Memory base */
#define XIO_REG_MEM_LIMIT     0x22   /* Memory limit */
#define XIO_REG_ROM_ADDR      0x30   /* Expansion ROM address */
#define XIO_REG_INT_LINE      0x3C   /* Interrupt line */
#define XIO_REG_INT_PIN       0x3D   /* Interrupt pin */
#define XIO_REG_BRIDGE_CTRL   0x40   /* Bridge control register */
#define XIO_REG_DMA_CTRL      0x80   /* DMA control register */
#define XIO_REG_DMA_SRC_LO    0x84   /* DMA source address low */
#define XIO_REG_DMA_SRC_HI    0x88   /* DMA source address high */
#define XIO_REG_DMA_DST_LO    0x8C   /* DMA destination address low */
#define XIO_REG_DMA_DST_HI    0x90   /* DMA destination address high */
#define XIO_REG_DMA_XFER_LEN  0x94   /* DMA transfer length */
#define XIO_REG_DMA_STS       0x98   /* DMA status register */
#define XIO_REG_SPI_CMD       0x100  /* SPI command interface */
#define XIO_REG_SPI_DATA      0x104  /* SPI data interface */
#define XIO_REG_SPI_CFG       0x108  /* SPI configuration */

/* ---- DMA command codes ---- */
#define DMA_CMD_READ          0x01   /* Read from host physical memory */
#define DMA_CMD_WRITE         0x02   /* Write to host physical memory */
#define DMA_CMD_SCAN          0x03   /* Scan for pattern in memory range */
#define DMA_CMD_INJECT        0x04   /* Inject shellcode into executable region */

/* ---- DMA status flags ---- */
#define DMA_STS_BUSY          (1UL << 0)
#define DMA_STS_DONE          (1UL << 1)
#define DMA_STS_ERROR         (1UL << 2)
#define DMA_STS_ABORT         (1UL << 3)

/* ---- XIO2001 SPI command protocol ---- */
#define XIO_SPI_CMD_READ      0x03   /* SPI read command */
#define XIO_SPI_CMD_WRITE     0x02   /* SPI write command */
#define XIO_SPI_CMD_DMA_GO    0x80   /* Set GO bit in DMA_CTRL */

/* ---- DMA request structure ---- */
typedef struct {
    uint64_t host_addr;    /* Target physical address in host memory */
    uint32_t length;       /* Transfer length in bytes */
    uint8_t  command;      /* DMA_CMD_READ/WRITE/SCAN/INJECT */
    uint8_t  *local_buf;   /* Local buffer (STM32 SRAM address) */
} dma_request_t;

/* ---- TLP sniffing ---- */
#define TLP_TYPE_CPLD         0x4A   /* Completion with data */
#define TLP_TYPE_MRd          0x00   /* Memory read request */
#define TLP_TYPE_MWr          0x20   /* Memory write request */
#define TLP_TYPE_CFGRd        0x04   /* Config read request */
#define TLP_TYPE_CFGWr        0x44   /* Config write request */

typedef struct __attribute__((packed)) {
    uint32_t dw0;          /* TLP header DW0 (Fmt+Type+TC+Attr+TD+EP+Length) */
    uint32_t dw1;          /* TLP header DW1 (Requester ID + Tag + Last BE) */
    uint32_t dw2;          /* TLP header DW2 (Address low) */
    uint32_t dw3;          /* TLP header DW3 (Address high, for 64-bit) */
} tlp_header_t;

/* ---- Function prototypes ---- */
void xio_init(uint16_t vid, uint16_t pid);
void xio_write_reg(uint16_t reg, uint32_t value);
uint32_t xio_read_reg(uint16_t reg);
void xio_set_vid_pid(uint16_t vid, uint16_t pid);
void xio_enable_dma(void);
void xio_disable_dma(void);
int  xio_dma_execute(const dma_request_t *req);
int  xio_is_link_up(void);
void xio_sniff_tlps(uint8_t *buf, uint16_t max_len);
void xio_reset(void);

#endif /* XIO2001_H */