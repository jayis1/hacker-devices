/*
 * spi.c — WireReaper SPI driver: sniffer, master, and slave emulator
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — GPL-2.0
 *
 * Two SPI channels for probe interface. Master mode for flash dumping
 * and peripheral control. Slave mode for host fuzzing. High-speed
 * sampling (>25 MHz) is delegated to the iCE40 FPGA co-processor.
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"

/* SPI peripheral instances — using STM32H7 extended SPI */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04: actual H7 CFG1 */
    volatile uint32_t SR;       /* 0x08 */
    volatile uint32_t DR;       /* 0x0C — H7 uses 16-bit access */
    uint32_t reserved[4];
    volatile uint32_t CFG2;     /* 0x24: H7 config 2 */
    volatile uint32_t IER;      /* 0x28: interrupt enable */
    volatile uint32_t SR2;      /* 0x2C: status 2 (H7) */
    volatile uint32_t IPR;      /* 0x30 */
} h7_spi_t;

static h7_spi_t *const spi_devs[NUM_SPI_CHANNELS] = {
    (h7_spi_t *)SPI0_BASE,
    (h7_spi_t *)SPI1_BASE,
};

/* ---- GPIO setup for SPI ---- */
static void spi_gpio_init(int ch) {
    /* Enable GPIO ports */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOB | RCC_AHB1ENR_GPIOC |
                   RCC_AHB1ENR_GPIOI | RCC_AHB1ENR_GPIOA;

    gpio_t *port; uint32_t sck, miso, mosi, cs_port, cs;
    uint32_t af;

    if (ch == 0) {
        port = GPIO(SPI0_PORT); sck = SPI0_SCK_PIN;
        miso = SPI0_MISO_PIN; mosi = SPI0_MOSI_PIN;
        cs_port = SPI0_NCS_PORT; cs = SPI0_NCS_PIN;
        af = 5; /* SPI2 AF on PI1-3 */
    } else {
        port = GPIO(SPI1_PORT); sck = SPI1_SCK_PIN;
        miso = SPI1_MISO_PIN; mosi = SPI1_MOSI_PIN;
        cs_port = SPI1_NCS_PORT; cs = SPI1_NCS_PIN;
        af = 6; /* SPI3 AF on PC10-12 */
    }

    /* SCK, MISO, MOSI as AF */
    uint32_t pins[3] = {sck, miso, mosi};
    for (int i = 0; i < 3; i++) {
        uint32_t p = pins[i];
        port->MODER &= ~(3U << (p * 2));
        port->MODER |= (GPIO_MODE_AF << (p * 2));
        port->OSPEEDR |= (GPIO_OSPEED_VHIGH << (p * 2));
        port->PUPDR &= ~(3U << (p * 2));
        port->AFR[p / 8] &= ~(0xFU << ((p % 8) * 4));
        port->AFR[p / 8] |= (af << ((p % 8) * 4));
    }

    /* NCS as GPIO output (manual control) */
    gpio_t *csport = GPIO(cs_port);
    csport->MODER &= ~(3U << (cs * 2));
    csport->MODER |= (GPIO_MODE_OUTPUT << (cs * 2));
    csport->OTYPER &= ~(1U << cs);
    csport->OSPEEDR |= (GPIO_OSPEED_HIGH << (cs * 2));
    csport->BSRR = (1U << (cs + 16)); /* Set NCS high (deselected) */
}

/* ---- Initialize SPI channel ---- */
void wr_spi_init(int ch) {
    if (ch < 0 || ch >= NUM_SPI_CHANNELS)
        return;

    /* Enable SPI clock */
    if (ch == 0)
        RCC_APB1ENR1 |= (1U << 14); /* SPI2 */
    else
        RCC_APB1ENR1 |= (1U << 15); /* SPI3 */

    spi_gpio_init(ch);

    h7_spi_t *spi = spi_devs[ch];
    /* Disable SPI */
    spi->CR1 = 0;

    /* Config: Master mode, 8-bit frame, Motorola mode */
    spi->CR1 = (7U << 0) |   /* BR: /256 (will adjust) */
               SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;
    spi->CR2 = 0x00070000; /* FIFO threshold 8-bit, DS=8 (H7 CFG1) */
    spi->CFG2 = (1U << 22); /* Master mode (H7 CFG2) */

    /* Enable SPI */
    spi->CR1 |= SPI_CR1_SPE;
}

/* ---- SPI master: send command, read response ---- */
int wr_spi_read(int ch, const uint8_t *cmd, int cmdlen,
                 uint8_t *buf, int len) {
    if (ch < 0 || ch >= NUM_SPI_CHANNELS)
        return WR_ERR_PARAM;

    h7_spi_t *spi = spi_devs[ch];

    /* Find NCS pin and assert low */
    uint32_t cs_port, cs;
    if (ch == 0) { cs_port = SPI0_NCS_PORT; cs = SPI0_NCS_PIN; }
    else         { cs_port = SPI1_NCS_PORT; cs = SPI1_NCS_PIN; }
    gpio_t *csp = GPIO(cs_port);
    csp->BSRR = (1U << (cs + 16)); /* NCS low */

    /* Send command bytes */
    for (int i = 0; i < cmdlen; i++) {
        while (!(spi->SR & SPI_SR_TXE));
        *(volatile uint8_t *)&spi->DR = cmd[i];
    }
    /* Drain any received bytes during command phase */
    while (spi->SR & SPI_SR_RXNE)
        (void)*(volatile uint8_t *)&spi->DR;

    /* Read response: send dummy bytes to clock data out */
    for (int i = 0; i < len; i++) {
        while (!(spi->SR & SPI_SR_TXE));
        *(volatile uint8_t *)&spi->DR = 0x00;
        while (!(spi->SR & SPI_SR_RXNE));
        buf[i] = *(volatile uint8_t *)&spi->DR;
    }

    /* Wait for completion and deselect */
    while (spi->SR & SPI_SR_BSY);
    csp->BSRR = (1U << cs); /* NCS high */

    return WR_OK;
}

/* ---- SPI master: write data ---- */
int wr_spi_write(int ch, const uint8_t *cmd, int cmdlen,
                  const uint8_t *data, int datalen) {
    if (ch < 0 || ch >= NUM_SPI_CHANNELS)
        return WR_ERR_PARAM;

    h7_spi_t *spi = spi_devs[ch];
    uint32_t cs_port, cs;
    if (ch == 0) { cs_port = SPI0_NCS_PORT; cs = SPI0_NCS_PIN; }
    else         { cs_port = SPI1_NCS_PORT; cs = SPI1_NCS_PIN; }
    gpio_t *csp = GPIO(cs_port);
    csp->BSRR = (1U << (cs + 16)); /* NCS low */

    /* Send command */
    for (int i = 0; i < cmdlen; i++) {
        while (!(spi->SR & SPI_SR_TXE));
        *(volatile uint8_t *)&spi->DR = cmd[i];
    }
    /* Send data */
    for (int i = 0; i < datalen; i++) {
        while (!(spi->SR & SPI_SR_TXE));
        *(volatile uint8_t *)&spi->DR = data[i];
    }
    /* Wait for completion */
    while (spi->SR & SPI_SR_BSY);
    /* Drain RX */
    while (spi->SR & SPI_SR_RXNE)
        (void)*(volatile uint8_t *)&spi->DR;

    csp->BSRR = (1U << cs); /* NCS high */
    return WR_OK;
}

/* ---- SPI flash read (common 0x03 command) ---- */
int wr_spi_flash_dump(int ch, uint32_t addr, uint8_t *buf, int len) {
    uint8_t cmd[4];
    cmd[0] = 0x03; /* READ */
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;
    return wr_spi_read(ch, cmd, 4, buf, len);
}

/* ---- SPI flash read JEDEC ID (0x9F) ---- */
int wr_spi_flash_jedec(int ch, uint8_t *id, int idlen) {
    uint8_t cmd = 0x9F;
    return wr_spi_read(ch, &cmd, 1, id, idlen > 6 ? 6 : idlen);
}

/* ---- SPI slave emulation ---- */
static uint8_t spi_slave_resp[256];
static int     spi_slave_resp_len = 0;
static int     spi_slave_idx = 0;

IRQ_HANDLER(SPI2_IRQHandler) {
    h7_spi_t *spi = spi_devs[0];
    if (spi->SR & SPI_SR_RXNE) {
        uint8_t b = *(volatile uint8_t *)&spi->DR;
        (void)b; /* consume command byte */
        /* If master is reading, load next response byte */
        if (spi_slave_idx < spi_slave_resp_len) {
            while (!(spi->SR & SPI_SR_TXE));
            *(volatile uint8_t *)&spi->DR = spi_slave_resp[spi_slave_idx++];
        } else {
            while (!(spi->SR & SPI_SR_TXE));
            *(volatile uint8_t *)&spi->DR = 0xFF;
        }
    }
}

int wr_spi_emulate(int ch, const uint8_t *resp, int resplen) {
    if (ch < 0 || ch >= NUM_SPI_CHANNELS || resplen > 256)
        return WR_ERR_PARAM;

    h7_spi_t *spi = spi_devs[ch];

    /* Copy response data */
    memcpy(spi_slave_resp, resp, resplen);
    spi_slave_resp_len = resplen;
    spi_slave_idx = 0;

    /* Reconfigure as slave */
    spi->CR1 = 0;
    spi->CR1 = SPI_CR1_SSM | SPI_CR1_SSI; /* slave, software NSS */
    spi->CR2 = 0x00070000; /* 8-bit */
    spi->CFG2 = 0; /* Slave mode */
    spi->IER = (1U << 0); /* RX interrupt enable (H7) */

    spi->CR1 |= SPI_CR1_SPE;

    if (ch == 0)
        nvic_enable(35); /* SPI2 IRQ */
    else
        nvic_enable(36); /* SPI3 IRQ */

    return WR_OK;
}

/* ---- SPI sniffer (via FPGA) ---- */
/* When high-speed SPI sniffing is needed (>25 MHz), the iCE40 FPGA
 * samples SCK/MOSI/MISO/CS at up to 96 MS/s and packs samples into
 * a FIFO that the MCU drains via the SPI_FPGA interface.
 * The FPGA bitstream handles:
 *   - CS edge detection (transaction boundaries)
 *   - SCK edge detection and SDA sampling
 *   - Sample packing (8 bits per byte, 4 bytes per word)
 *   - FIFO full flag to MCU
 * See fpga.c for the FPGA interface driver. */

int wr_spi_sniff_start(int ch, uint32_t max_freq) {
    /* Configure FPGA for SPI sniffing on channel ch */
    if (ch < 0 || ch >= NUM_SPI_CHANNELS)
        return WR_ERR_PARAM;

    /* The FPGA sampling rate must be >= 2x max SPI frequency */
    uint32_t sample_rate = max_freq * 3; /* 3x oversampling */
    if (sample_rate > 96000000)
        sample_rate = 96000000;

    /* Send config to FPGA via SPI_FPGA */
    extern void wr_fpga_set_trigger(int ch, uint32_t mask, uint32_t val);
    wr_fpga_set_trigger(ch, 0x01, 0x01); /* CS=active trigger */

    return WR_OK;
}