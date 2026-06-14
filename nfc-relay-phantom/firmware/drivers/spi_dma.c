/*
 * SPI DMA Driver for NFC Relay Phantom
 * Handles SPI1 (PN5180) and SPI2 (W25Q128) with DMA support
 *
 * Copyright (c) 2026 Hacker Devices. Licensed under GPL-2.0.
 */

#include "spi_dma.h"
#include "../board.h"
#include "../registers.h"

/* DMA buffers in SRAM3 for zero-copy */
static uint8_t spi1_dma_tx_buf[SPI1_DMA_BUF_SIZE];
static uint8_t spi1_dma_rx_buf[SPI1_DMA_BUF_SIZE];
static uint8_t spi2_dma_tx_buf[SPI2_DMA_BUF_SIZE];
static uint8_t spi2_dma_rx_buf[SPI2_DMA_BUF_SIZE];

static volatile bool spi1_dma_done = true;
static volatile bool spi2_dma_done = true;

/* ======================================================================
 * SPI Initialization
 * ====================================================================== */
void spi_init(void) {
    /* Enable DMA1 clock */
    RCC->AHB1ENR |= (1 << 0);  /* DMA1EN */

    /* --- SPI1: Master, 20 MHz, CPOL=0, CPHA=0, 8-bit --- */
    SPI1->CR1 = 0;  /* Disable first */
    /* BR[2:0] = 101 → fPCLK/64 = 120/64 ≈ 1.875 MHz (will change to 20MHz via prescaler) */
    /* For 20 MHz: fPCLK/6 → BR=010 but need fPCLK ≥ 2*20MHz. Let's use /4 = 30MHz */
    /* Actually: APB2=120MHz, /8 = 15MHz is safe. /4 = 30MHz might be too fast. */
    /* Use /8 = 15 MHz for reliable PN5180 communication */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
              | (2 << SPI_CR1_BR_Pos);  /* BR=010 → /8 = 15 MHz */
    SPI1->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_DMAEN;
    SPI1->CR1 |= SPI_CR1_SPE;

    /* --- SPI2: Master, 40 MHz (use /4 = 30 MHz for safety) --- */
    SPI2->CR1 = 0;
    /* APB1=120MHz, /4 = 30 MHz */
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
              | (1 << SPI_CR1_BR_Pos);  /* BR=001 → /4 = 30 MHz */
    SPI2->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_DMAEN;
    SPI2->CR1 |= SPI_CR1_SPE;

    /* Configure DMA channels */
    /* SPI1_RX: DMA1 Channel 2 (Stream 0) */
    /* SPI1_TX: DMA1 Channel 3 (Stream 1) */
    /* SPI2_RX: DMA1 Channel 4 (Stream 2) */
    /* SPI2_TX: DMA1 Channel 5 (Stream 3) */

    NVIC_ISER0 |= (1 << 11);   /* DMA1 Channel 2 interrupt */
    NVIC_ISER0 |= (1 << 12);   /* DMA1 Channel 3 interrupt */
    NVIC_ISER0 |= (1 << 13);   /* DMA1 Channel 4 interrupt */
    NVIC_ISER0 |= (1 << 14);   /* DMA1 Channel 5 interrupt */
}

/* ======================================================================
 * Blocking SPI Transfer
 * ====================================================================== */
void spi_transfer_blocking(spi_bus_t bus, const uint8_t *tx, uint8_t *rx, size_t len) {
    SPI_TypeDef *spi = (bus == SPI_BUS_1) ? SPI1 : SPI2;

    if (bus == SPI_BUS_1) SPI1_NS_LOW();
    else SPI2_NS_LOW();

    for (size_t i = 0; i < len; i++) {
        /* Wait for TX empty */
        while (!(spi->ISR & SPI_SR_TXE));
        spi->DR = tx ? tx[i] : 0xFF;

        /* Wait for RX ready */
        while (!(spi->ISR & SPI_SR_RXNE));
        uint8_t dummy = spi->DR;
        if (rx) rx[i] = dummy;
    }

    /* Wait for BSY clear */
    while (spi->SR & SPI_SR_BSY);

    if (bus == SPI_BUS_1) SPI1_NS_HIGH();
    else SPI2_NS_HIGH();
}

/* ======================================================================
 * DMA SPI Transfer (non-blocking)
 * ====================================================================== */
void spi_transfer_dma(spi_bus_t bus, const uint8_t *tx, uint8_t *rx, size_t len) {
    SPI_TypeDef *spi = (bus == SPI_BUS_1) ? SPI1 : SPI2;
    volatile bool *done = (bus == SPI_BUS_1) ? &spi1_dma_done : &spi2_dma_done;
    uint8_t *dma_tx = (bus == SPI_BUS_1) ? spi1_dma_tx_buf : spi2_dma_tx_buf;
    uint8_t *dma_rx = (bus == SPI_BUS_1) ? spi1_dma_rx_buf : spi2_dma_rx_buf;

    /* Copy TX data to DMA buffer */
    if (tx) {
        for (size_t i = 0; i < len && i < SPI1_DMA_BUF_SIZE; i++) {
            dma_tx[i] = tx[i];
        }
    } else {
        for (size_t i = 0; i < len && i < SPI1_DMA_BUF_SIZE; i++) {
            dma_tx[i] = 0xFF;
        }
    }

    *done = false;

    if (bus == SPI_BUS_1) SPI1_NS_LOW();
    else SPI2_NS_LOW();

    /* Configure DMA RX channel */
    DMA1_Channel[2 + bus * 2].CCR = DMA_CCR_MINC | DMA_CCR_TCIE;
    DMA1_Channel[2 + bus * 2].CPAR = (uint32_t)&spi->DR;
    DMA1_Channel[2 + bus * 2].CMAR = (uint32_t)dma_rx;
    DMA1_Channel[2 + bus * 2].CNDTR = len;

    /* Configure DMA TX channel */
    DMA1_Channel[3 + bus * 2].CCR = DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE;
    DMA1_Channel[3 + bus * 2].CPAR = (uint32_t)&spi->DR;
    DMA1_Channel[3 + bus * 2].CMAR = (uint32_t)dma_tx;
    DMA1_Channel[3 + bus * 2].CNDTR = len;

    /* Enable DMA channels */
    DMA1_Channel[2 + bus * 2].CCR |= DMA_CCR_EN;
    DMA1_Channel[3 + bus * 2].CCR |= DMA_CCR_EN;

    /* Enable SPI DMA requests */
    spi->CR2 |= SPI_CR2_DMAEN;
}

bool spi_dma_busy(spi_bus_t bus) {
    volatile bool *done = (bus == SPI_BUS_1) ? &spi1_dma_done : &spi2_dma_done;
    return !(*done);
}

void spi_dma_wait(spi_bus_t bus) {
    volatile bool *done = (bus == SPI_BUS_1) ? &spi1_dma_done : &spi2_dma_done;
    while (!(*done));
}

/* ======================================================================
 * DMA Interrupt Handlers
 * ====================================================================== */
void DMA1_Channel2_IRQHandler(void) {
    /* SPI1 RX complete */
    if (DMA1->ISR & (1 << 1)) {  /* TCIF2 */
        DMA1->IFCR = (1 << 1);
        SPI1->CR2 &= ~SPI_CR2_DMAEN;
        SPI1_NS_HIGH();
        spi1_dma_done = true;
    }
}

void DMA1_Channel3_IRQHandler(void) {
    /* SPI1 TX complete (we rely on RX complete for full transfer) */
    if (DMA1->ISR & (1 << 5)) {
        DMA1->IFCR = (1 << 5);
    }
}

void DMA1_Channel4_IRQHandler(void) {
    /* SPI2 RX complete */
    if (DMA1->ISR & (1 << 9)) {
        DMA1->IFCR = (1 << 9);
        SPI2->CR2 &= ~SPI_CR2_DMAEN;
        SPI2_NS_HIGH();
        spi2_dma_done = true;
    }
}

void DMA1_Channel5_IRQHandler(void) {
    /* SPI2 TX complete */
    if (DMA1->ISR & (1 << 13)) {
        DMA1->IFCR = (1 << 13);
    }
}